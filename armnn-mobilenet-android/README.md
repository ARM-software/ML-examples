# Image Classification with Arm NN SDK

## Overview
Image classification is one of the most popular deep learning problems. Because of the vast number of datasets available, neural networks have been able to excel in this field.

There have also been many advancements for edge devices. One key model is the MobileNet architecture. This architecture uses depth-wise separable convolution layers to create a lightweight image classification model that performs efficiently on mobile and embedded devices.

## Arm NN SDK
In our application, we will use Arm NN under the hood for performance enhancement on Arm hardware architecture. Arm NN SDK is a set of open-source Linux software tools that enables machine learning workloads on power-efficient devices. This inference engine provides a bridge between existing neural network frameworks and Arm Cortex-A CPUs, Arm Mali GPUs and DSPs. 

## Android NN API & TFLite delegate
Android Neural Networks API(NNAPI) is an Android C API designed for running computationally intensive operations for machine learning on mobile devices. It supports various hardware accelerations. NNAPI uses TensorFlow as a core technology. If you build your mobile app with TensorFlow, your app will get the benefits of hardware acceleration through the NNAPI. It basically abstracts the hardware layer for ML inference. Arm NN works with Android NNAPI to target Arm processors, enabling exponential performance boosts.

A TensorFlow Lite delegate is a way to delegate part or all of graph execution to another executor. Running computation-intensive NN models on mobile devices is resource demanding on mobile CPUs, thus, devices that have hardware accelerators, provide better performance and higher energy efficiency through NN API. 

TensorFlow Lite uses an NNAPI delegate to access NN API. The open source code can be found at https://github.com/tensorflow/tensorflow/tree/r2.0/tensorflow/lite/delegates/nnapi.

## Dive into the Android Code
What do you need?

•	An Android device running Android 9 or 10 

•	An Android device that supports Camera 2 SDK. You can follow this online guide to confirm

•	Android Studio, including v24 or higher of the SDK build tools


## Image Classification code 
Import MobileNet Test project into Android Studio. 

Our Image classification is implemented in recognize() function withing the Recognizer.java file. 

In this function, we first run the supplied image though the Tensorflow ImageProcessor, and store as a TensorImage object,

which converts the image into data that the model can properly understand. 

    TensorFlowHelper.convertBitmapToByteBuffer(image, intValues, imgData);

Then TF Lite interpreter runs the model assigned to it. This magic line runs a neural model without exposing the complexity of it.

    tfLiteInterpreter.run(inputBuffer, outputBuffer);

From the outputBuffer we get the highest probable classification, it's corresponding label and it's probability. 

    int itemID = getStongestPosition(outputBuffer);
    String label = getLabel(itemID);
    byte probability = outputBuffer[0][itemID] ;


## Arm NN optimization 
Arm NN uses Arm Compute Library(ACL) to provide a set of optimized operators, e.g, convolution, pooling etc. that target ARM specific accelerators, like the DSP(NEON) or the Mali GPU. ACL also provides a GPU tuner tool called CLTuner. CLTuner tunes a set of hardware knobs to fully utilize all the computational horsepower the GPU provides. 

This part of the code is illustrated in prepareInterpretor() function in Recognizer.java. In the code, we check Android version and decide if NN API can be enabled on the device, then create a delegate if NN API can be supported.

    if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && this.useNNAPI) {
        nnApiDelegate = new NnApiDelegate();
        options.addDelegate(nnApiDelegate);
    }

Toggle ArmNN checkbox to experience the performance enhancement the NN API provides.

Since Arm NN implements the Android NNAPI interface, once developers have a device with the driver installed, your Android application will seamlessly interact with the underlying APIs to exploit the aforementioned accelerators. 

The NN driver is not bundled with any Android releases, instead it is shipped by OEMs like Samsung, HiSilicon, MTK. E.g, All Samsung devices with Android O MR1 or later firmware releases have the Arm NN driver pre-installed.
 
## Image classification model
In our Android application, we used a pre-trained model from https://github.com/ARM-software/ML-zoo. 



