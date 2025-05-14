#
# SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0
#

import logging
from typing import Any, Dict, Optional, Tuple

import torch

from stable_audio_tools.models.factory import create_model_from_config
from stable_audio_tools.models.pretrained import get_pretrained_model
from stable_audio_tools.models.utils import load_ckpt_state_dict

MODEL = None
SAMPLE_RATE = 44100
SAMPLE_SIZE = 524288
DEVICE = torch.device("cpu")


## ----------------- Loading SAO Model -------------------
def copy_state_dict(model, state_dict):
    """Load state_dict to model, but only for keys that match exactly.

    Args:
        model (nn.Module): model to load state_dict.
        state_dict (OrderedDict): state_dict to load.
    """
    model_state_dict = model.state_dict()
    for key in state_dict:
        if key in model_state_dict and state_dict[key].shape == model_state_dict[key].shape:
            if isinstance(state_dict[key], torch.nn.Parameter):
                # backwards compatibility for serialized parameters
                state_dict[key] = state_dict[key].data
            model_state_dict[key] = state_dict[key]

    model.load_state_dict(model_state_dict, strict=False)

def load_model(
    model_config: Optional[Dict[str, Any]] = None,
    model_ckpt_path: Optional[str] = None,
    pretrained_name: Optional[str] = None,
    pretransform_ckpt_path: Optional[str] = None,
    device: torch.device = DEVICE,
) -> Tuple[torch.nn.Module, Dict[str, Any]]:
    """Load the AudioGen model and its configuration.

    Either a pretrained model (via `pretrained_name`) or a freshly constructed one
    (via `model_config` + `model_ckpt_path`) will be loaded.

    Args:
        model_config: Configuration dict for creating the model.
        model_ckpt_path: Path to a model checkpoint file.
        pretrained_name: Name of a model to load from the repo.
        pretransform_ckpt_path: Optional path to a pretransform checkpoint.
        device: Torch device to map the model to.

     Returns:
        A tuple of (model, model_config), where `model` is in eval mode
        and cast to float, and `model_config` contains sample_rate/size, etc.
    """
    global MODEL, SAMPLE_RATE, SAMPLE_SIZE

    if pretrained_name is not None:
        logging.info("Loading pretrained model: %s", pretrained_name)
        model, model_config = get_pretrained_model(pretrained_name)

    elif model_config is not None:
        if model_ckpt_path is None:
            raise ValueError(
                "model_ckpt_path must be provided when specifying model_config"
            )
        logging.info("Creating model from config")
        model = create_model_from_config(model_config)

        logging.info("Loading model checkpoint from: %s", model_ckpt_path)
        # Load checkpoint
        copy_state_dict(model, load_ckpt_state_dict(model_ckpt_path))
        logging.info("Done loading model checkpoint")

    SAMPLE_RATE = model_config["sample_rate"]
    SAMPLE_SIZE = model_config["sample_size"]

    if pretransform_ckpt_path is not None:
        logging.info("Loading pretransform checkpoint from %r", pretransform_ckpt_path)
        model.pretransform.load_state_dict(
            load_ckpt_state_dict(pretransform_ckpt_path), strict=False
        )
        logging.info("Done loading pretransform.")
    
    model.to(device).eval().requires_grad_(False)
    model = model.to(torch.float)
    model.pretransform.model_half=False

    print("Done loading model")
    return model, model_config
