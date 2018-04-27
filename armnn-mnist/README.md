# Arm NN MNIST
Use Arm NN to deploy a Tensorflow or Caffe MNIST network on an Arm Cortex-A CPU or Arm Mali GPU.

## Getting started

This is the example code used in Arm's [Deploying a TensorFlow MNIST model on Arm NN](https://developer.arm.com/technologies/machine-learning-on-arm/developer-material/how-to-guides/) tutorial - a more detailed description can be found there.

Two applications are included:

* mnist_tf.cpp, which reads from a TensorFlow model.
* mnist_caffe.cpp, which reads from a Caffe model.

## Dependencies

You will need the Arm NN SDK, available from the [Arm NN SDK site](https://developer.arm.com/products/processors/machine-learning/arm-nn). TensorFlow support will require version 18.02.1 or above.

## Usage

Edit the Makefile and change ARMNN\_LIB and ARMNN\_INC to point to the library and include directories of your Arm NN installation:

    ARMNN_LIB = ${HOME}/devenv/build-x86_64/release/armnn
    ARMNN_INC = ${HOME}/devenv/armnn/include

You can then build and run the examples with:

    make test

This builds and runs mnist, which loads the TensorFlow model from models/ and performs a single inference for one image from the MNIST data set in data/. It also builds and runs mnist_caffe, which repeats this with the Caffe model.

The purpose of these examples is to demonstrate how to use Arm NN to load and execute TensorFlow or Caffe models in a C++ application.

You can change the image used by modifying this line in mnist.cpp and/or mnist_caffe.cpp:

    int testImageIndex = 0;

To run on an Arm Mali GPU, change CpuAcc to GpuAcc when creating the IRuntime:

    armnn::IRuntimePtr runtime = armnn::IRuntime::Create(armnn::Compute::GpuAcc);
