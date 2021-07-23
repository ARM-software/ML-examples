## Table of Contents

- [Table of Contents](#table-of-contents)
- [Introduction](#introduction)
- [Hardware](#hardware)
- [Building application](#building-application)
  - [Prerequisites](#prerequisites)
  - [Compiling](#compiling)
- [Deploying the application on the board](#deploying-the-application-on-the-board)
- [Using custom NN model](#using-custom-nn-model)

## Introduction

The first step in deploying the trained keyword spotting models on microcontrollers is quantization, which is described [here](../Training/README.md). 

This directory consists of example codes and steps for running a quantized DNN or DS_CNN model on any Cortex-M board using mbed-cli, TensorFlow Lite Micro library and CMSIS-NN.

## Hardware

One of the following boards:

* [STM32F746NG-DISCO board (Cortex-M7)](https://os.mbed.com/platforms/ST-Discovery-F746NG/)
* [STM32H747I-DISCO board (Cortex-M7)](https://os.mbed.com/platforms/ST-Discovery-H747I/)
* [FRDM-K64F board (Cortex-M4)](https://os.mbed.com/platforms/FRDM-K64F/)

## Building application

### Prerequisites

Install mbed-cli:

```sh
$ pip install mbed-cli
```

Install the arm-none-eabi-toolchain:
```sh
$ apt install gcc-arm-none-eabi
```

Recommended version is 9 and up. Other versions can be downloaded from [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
for Windows, Mac OS X and Linux. We have tested on 9.3.1

### Compiling

This project is designed to be integrated into the TensorFlow build environment. As such, you must clone the
TensorFlow repository and copy the *["kws_cortex_m"](../kws_cortex_m)* **sub-folder** from this repository
into the TensorFlow Lite Micro examples sub-folder *"tensorflow/lite/micro/examples"* of the TensorFlow repository:

```sh
$ git clone https://github.com/tensorflow/tensorflow.git
$ cp -r ./kws_cortex_m tensorflow/tensorflow/lite/micro/examples/
```

In order to generate the mbed project contained in this folder change directory to the TensorFlow repository you just cloned:
```sh
$ cd tensorflow
``` 

and from there run the following commands. Checkout of earlier version of Tensorflow is necessary due to deprecation of support for 'TAGS'.

```sh
$ git checkout 72c19e8880
$ make -f tensorflow/lite/micro/tools/make/Makefile TAGS=<tags> generate_kws_cortex_m_mbed_project
```
Where TAGS specify the application to build (Simple_KWS_Test, Realtime_KWS_Test) and the model to use (DNN, DS_CNN).

For example:

```sh
$ make -f tensorflow/lite/micro/tools/make/Makefile TAGS="DS_CNN Simple_KWS_Test" generate_kws_cortex_m_mbed_project
```

Navigate to the generated folder:

```sh
$ cd tensorflow/lite/micro/tools/make/gen/linux_x86_64/prj/kws_cortex_m/mbed/
```

Change the version of mbed to a recent version if the generated one points to an old instance. For example, `mbed-os.lib`:
```
https://github.com/ARMmbed/mbed-os/#60cbab381dd2d5d860407b1b789741275012a075
```

To load the dependencies required, run:

```sh
$ mbed config root .
$ mbed deploy
```

In order to compile the "Simple_KWS_Test" application, run:

```sh
$ mbed compile -m <TARGET> -t GCC_ARM
```

for example:
```sh
$ mbed compile -m K64F -t GCC_ARM
```

or,

```sh
$ mbed compile -m DISCO_H747I -t GCC_ARM
```

or,

```sh
$ mbed compile -m DISCO_F746NG -t GCC_ARM
```


The "Realtime_KWS_Test" application uses the audio and lcd provided on the board and is tailored for the STM32F746NG-DISCO board.

In order to compile the application, run:

```sh
$ mbed add http://os.mbed.com/teams/ST/code/BSP_DISCO_F746NG/
$ mbed deploy
$ mbed compile -m DISCO_DISCO_F746NG -t GCC_ARM
```

## Deploying the application on the board

Connect the board to the build machine via USB. The board will enumerate as a USB mass storage device.

The binary you built will be in ```./BUILD/\<TARGET>/\<TOOLCHAIN>/``` and will be named ```mbed.bin```. Copy this file to the USB mass storage device and the LED on the board will flash to indicate the board is being reflashed.

Once the flashing LED has stopped, you may need to manually reset the board (depending on the target) by pressing its reset button, so do this regardless to make sure. You can then connect to the boards USB serial port using a baud rate of 9600 and each time you reset the board it will run your application and print to the serial port.

## Using custom NN model

In order to use a custom NN model in this project, the developer needs to:
- Implement the Model class (that is an abstract class wrapping the underlying TensorFlow Lite Micro API and providing methods to perform common operations).

Start with creation of a sub-directory under NN directory and the model sources:
```sh
└── NN
    ├── BufAttributes.h
    ├── DNN
    │   └── ...
    ├── DS_CNN
    │   └── ...
    ├── HELLO_WORLD
    │   ├── HelloWorldModel.cc
    │   └── HelloWorldModel.h
    ├── Model.cc
    └── Model.h
```
Network models have different set of operators that must be registered with tflite::MicroMutableOpResolver object in the EnlistOperations method, to minimize application memory footprint, it is advised to register only operators used by the NN model.

Network models could require different size of activation buffer that is returned as tensor arena memory for TensorFlow Lite Micro framework by the GetTensorArena and GetActivationBufferSize methods.
Please see `DnnModel.hpp` and `DnnModel.cc` files as an example of the model base class extension.

- Implement the correct _InitModel of the KWS Class.

Start with creation of a sub-directory under KWS directory and the sources:
```sh
└── KWS
    ├── kws.cc
    ├── KWS_DNN
    │   └── ...
    ├── KWS_DS_CNN
    │   └── ...
    ├── KWS_HELLO_WORLD
    │   ├── kws_helloworld.cc
    └── kws.h
```
where in the `kws_helloworld.cc`:
```sh
bool KWS::_InitModel()
{
    this->model =  std::unique_ptr<HelloWorldModel>();
    if (this->model) {
        return this->model->Init();
    }
    printf("Failed to allocate memory for the model\r\n");
    return false;
}

```
- Modify the Makefile in order to enable the compilation with the new model tag:
```sh
ifneq ($(filter DNN,$(ALL_TAGS)),)
    MODEL = DNN
else ifneq ($(filter DS_CNN,$(ALL_TAGS)),)
    MODEL = DS_CNN
else ifneq ($(filter HELLO_WORLD,$(ALL_TAGS)),)
    MODEL = HELLO_WORLD
else
    $(error Error parsing the model to run)
endif
``` 

- Use the python script [tflite_to_tflu.py](tflite_to_tflu.py) for converting a TFLite model into a C source file loadable by TensorFlow Lite Micro. 
```sh
python tflite_to_tflu.py --tflite_path <path_to_quantized_tflite> --output_path <output_path>
```

Add the converted file into the `Generated` folder tree:

```sh
└── Generated
    ├── DNN
    │   └── ...
    ├── DS_CNN
    │   └── ...
    └── HELLO_WORLD
        └── helloworld_s_tflu.cc
```
