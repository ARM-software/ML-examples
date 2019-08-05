# Arm NN Quantized MobileNet

Use Arm NN to deploy a TensorflowLite Quantized MobileNet V1 network on an Arm Cortex-A CPU or Arm Mali GPU.

## Getting started

This is the example code used in Armn's Deploying a Tensorflow Lite quantised MobileNet model on ArmNN (TODO: Add link)
tutorial - a more detailed description can be found there.

One application, along with utility code is included:
* mobilenetv1_quant_tflite.cpp, which parses and uses a TensorFlowLite mobilenet model to classify images.
* model_output_labels_loader.hpp, which includes utility code that can be used to load model output labels.


## Dependencies
You will need:
* Arm NN SDK, available from the [Arm NN SDK
site](https://developer.arm.com/products/processors/machine-learning/arm-nn).
* Boost library, which should be part of the Arm NN SDK installation process.


## Building
Specify these environment variables before building:
```sh
CXX=<compiler to use>
ARMNN_ROOT=<path to ArmNN source folder>
ARMNN_BUILD=<path to ArmNN build folder>
BOOST_ROOT=<path to Boost source folder>
```

You can then build the example with:
```sh
make
```

This builds the mobilenetv1_quant_tflite program.

## Usage

1. Push the ArmNN library and the program to your device. This includes: libarmnn.so, libarmnnTfLiteParser.so and
mobilenetv1_quant_tflite

2. Run the program on your device.

The usage of the program is as follows:
```sh
./mobilenetv1_quant_tflite -m <ModelPath> -d <DataPath> -p <ModelOutputLabels> [-c <ComputeDevices>]

  -h [ --help ]                         Display help messages
  -m [ --model-path ] arg               Path to armnn format model file
  -d [ --data-dir ] arg                 Path to directory containing the
                                        ImageNet test data
  -p [ --model-output-labels ] arg      Path to model output labels file.
  -c [ --compute ] arg (=[CpuAcc CpuRef ])
                                        Which device to run layers on by
                                        default. Possible choices: CpuAcc,
                                        GpuAcc, CpuRef
```
For example, to run mobilenetv1_quant_tflite on mali GPU, with Cpu as fallback:
```sh
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./mobilenetv1_quant_tflite -m ./models/mobilenetv1_1.0_quant_224.tflite
-d ./data/Dog.JPEG -p ./models/labels.txt -c GpuAcc CpuAcc
```

And you should expect outputs similar to below:
```sh
ArmNN v20190500

Running network...
Prediction: maltese puppy,
Ran successfully!
```
