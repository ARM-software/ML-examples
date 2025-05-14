#
# SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0
#

# Exporting Conditioners to .onnx and LiteRT format (.tflite)
import argparse
import json
import logging
import os
import subprocess
from typing import Any

import torch
from utils_load_model import load_model

logging.basicConfig(level=logging.INFO)

os.environ["CUDA_VISIBLE_DEVICES"] = ""
DEVICE = torch.device("cpu")


## ----------------- Utility Functions -------------------
def get_conditioners(model: str):
    """Load the conditioners module from the AudioGen model.
    Args:
        model (str): The AudioGen model.
    Returns:
        sao_t5_cond: The T5 encoder.
        sao_seconds_total_cond: The seconds_total conditioner.
    """
    cond_model = model.conditioner
    t5_cond = cond_model.conditioners["prompt"]
    seconds_total_cond = cond_model.conditioners["seconds_total"]

    return t5_cond, seconds_total_cond


def get_conditioners_example_input(seconds_total: float, seq_length: int):
    """Provide example input tensors for the AudioGen Conditioners submodule.
    Args:
        seconds_total (float): The total seconds for the audio.
        seq_length (int): The sequence length for the T5 encoder.
    Returns:
        input_ids (torch.Tensor): The input IDs tensor for the T5 encoder.
        attention_mask (torch.Tensor): The attention mask tensor for the T5 encoder.
        seconds_total (torch.Tensor): The seconds_total tensor.
    """
    from transformers import AutoTokenizer

    tokenizer = AutoTokenizer.from_pretrained("t5-base")
    encoded = tokenizer(
        text="birds singing in the morning",
        truncation=True,
        max_length=seq_length,
        padding="max_length",
        return_tensors="pt",
    )

    # Create the input_ids and attention_mask tensors for sao conditioners
    input_ids = encoded["input_ids"]
    attention_mask = encoded["attention_mask"]

    # Create the seconds_total tensor
    seconds_total = torch.tensor([seconds_total], dtype=torch.float)

    return (
        input_ids,
        attention_mask,
        seconds_total,
    )


def get_conditioners_module(model):
    """
    Wrap both the T5 encoder and seconds_total conditioner in a single module.
    """
    # Load the SAO conditioners
    sao_t5_cond, sao_seconds_total_cond = get_conditioners(model)

    # Return the conditioners module
    return ConditionersModule(
        sao_t5_cond=sao_t5_cond,
        sao_seconds_total_cond=sao_seconds_total_cond,
    )


def convert_conditioners_to_onnx(model, example_inputs, output_path):
    """Convert the Pytorch Conditioners model to ONNX format.
    Args:
        model (torch.nn.Module): The Pytorch model to convert.
        example_inputs (tuple): A tuple of example input tensors for the model.
        output_path (str): The path to save the converted ONNX model.
    Returns:
        str: The path to the converted ONNX model.
    """
    # Export the model to ONNX format
    torch.onnx.export(
        model,
        example_inputs,
        output_path,
        input_names=["input_ids", "attention_mask", "seconds_total"],
        output_names=["cross_attention_input", "cross_attention_masks", "global_cond"],
        opset_version=15,
    )
    print(f"Model exported to {output_path}")
    return output_path


## ----------------- Wrapper Classes -------------------
class ExportableNumberConditioner(torch.nn.Module):
    """NumberConditioner Module. Take a list of floats,
    normalizes them for a given range, and returns a list of embeddings.
    """

    def __init__(
        self,
        numberConditioner,
    ):
        super(ExportableNumberConditioner, self).__init__()

        self.min_val = numberConditioner.min_val
        self.max_val = numberConditioner.max_val

        self.embedder = numberConditioner.embedder

    def forward(self, floats: torch.tensor) -> Any:
        floats = floats.clamp(self.min_val, self.max_val)

        normalized_floats = (floats - self.min_val) / (self.max_val - self.min_val)

        # Cast floats to same type as embedder
        embedder_dtype = next(self.embedder.parameters()).dtype
        normalized_floats = normalized_floats.to(embedder_dtype)

        float_embeds = self.embedder(normalized_floats).unsqueeze(1)

        return float_embeds, torch.ones(float_embeds.shape[0], 1)

class ConditionersModule(torch.nn.Module):
    """Conditioners Module. Takes the T5 encoder and seconds_total conditioner,
    and returns the cross-attention inputs and global conditioning inputs.
    """

    def __init__(
        self,
        sao_t5_cond: torch.nn.Module,
        sao_seconds_total_cond: torch.nn.Module,
    ):
        super().__init__()
        self.sao_t5 = sao_t5_cond
        self.sao_seconds_total_cond = ExportableNumberConditioner(
            sao_seconds_total_cond
        )

        # Use float
        self.sao_t5 = (
            self.sao_t5.to("cpu").to(dtype=torch.float).eval().requires_grad_(False)
        )
        self.sao_seconds_total_cond = self.sao_seconds_total_cond.to(dtype=torch.float)

    def forward(
        self,
        input_ids: torch.Tensor,
        attention_mask: torch.Tensor,
        seconds_total: torch.Tensor,
    ):
        # Get the projections and conditioner results
        with torch.no_grad():
            t5_embeddings = self.sao_t5.model(
                input_ids=input_ids, attention_mask=attention_mask
            )["last_hidden_state"]
            # Resize the embeddings and attention mask to 64 to match DiT model
            t5_embeddings = t5_embeddings[:, :64, :]
            attention_mask = attention_mask[:, :64]
            # Get the T5 projections
            t5_proj = self.sao_t5.proj_out(t5_embeddings.float())
            t5_proj = t5_proj * attention_mask.unsqueeze(-1).float()
            t5_mask = attention_mask.float()

        # Get seconds_total conditioner results
        seconds_total_embedding, seconds_total_mask = self.sao_seconds_total_cond(
            seconds_total
        )

        # Concatenate all cross-attention inputs (t5_embedding, seconds_total) over the sequence dimension
        # Assumes that the cross-attention inputs are of shape (batch, seq, channels)
        cross_attention_input = torch.cat(
            [
                t5_proj,
                seconds_total_embedding,
            ],
            dim=1,
        )
        cross_attention_masks = torch.cat(
            [
                t5_mask,
                seconds_total_mask,
            ],
            dim=1,
        )

        # Concatenate all global conditioning inputs (seconds_start, seconds_total) over the channel dimension
        # Assumes that the global conditioning inputs are of shape (batch, channels)
        global_cond = torch.cat(
            [
                seconds_total_embedding
            ],
            dim=-1,
        )
        global_cond = global_cond.squeeze(1)

        return cross_attention_input, cross_attention_masks, global_cond


## ----------------- Exporting Conditioners to LiteRT format -------------------
def export_conditioners(args) -> None:
    """Export the conditioners of the AudioGen model to LiteRT format."""

    model_config = None
    dtype = torch.float32

    # Load the model configuration
    logging.info("Loading the AudioGen Checkpoint...")
    with open(args.model_config, encoding="utf-8") as f:
        model_config = json.load(f)
    model, model_config = load_model(
        model_config,
        args.ckpt_path,
        pretrained_name=None,
        device=DEVICE,
    )

    # Load the conditioners and the t5 model
    conditioners = get_conditioners_module(model=model)
    conditioners = conditioners.to(dtype).eval().requires_grad_(False)
    conditioners_example_input = get_conditioners_example_input(
        seq_length=128, seconds_total=10.0
    )
    # Export the conditioners first to ONNX
    logging.info("Starting Conditioners export to ONNX...\n")
    onnx_model_path = convert_conditioners_to_onnx(
        conditioners,
        conditioners_example_input,
        output_path="./conditioners.onnx",
    )
    logging.info(
        "Conditioners in ONNX format has been saved to %s/conditioners.onnx",
        onnx_model_path,
    )
    logging.info("Starting ONNX to LiteRT conversion...\n")
    # Convert the ONNX model to LiteRT format - Use command line for faster conversion
    onnx2tf_command = [
        "onnx2tf",
        "-i",
        str(onnx_model_path),
        "-o",
        "./conditioners_tflite",
    ]
    # Call the command line tool
    subprocess.run(onnx2tf_command, check=True)
    logging.info(
        "Conditioners in LiteRT format has been saved to %s/conditioners_tflite",
    )


def main():
    """Main function to export the AudioGen Conditioners model to onnx and then LiteRT format."""
    parser = argparse.ArgumentParser()
    # Export the model to ONNX and LiteRT format
    parser.add_argument(
        "-m",
        "--model_config",
        type=str,
        help="Path to the model configuration file.",
        required=True,
    )
    parser.add_argument(
        "--ckpt_path",
        type=str,
        help="Path to the model checkpoint file.",
        required=True,
    )
    export_conditioners(parser.parse_args())


if __name__ == "__main__":
    main()
