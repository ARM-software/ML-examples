- [ML examples](#ml-examples)
  - [Trademarks](#trademarks)
  - [Licenses](#licenses)
  - [Prerequisites](#prerequisites)
    - [Tools](#tools)
    - [Packs](#packs)
  - [Download Software Packs](#download-software-packs)
  - [Generate Project](#generate-project)
# ML examples

A collection of ML examples using the CMSIS pack from [ml-embedded-eval-kit](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/refs/heads/main). Currently, a couple of examples are supported:

- Object detection
- Keyword spotting

## Trademarks

- Arm® and Cortex® are registered trademarks of Arm® Limited (or its subsidiaries) in the US and/or elsewhere.
- Arm® and Ethos™ are registered trademarks or trademarks of Arm® Limited (or its subsidiaries) in the US and/or
  elsewhere.
- Arm® and Corstone™ are registered trademarks or trademarks of Arm® Limited (or its subsidiaries) in the US and/or
  elsewhere.
- Arm®, Keil® and µVision® are registered trademarks of Arm Limited (or its subsidiaries) in the US and/or elsewhere.
- TensorFlow™, the TensorFlow logo, and any related marks are trademarks of Google Inc.

## Licenses

The application samples are provided under the Apache 2.0 license, see [License](./LICENSE).

Application input data sample files (audio or image files) and the neural network model files have
been converted into C/C++ type arrays and are distributed under Apache 2.0 license. The models have
been processed by the [Vela compiler](https://pypi.org/project/ethos-u-vela/) and then converted
into C/C++ arrays to be baked into the example applications.

| Example | Licence | Provenance |
|---------------|---------|---------|
| Keyword Spotting | Apache 2.0 | [micronet_medium](https://github.com/ARM-software/ML-zoo/raw/9f506fe52b39df545f0e6c5ff9223f671bc5ae00/models/keyword_spotting/micronet_medium/tflite_int8/) |
| Object Detection | Apache 2.0 | [object_detection](https://github.com/emza-vs/ModelZoo/blob/v1.0/object_detection/) |

## Prerequisites

For developing on a local host machine, we recommend a Linux based system as we test
the flow of the instructions in that environment, but a Windows based machine should
also work.
### Tools

The following tools are required if building on a local machine (and not using the project via Keil Studio Cloud):

- [CMSIS-Toolbox 1.0.0](https://github.com/Open-CMSIS-Pack/devtools/releases) or higher.
- Arm Compiler 6.18 (part of Keil MDK or Arm Development Studio, evaluation version sufficient for compilation).
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

### Packs

Access to the Internet to download the required packs are listed in the file
[`mlek.csolution.yml`](./mlek.csolution.yml).

## Download Software Packs

The `csolution.yml` file lists the software packs used in all projects. These can be downloaded using the following command:

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

## Generate Project

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

## Execute Project

The project is configured for execution on Arm Virtual Hardware which removes the requirement for a physical hardware board.

- When using a Fixed Virtual Platform installed locally:
  ```
  > <path_to_installed_FVP> -a ./out/kws/AVH-U55/Debug/kws.Debug+AVH-U55.axf -C ethosu.num_macs=256
  ```
  > **NOTE**: The FVP defaults to running 128 MAC configuration for Arm Ethos-U55 NPU.
  > However, our default neural network model for the NPU is for 256 MAC configuration.

- [Keil Studio Cloud](https://studio.keil.arm.com/) integrates also the Arm Virtual Hardware VHT_Corstone_SSE-300_Ethos-U55 model. The steps to use the example are:
  - Start [Keil Studio Cloud](https://studio.keil.arm.com/) and login to the system using your account.
  - Drag and drop this directory (cmsis-pack-examples) into the project pane.
  - Select the *Active Project* from the drop down - it should show all projects referenced in the `csolution` file.
  - Select *Target hardware* from the drop-down: **SSE-300 MPS3**
  - Click **Run project** which executes the project build step and then starts running on Arm Virtual Hardware.

> **Note:** Arm Virtual Hardware models are also available on AWS Marketplace.
