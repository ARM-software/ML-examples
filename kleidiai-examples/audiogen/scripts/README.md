<!--
    SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

    SPDX-License-Identifier: Apache-2.0
-->

# Building and Running the Audio Generation Application on Arm® CPUs with the Stable Open Audio Small Model

## Goal
This guide will show you how to convert the Stable Open Audio Small Model to LiteRT-compatible form to run on Arm® CPUs with the LiteRT runtime.

### Converting the Stable Open Audio Small Model to LiteRT format
The Stable Open Audio Small Model is made of three submodules:
- Conditioners (Text conditioner and number conditioners)
- Dffusion Transformer (DiT)
- AutoEncoder.

We will explore two different conversion routes, to convert the submodules to LiteRT format.

1. __ONNX → LiteRT__ using the [onnx2tf](https://github.com/PINTO0309/onnx2tf) tool. This is the traditional two-step approach (<strong>PyTorch</strong> → <strong>ONNX</strong> → <strong>LiteRT</strong>). We will use it to convert the Conditioners submodule.

2. __PyTorch → LiteRT__ using the [Google AI Edge Torch](https://developers.googleblog.com/en/ai-edge-torch-high-performance-inference-of-pytorch-models-on-mobile-devices/) tool. This method, currently under active development, aims to simplify the conversion by performing it in a single step. We will use this tool to convert the DiT and AutoEncoder submodules.

### Create a virtual enviroment and install dependencies.

#### Step 1
Create and activate a virtual environment (it is recommended to use Python 3.10 for compatibility with the specified packages):
```bash
python3.10 -m venv .venv
source .venv/bin/activate
```
#### Step 2
Install the required dependencies. These dependencies are specified in [`install_requirements.sh`](/install_requirements.sh). You can install them by using a bash script (option A) or manually using pip install (option B).

<strong> Option A</strong>
```bash
# Option A (with .venv activated)
bash install_requirements.sh
```

<strong> Option B</strong>
```bash
# Option B (with .venv activated)
# Packages for the ai-edge-torch tool
pip install ai-edge-torch==0.4.0 \
    "tf-nightly>=2.19.0.dev20250208" \
    "ai-edge-litert-nightly>=1.1.2.dev20250305" \
    "ai-edge-quantizer-nightly>=0.0.1.dev20250208"

# Stable-Audio Tools
pip install "stable_audio_tools==0.0.19"

# Working out dependency issues, this combination of packages has been tested on different systems (Linux® and macOS®).
pip install --no-deps "torch==2.6.0" \
                      "torchaudio==2.6.0" \
                      "torchvision==0.21.0" \
                      "protobuf==5.29.4" \
                      "numpy==1.26.4" \

# Packages to convert using ONNX
pip install --no-deps onnx \
                      onnxsim \
                      onnx2tf \
                      tensorflow \
                      tf_keras \
                      onnx_graphsurgeon \
                      ai_edge_litert \
                      sng4onnx 
```

> [!NOTE]
>
> If you are using GPU on your machine, you might faced the following error:
> ```bash
> Traceback (most recent call last):
>   File "/home/<user>/Workspace/tflite/env3_10/lib/python3.10/site-packages/torch/_inductor/runtime/hints.py", line 46, in <module>
>     from triton.backends.compiler import AttrsDescriptor
> ImportError: cannot import name 'AttrsDescriptor' from 'triton.backends.compiler' (/home/<user>/Workspace/tflite/env3_10/lib/> python3.10/site-packages/triton/backends/compiler.py)
> 
> During handling of the above exception, another exception occurred:
> 
> Traceback (most recent call last):
>   File "/home/<user>/Workspace/tflite/audiogen/./scripts/export_dit_autoencoder.py", line 6, in <module>
>     import ai_edge_torch
>   File "/home/<user>/Workspace/tflite/env3_10/lib/python3.10/site-packages/ai_edge_torch/__init__.py", line 16, in <module>
>     from ai_edge_torch._convert.converter import convert
>   File "/home/<user>/Workspace/tflite/env3_10/lib/python3.10/site-packages/ai_edge_torch/_convert/converter.py", line 21, in > <module>
>     from ai_edge_torch._convert import conversion
>   File "/home/<user>/Workspace/tflite/env3_10/lib/python3.10/site-packages/ai_edge_torch/_convert/conversion.py", line 23, in > <module>
>     from ai_edge_torch._convert import fx_passes
>   File "/home/<user>/Workspace/tflite/env3_10/lib/python3.10/site-packages/ai_edge_torch/_convert/fx_passes/__init__.py", line 21, > in <module>
>     from ai_edge_torch._convert.fx_passes.optimize_layout_transposes_pass import OptimizeLayoutTransposesPass
> .
> .
> .
> ImportError: cannot import name 'AttrsDescriptor' from 'triton.compiler.compiler' (/home/<user>/Workspace/tflite/env3_10/lib/> python3.10/site-packages/triton/compiler/compiler.py)
> ```
> Please use triton 3.2.0 as the following:
> ```bash
> pip install triton==3.2.0
> ```


### Convert Conditioners Submodule
The Conditioners submodule is based on the <strong>T5Encoder</strong> model. We convert it first to <strong>ONNX</strong>, then to <strong>LiteRT</strong> format. All details are implemented in [`scripts/export_conditioners.py`](/scripts/export_conditioners.py), which includes the following steps:

  1. Load the Conditioners submodule from the Stable Open Audio Small Model configuration and checkpoint.
  2. Export the Conditioners submodule to ONNX via `torch.onnx.export()`.
  3. Convert the resulting `.onnx` file to LiteRT using `onnx2tf`.

The two conversion steps (PyTorch -> ONNX and ONNX -> LiteRT) are defined as follows:

<strong> PyTorch -> ONNX </strong>
```python
# Export to ONNX
torch.onnx.export(
        model,
        example_inputs,
        output_path,
        input_names=[], #Model inputs, a list of input tensors
        output_names=[], #Model outputs, a list of output tensors
        opset_version=15,
    )
```

<strong> ONNX -> LiteRT </strong>
```bash
# Conversion to LiteRT format
onnx2tf -i "input_onnx_model_path" -o "output_folder_path"
```
_or within a Python script_:
```python
import subprocess

onnx2tf_command = [
  "onnx2tf",
  "-i", str(input_onnx_model_path),
  "-o", str(output_folder_path),
]
# Call the command line tool
subprocess.run(onnx2tf_command, check=True)
```
Converting an `.onnx` model to `.tflite`, creates a folder containing models with different precisions (e.g., float16, float32). We will be using the float32.tflite model for on-device inference.

To run the [`scripts/export_conditioners.py`](/scripts/export_conditioners.py) script, use the following command (ensure your .venv is still active):

```bash
python3 ./scripts/export_conditioners.py --model_config "../sao_small_distilled/sao_small_distilled_1_0_config.json" --ckpt_path "../sao_small_distilled/sao_small_distilled_1_0.ckpt"
```

###  Convert DiT and AutoEncoder Submodules
To convert the DiT and AutoEncoder submodules, we use the [Generative API](https://github.com/google-ai-edge/ai-edge-torch/tree/main/ai_edge_torch/generative/) provided in by the `ai-edge-torch` tools. This API supports exporting a PyTorch model directly to LiteRT following three mains steps; model re-authoring, quantization, and finally conversion.

Here is a code snippet illustrating how the API works in practice.
```python
import ai_edge_torch
from ai_edge_torch.generative.quantize import quant_recipe

# Specify the quantization format
quant_config = quant_recipes.full_int8_dynamic_recipe()
# Iniitate the conversion
edge_model = ai_edge_torch.convert(
    model, example_inputs, quant_config=quant_config
)
```
Notes on the arguments for `ai_edge_torch.convert()`:
- __model__: The PyTorch model to be converted. This should be the pre-trained model loaded from the `.config` and `.ckpt` files, and set to evaluation mode (model.eval()).
- __example_inputs__: A tuple of torch.Tensor objects. These are dummy input tensors that match the expected shape and type of your model's forward pass arguments. For models with multiple inputs, provide them as a tuple in the correct order.

To convert the DiT and AutoEncoder submodules, run the [`export_dit_autoencoder.py`](/scripts/export_dit_autoencoder.py) script using the following command (ensure your .venv is still active):

```bash
python3 ./scripts/export_dit_autoencoder.py --model_config "../sao_small_distilled/sao_small_distilled_1_0_config.json" --ckpt_path "../sao_small_distilled/sao_small_distilled_1_0.ckpt"
# Optional Paramters --output_path "./"
```

The three LiteRT format models will be required to run the audiogen application on Android™ device.

You can now follow the instructions located in the `app/` directory to build the audio generation application.