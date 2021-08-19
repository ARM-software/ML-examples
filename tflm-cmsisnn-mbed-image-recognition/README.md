# Image recognition on STMicroelectonics(TM) STM32F4 using Arm(R) Mbed(TM) CLI 2

## Table of Contents

-   [Introduction](#introduction)
-   [Operating system](#operating-system)
-   [Hardware](#hardware)
-   [The model](#the-model)
-   [Building the image recognition application](#building-the-image-recognition-application)
    -   [Prerequisites](#prerequisites)
        -   [Mbed CLI 2](#mbed-cli-2)
        -   [Arm GCC toolchain](#arm-gcc-toolchain)
        -   [CMake](#cmake)
        -   [Ninja](#ninja)
    -   [Compiling and flashing](#compiling-and-flashing)
        -   [CMSIS-NN Performance analysis](#cmsis-nn-performance-analysis)
        -   [Compiler configuration considerations](#compiler-configuration-considerations)
            -   [Optimization level](#optimization-level)
            -   [NDEBUG](#ndebug)
    -   [Testing performance](#testing-performance)
    -   [Resetting the project](#resetting-the-project)

## Introduction
This is a project to build a TensorFlow Lite for Microcontrollers (TFLM) and CMSIS-NN image recognition demo for the Discovery STM32F746G board using Mbed CLI 2. The core of the project is based on this [Image recognition example](https://github.com/tensorflow/tensorflow/tree/0acc4e3260266beb94469de434642b4f7f4985a0/tensorflow/lite/micro/examples/image_recognition_experimental). The model is trained on the [CIFAR-10 dataset](https://www.cs.toronto.edu/~kriz/cifar.html) and can classify 10 different classes:
- plane
- car
- bird
- cat
- deer
- dog
- frog
- horse
- ship
- truck

## Operating system

These instructions have been written and tested on Ubuntu 20.04. Please modify the steps according to the Operating System in use.

## Hardware

[STM32F746G-DISCO board (Cortex-M7)](https://www.st.com/en/evaluation-tools/32f746gdiscovery.html)
\
[STM32F4DIS-CAM Camera module](https://www.element14.com/community/docs/DOC-67585?ICID=knode-STM32F4-cameramore)

## The model

The model used in this project was produced using the same techniques as decribed in [this paper](https://arxiv.org/pdf/2010.11267.pdf). It is included as .cc file but can be converted into a .tflite file using this command:

`$ grep -E "(0x[0-9a-f]{2}(,|)){1,16}" image_recognition_model.cc | xxd -r -p > output.tflite`.

## Building the image recognition application

These instructions are tested on Ubuntu 20.04 with the following toolchains:\
- [Arm GCC toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) version 10.2.1.
- [Arm Compiler armclang](https://developer.arm.com/tools-and-software/embedded/arm-compiler/downloads/version-6) version 6.16.

Instructions on how to install the Arm GCC toolchain are provided below.

### Prerequisites

#### Mbed CLI 2
Install Mbed CLI 2:

`$ pip install mbed-tools`

See [Mbed-tools documentation](https://os.mbed.com/docs/mbed-os/v6.12/build-tools/install-or-upgrade.html) if any problems arise. During development of this project, mbed-tools version 7.27.0 and Python 3.8.5 was used.

#### Arm GCC toolchain
Install the Arm GCC toolchain. The latest version can be found [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads). To install it, download the Linux x86_64 Tarball and extract it with

`$ tar -xvjf gcc-arm-none-eabi-<version-num>-x86_64-linux.tar.bz2 -C /<user path for gnu tools>/`

Make sure it has been added to PATH,

`$ echo "$PATH"`

should include ` /<user path for gnu tools>/gcc-arm-none-eabi-<version-num>/bin/`. If it does not, do

`$ gedit ~/.bashrc`

and add:

`export PATH="$PATH:/<user path for gnu tools>/gcc-arm-none-eabi-<version-num>/bin/"`.

#### CMake
Cmake version 3.21.0, or later. Can be downloaded with

`$ pip install cmake`,

or following the instructions [here](https://cmake.org/install/).

#### Ninja
Ninja 1.0, or later. Can be downloaded with

`$ apt-get install ninja-build` [Note that sudo might be needed]
### Compiling and flashing

All commands should be executed in the project root folder.

1. Start by running\
`$ ./setup.sh`,\
which downloads the necessary files from the tflite-micro repository, and copies them into a new folder; `tensorflow/`. The setup script will also download CIFAR-10-binary files which are used for testing. Finally, the script also downloads some necessary drivers for the LCD and camera.
2. To compile, run:\
`$ mbed-tools compile -m DISCO_F746NG -t <TOOLCHAIN>`\
`-t <TOOLCHAIN>` specifies the toolchain used to compile, can be either `GCC_ARM` or `ARM`. Mbed-tools allows one to choose different profiles that control the compiler arguments like optimization level and debug info to name a few. To compile with a build profile, add the `--profile` flag with either debug, develop, or release. If the flag is not manually set, develop will be used as profile.
3. This will produce a file named `mbed-tflm-image-recognition.bin` in `cmake_build/DISCO_F746NG/<PROFILE>/<TOOLCHAIN>`. To flash it to the board, copy the file to the volume mounted as a USB drive. For instance:\
`$ cp cmake_build/DISCO_F746NG/<PROFILE>/<TOOLCHAIN>/mbed-tflm-image-recognition.bin /media/<USER>/<BOARD_NAME>/`\
If the volume complains about being full, disconnect and reconnect the board and flash to it again.

#### CMSIS-NN Performance analysis
To see the performance benefits of CMSIS-NN it might be interesting to compare the program with and without the CMSIS kernels. To build and compile with CMSIS-NN, open `setup.sh` and set the variable `CMSIS` to true. Setting it to false will use the reference kernels instead.

#### Compiler configuration considerations
A few aspects of the compiler options that influence the performance are mentioned below.
##### Optimization level
The selected optimization level affects the number of cycles consumed. Depending on which profile and/or compiler you use, the optimzation level will be different. In general, the release profile is the fastest of the profiles, but not necessarily the fastest possible optimization level. You can see which optimization flags are set in different build profiles and compliers in `mbed-os/tools/profiles/<PROFILE>.json`.

##### NDEBUG
The debug and develop profiles does not use the NDEBUG macro. This will define a macro called NDEBUG, which disables debugging and error information to reduce code size. When using CMSIS-NN, the NDEBUG macro has no effect, which can lead to misleading conclusions if analyzed improperly. Therefore, it is recomended to use the release profile when comparing CMSIS-NN to the reference implementations (non-CMSIS-NN).

### Testing performance

It is possible to test the accuracy of the model, on 50 images at a time. This will also produce a layer-by-layer profling where it is possible to see the amount cycles spent in each layer of the network.

1. To run a test, execute\
`$ ./test_performance.sh <SEED> <TOOLCHAIN>`,\
where the `<SEED>` is any integer >= 0, and `<TOOLCHAIN>` is `GCC_ARM` or `ARM`. Running the command with the same seed will test the same set of images.

2. Flash the binary file onto the board as described in step 3 in [Compiling and flashing](#compiling-and-flashing).

3. To read the results, a serial terminal is needed, with the baud rate set to 9600. Running\
`$ mbed-tools sterm --baudrate 9600`\
will work, but any serial terminal is fine. Running one test after another has finished should work without any issues.

### Resetting the project

If you wish to reset the project completely, run

`$ ./clean.sh`.

This will delete all downloaded files and the build folder.

If you only want to restart the build process, run

`$ rm -rf cmake_build`

and then repeat the build instructions. Running the setup script will automatically run the clean script.
