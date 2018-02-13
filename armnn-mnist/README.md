# Arm NN MNIST
Use Arm NN to deploy a Tensorflow MNIST network on an Arm Cortex-A CPU or Arm Mali GPU.

## Getting started

This is the example code used in Arm's [Deploying a TensorFlow MNIST model on Arm NN](https://developer.arm.com/technologies/machine-learning-on-arm/developer-material/how-to-guides/) tutorial - a more detailed description can be found there.

## Dependencies

You will need the Arm NN SDK, available from the [Arm NN SDK site](https://developer.arm.com/products/processors/machine-learning/arm-nn) in March 2018.

## Usage

Edit the Makefile and change ARMNN\_LIB and ARMNN\_INC to point to the library and include directories of your Arm NN installation:

    ARMNN_LIB = ${HOME}/devenv/build-x86_64/release/armnn
    ARMNN_INC = ${HOME}/devenv/armnn/include

You can then build and run the example with:

    make test

This loads a TensorFlow model from models/ and performs a single inference for one image from the MNIST data set in data/. The purpose is to demonstrate how to use Arm NN to execute TensorFlow models in a C++ application.

You can easily change the image used by modifying mnist.cpp:

    int testImageIndex = 0;

To run on an Arm Mali GPU, change CpuAcc to GpuAcc when creating the IGraphContext:

    armnn::IGraphContextPtr context = armnn::IGraphContext::Create(armnn::Compute::GpuAcc);
