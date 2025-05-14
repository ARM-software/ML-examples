#
# SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0
#

#!/bin/bash

# Install individual packages
echo "Installing required packages for the Audiogen module..."

# ai-edge-torch
pip install ai-edge-torch==0.4.0 \
    "tf-nightly>=2.19.0.dev20250208" \
    "ai-edge-litert-nightly>=1.1.2.dev20250305" \
    "ai-edge-quantizer-nightly>=0.0.1.dev20250208"

# Stable audio tools
pip install "stable_audio_tools==0.0.19"


# Working out dependency issues, this combination of packages has been tested on different systems (Linux and MacOS).
pip install --no-deps "torch==2.6.0" \
                      "torchaudio==2.6.0" \
                      "torchvision==0.21.0" \
                      "protobuf==5.29.4" \
                      "numpy==1.26.4" \

# Packages to convert via onnx 
pip install --no-deps onnx \
                      onnxsim \
                      onnx2tf \
                      tensorflow \
                      tf_keras \
                      onnx_graphsurgeon \
                      ai_edge_litert \
                      sng4onnx 

echo "Finished installing required packages for AudioGen submodules conversion."
echo "To start converting the Conditioners, DiT and Autoencoder modules conversion, use the following command:"
echo "python ./scripts/export_{MODEL-T0-CONVERT}.py"
