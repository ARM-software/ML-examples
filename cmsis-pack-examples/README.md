# CMSIS-Pack based Machine Learning Examples

- [Introduction](#introduction)
- [Overview](#overview)
  - [Object detection](#object-detection)
  - [Keyword spotting](#keyword-spotting)
- [Building the examples](#building-the-examples)
  - [Prerequisites](#prerequisites)
    - [Tools](#tools)
    - [Packs](#packs)
  - [Download Software Packs](#download-software-packs)
  - [Generate and build the project](#generate-and-build-the-project)
  - [Application output](#application-output)
- [Trademarks](#trademarks)
- [Licenses](#licenses)

## Introduction

This repository has a collection of Machine Learning (ML) examples using the CMSIS-Pack from
[ML Embedded Evaluation Kit](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/refs/heads/main).
Currently, a couple of examples are supported:

- Object detection - detects objects in the input image.
- Keyword spotting - detects specific keywords in the input audio stream.

Use this import button to open the solution in Keil Studio Cloud: [![Open in Keil Studio](https://img.shields.io/badge/Keil%20Studio-Import-blue?logo=data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgiPz4NCjwhLS0gR2VuZXJhdG9yOiBBZG9iZSBJbGx1c3RyYXRvciAyNS40LjEsIFNWRyBFeHBvcnQgUGx1Zy1JbiAuIFNWRyBWZXJzaW9uOiA2LjAwIEJ1aWxkIDApICAtLT4NCjxzdmcgdmVyc2lvbj0iMS4xIiBpZD0iTGF5ZXJfMSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIiB4bWxuczp4bGluaz0iaHR0cDovL3d3dy53My5vcmcvMTk5OS94bGluayIgeD0iMHB4IiB5PSIwcHgiDQoJIHZpZXdCb3g9IjAgMCA0NyAxNCIgc3R5bGU9ImVuYWJsZS1iYWNrZ3JvdW5kOm5ldyAwIDAgNDcgMTQ7IiB4bWw6c3BhY2U9InByZXNlcnZlIj4NCjxzdHlsZSB0eXBlPSJ0ZXh0L2NzcyI+DQoJLnN0MHtmaWxsOiNGRkZGRkY7fQ0KPC9zdHlsZT4NCjxwYXRoIGNsYXNzPSJzdDAiIGQ9Ik00LjcsN2MwLDIuMiwxLjQsNC4xLDMuNSw0LjFjMS44LDAsMy42LTEuNCwzLjYtNC4xYzAtMi44LTEuNy00LjItMy42LTQuMkM2LjIsMi45LDQuNyw0LjcsNC43LDcgTTExLjYsMC41DQoJaDIuOXYxM2gtMi45di0xLjNjLTAuOSwxLjEtMi4zLDEuNy0zLjcsMS43QzQsMTMuOSwxLjgsMTAuNiwxLjgsN2MwLTQuMywyLjctNi45LDYuMS02LjljMS41LDAsMi44LDAuNywzLjcsMS45VjAuNXoiLz4NCjxwYXRoIGNsYXNzPSJzdDAiIGQ9Ik0xOCwwLjVIMjF2MS4yYzAuMy0wLjQsMC43LTAuOCwxLjItMS4xYzAuNS0wLjMsMS4yLTAuNCwxLjctMC40YzAuOCwwLDEuNiwwLjIsMi4zLDAuNmwtMS4yLDIuOA0KCWMtMC40LTAuMy0xLTAuNC0xLjUtMC40Yy0wLjctMC4xLTEuMywwLjItMS44LDAuN0MyMSw0LjYsMjEsNS45LDIxLDYuOHY2LjdIMThWMC41eiIvPg0KPHBhdGggY2xhc3M9InN0MCIgZD0iTTI4LjIsMC41aDIuOXYxLjJjMC43LTAuOSwxLjktMS42LDMuMS0xLjZjMS4zLDAsMi42LDAuNywzLjIsMS45YzAuOS0xLjIsMi4yLTEuOSwzLjctMS45DQoJQzQyLjcsMCw0NCwwLjksNDQuNywyLjJjMC4yLDAuNCwwLjcsMS40LDAuNywzLjN2OC4xaC0yLjlWNi4zYzAtMS41LTAuMi0yLjEtMC4yLTIuM2MtMC4yLTAuNy0wLjktMS4yLTEuNy0xLjENCgljLTAuNywwLTEuMywwLjMtMS43LDAuOWMtMC41LDAuOC0wLjYsMS45LTAuNiwyLjl2Ni43aC0yLjlWNi4zYzAtMS41LTAuMi0yLjEtMC4yLTIuM2MtMC4yLTAuNy0wLjktMS4yLTEuNy0xLjENCgljLTAuNywwLTEuMywwLjMtMS43LDAuOWMtMC41LDAuOC0wLjYsMS45LTAuNiwyLjl2Ni43aC0yLjlMMjguMiwwLjV6Ii8+DQo8L3N2Zz4NCg==&logoWidth=47)](https://studio.keil.arm.com/?import=https://github.com/Arm-Examples/mlek-corstone-300-examples.git)

## Overview

The examples presented in this repository showcase how to build and deploy end-to-end Machine
Learning applications using existing code from various CMSIS-packs. These examples are built
using Google's [TensorFlow Lite Micro framework](https://www.tensorflow.org/lite/microcontrollers)
and Arm's [ML Embedded Evaluation Kit](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/refs/heads/main/Readme.md)
API's. The embedded evaluation kit API pack has ready-to-use machine learning API's for several
use cases covering typical `voice`, `vibration` and `vision` applications.

Although the target platform here is Arm® Corstone™-300, the examples can be easily ported to new
targets, potentially using physical (or virtual) peripherals for live data being fed into the
neural network model. Corstone™-300 platform gives us the choice of executing the ML workload on
Arm® Cortex®-M55 CPU or running it more efficiently on Arm® Ethos™-U55 NPU. The examples are set
up to use the NPU by default with unsupported operators falling back on the CPU.

### Object detection

This example uses a neural network model that specialises in detecting human faces in images.
The input size for these images is 192x192 (monochrome) and the smallest face that can be
detected is of size 20x20. The output of the application will be co-ordinates for rectangular
bounding boxes for each detection.

### Keyword spotting

This example can detect up to twelve keywords in the input audio stream. The [audio file used](./resources/sample_audio.wav)
contains the keyword "down" being spoken.

More details about the input for this example can be found [here](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/refs/heads/main/docs/use_cases/kws.md#preprocessing-and-feature-extraction).

## Building the examples

We recommend using [Keil Studio Cloud](https://studio.keil.arm.com/?import=https://github.com/Arm-Examples/mlek-corstone-300-examples.git) for building these examples.
This is by far the easiest way to get started. However, it is possible to build these projects locally too and the following sections cover the requirements for such a
set up.

### Prerequisites

For developing on a local host machine, we recommend a Linux based system as we test
the flow of the instructions in that environment, but a Windows based machine should
also work.

#### Tools

The following tools are required if building on a local machine (and not using the project via
Keil Studio Cloud):

- [CMSIS-Toolbox 1.0.0](https://github.com/Open-CMSIS-Pack/devtools/releases) or higher.
- Arm Compiler 6.18 (part of Keil MDK or Arm Development Studio, evaluation version sufficient
  for compilation).
- Arm Virtual Hardware for Corstone-300 v11.18.1 or a local installation of Fixed Virtual Platform

> **NOTE**: For Linux, we recommend using the script installer as this sets up the
> basic paths required the the tools. Otherwise, paths like CMSIS_ROOT_PATH and the
> toolchain root paths in the toolchain CMake files will need to be set manually.
> The script installer will prompt for the different paths:
> ```commandline
> $ sudo chmod +x ~/Downloads/cmsis-toolbox.sh
> $ ~/Downloads/cmsis-toolbox.sh
> (cmsis-toolbox.sh): CMSIS Toolbox Installer 1.0.0 (C) 2022 ARM
> [INFO] Linux platform detected
> Enter base directory for CMSIS Toolbox [./cmsis-toolbox]: /home/user/> cmsis-toolbox-linux64/
> Enter the CMSIS_PACK_ROOT directory [/home/user/.cache/arm/packs]:
> Enter the installed Arm Compiler 6.18 directory [/home/user/ArmCompilerforEmbedded6.18/bin]: /home/user/toolchains/ArmCompilerforEmbedded6.18/bin
> ...
> Installing CMSIS Toolbox to /home/user/cmsis-toolbox-linux64...
> CMSIS Toolbox installation completed!
> To setup the bash environment run:
> $ source /home/user/cmsis-toolbox-linux64/etc/setup
> ```

#### Packs

CMSIS-Pack defines a standardized way to deliver software components, device parameters and board
support information and code. A list of available CMSIS-Packs can be found
[here](https://developer.arm.com/tools-and-software/embedded/cmsis/cmsis-packs).

### Download Software Packs

The [mlek.csolution.yml](./mlek.csolution.yml) file lists the software packs used in all projects. These can be
downloaded using the following command:

```
> csolution list packs -s mlek.csolution.yml -m > packlist.txt
> cpackget add -f packlist.txt
```

> **NOTE**: The above commands expect CMSIS_PACK_ROOT to be defined as the path
> where the packs are, or will be, installed. The environment setup script for
> CMSIS-Toolbox should set this up automatically:
> ```commandline
> $ source /home/user/cmsis-toolbox-linux64/etc/setup
> ```

### Generate and build the project

1. Use the `csolution` command to create `.cprj` project files:
   ```
   $ csolution convert -s ./mlek.csolution.yml
    kws/kws.Debug+AVH-U55.cprj - info csolution: file generated successfully
    kws/kws.Release+AVH-U55.cprj - info csolution: file generated successfully
    object-detection/object_detection.Debug+AVH-U55.cprj - info csolution: file generated successfully
    object-detection/object_detection.Release+AVH-U55.cprj - info csolution: file generated successfully
   ```
   > **NOTE**: A single project could also be generated using the context argument:
   > ```commandline
   > $ csolution convert -s ./mlek.csolution.yml -c object_detection.Release+AVH-U55
   > object-detection/object_detection.Release+AVH-U55.cprj - info csolution: file generated successfully
   > ```
   ```

2. Use the `cbuild` command to build an executable file, for example building KWS project in `Debug` type:
   ```
   $ cbuild ./kws/kws.Debug+AVH-U55.cprj -g "Unix Makefiles" -j 4
   info cbuild: Build Invocation 1.0.0 (C) 2022 ARM
      :    // output of the build steps
   info cbuild: build finished successfully!
   ```

> **Note:**
> 1. During the build process required packs may be downloaded.
> 2. The generator specified depends on CMake and the host platform OS.

### Execute project

The project is configured for execution on Arm Virtual Hardware which removes the requirement for
a physical hardware board.

- When using a Fixed Virtual Platform installed locally:
  ```
  > <path_to_installed_FVP> -a ./out/kws/AVH-U55/Debug/kws.Debug+AVH-U55.axf -C ethosu.num_macs=256
  ```
  > **NOTE**: The FVP defaults to running 128 MAC configuration for Arm® Ethos™-U55 NPU.
  > However, our default neural network model for the NPU is for 256 MAC configuration.

- [Keil Studio Cloud](https://studio.keil.arm.com/) integrates also the Arm Virtual Hardware
  VHT_Corstone_SSE-300_Ethos-U55 model. The steps to use the example are:
  - Start [Keil Studio Cloud](https://studio.keil.arm.com/) and login to the system using your
    account.
  - Drag and drop this directory (cmsis-pack-examples) into the project pane, or use the import
    button provided above, under [ML-examples](#ml-examples).
  - Select the *Active Project* from the drop-down - it should show all projects referenced in
    the `csolution` file.
  - Select *Target hardware* from the drop-down: **SSE-300 MPS3**
  - Click **Run project** which executes the project build step and then starts running on Arm
    Virtual Hardware.

> **Note:** Arm Virtual Hardware models are also available on AWS Marketplace.

### Application output

Once the project can be built successfully, the execution on target hardware will show output of
the application in `Output` window in Keil Studio Cloud. Currently, this includes the following:
  - Arm® Ethos™-U55 NPU version information
  - Information about model's memory allocation
  - Running inference on specified input
  - Output of inference
  - Simulation information such as simulated time, user time, system time, etc

The output is different for the two example applications:
  - object detection application will detect two objects on the sample input image and will
    present the detected bounding boxes for objects in the image.
  - keyword spotting application will detect a keyword in the sample audio file and will present
    the highest confidence score and the associated keyword label. Threshold, applied for a label
    to be deemed valid, is also shown

## Trademarks

- Arm® and Cortex® are registered trademarks of Arm® Limited (or its subsidiaries) in the US and/or elsewhere.
- Arm® and Ethos™ are registered trademarks or trademarks of Arm® Limited (or its subsidiaries) in the US and/or
  elsewhere.
- Arm® and Corstone™ are registered trademarks or trademarks of Arm® Limited (or its subsidiaries) in the US and/or
  elsewhere.
- Arm®, Keil® and µVision® are registered trademarks of Arm Limited (or its subsidiaries) in the US and/or elsewhere.
- TensorFlow™, the TensorFlow logo, and any related marks are trademarks of Google Inc.

## Licenses

The application samples and [resources](./resources) are provided under the Apache 2.0 license, see [License](./LICENSE).

Application input data sample files (audio or image files) and the neural network model files have
been converted into C/C++ type arrays and are distributed under Apache 2.0 license. The models have
been processed by the [Vela compiler](https://pypi.org/project/ethos-u-vela/) and then converted
into C/C++ arrays to be baked into the example applications.

| Example | Licence | Provenance |
|---------------|---------|---------|
| Keyword Spotting | Apache 2.0 | [micronet_medium](https://github.com/ARM-software/ML-zoo/raw/9f506fe52b39df545f0e6c5ff9223f671bc5ae00/models/keyword_spotting/micronet_medium/tflite_int8/) |
| Object Detection | Apache 2.0 | [yolo-fastest_192_face_v4](https://github.com/emza-vs/ModelZoo/blob/v1.0/object_detection/) |
