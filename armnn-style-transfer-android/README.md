# Style Transfer with Arm NN SDK

## What is neural style transfer?
Neural style transfer is a technique that takes two images, a content image and a style image (such as an artwork by a famous painter), then copies the texture, color etc. of the style image and applies them to the content image. 

                     
## How does neural style transfer work?
Neural style transfer uses a pretrained convolution neural network. Given your content image and your style image, we will generate a new image that blends the previous two. We start off with a simple white noise image (We could also start off with the content image or style image for optimization efficiency), then process the content, style and generated images through the pretrained neural network, and calculate loss functions at different layers. 

There are three types of loss functions: content loss function, style loss function and total loss function. Content cost function makes sure the content present in the content image is captured in the generated image. In a multiple layer CNN network, lower layers are more focused on individual pixel values, and higher layers capture information about content, thus we use the top-most CNN layer to define the content loss function in our illustration.

Style loss function makes sure the correlation of activations in all the layers are similar between the style image and the generated image. 

Total loss function is the weighted sum of the content and style loss functions for the generated image. The weights are user defined hyperparameters that control the amount of content and style injected to the generated image. Once the loss is calculated, it can be minimized using backpropagation which in turn optimizes the randomly generated image into a piece of art. 

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


## Style transfer code 
Import Live Style project into Android Studio. 

Our style transfer code is implemented in doStyleTransfer() function in TensorFlowImageStyleTransfer.java.

In this function, we first convert the image to data that the model can properly understand. 

TensorFlowHelper.convertBitmapToByteBuffer(image, intValues, imgData);

Then TF Lite interpreter runs the model assigned to it. This magic line runs a neural model without exposing the complexity of it.  

    tfLite.run(imgData, outputVector);

Convert the output data of the interpreter to an image. 

    outputImage = Bitmap.createBitmap(mInputImageWidth, mInputImageHeight, Bitmap.Config.ARGB_8888);


## Arm NN optimization 
Arm NN uses Arm Compute Library(ACL) to provide a set of optimized operators, e.g, convolution, pooling etc. that target ARM specific accelerators, like the DSP(NEON) or the Mali GPU. ACL also provides a GPU tuner tool called CITuner. CLTuner tunes a set of hardware knobs to fully utilize all the computational horsepower the GPU provides. 

Since Arm NN implements the Android NNAPI interface, developers only need to install the driver, and your Android application will seamlessly interact with the driver to exploit the aforementioned accelerations. 

This part of the code is illustrated in TensorFlowImageStyleTranfer()function in TensorFlowImageStyleTransfer.java. In the code, we check Android version and decide if NN API can be enabled on the device, then create a delegate if NN API can be supported.


    if (enableNNAPI && TensorFlowHelper.canNNAPIEnabled()){
        delegate = new NnApiDelegate();
        this.tfLiteOptions.addDelegate(delegate);
        this.tfLite = new Interpreter(TensorFlowHelper.loadModelFile(context, mModelFile), tfLiteOptions);
    } else {    
       this.tfLite = new Interpreter(TensorFlowHelper.loadModelFile(context, mModelFile));
    }

Since Arm NN implements the Android NNAPI interface, once developers have the driver installed, your Android application will seamlessly interact with the underlying APIs to exploit the aforementioned accelerators. 

Toggle NN API checkbox to experience the performance enhancement NN API provides.

The NN driver is not bundled with Android releases, instead it is shipped by OEMs like Samsung, HiSilicon, MTK. E.g, All Samsung devices with Android O MR1 or later firmware releases have Arm NN driver pre-installed. However, if your Android device doesn’t have NN driver pre-installed or you want to build your own Arm NN driver and manually install the driver. 
 
## Style transfer models
In our Android application, we used pre-trained models from https://github.com/misgod/fast-neural-style-keras. We made few tweaks to the model architecture to make it fully compatible with Arm NN operators. The tweaks we made include:

•	Replace all reflection padding with same padding.

•	Replace all instance normalization layers with batch normalization layers.

•	In un-pooling layers, use bilinear resize operation instead of nearest neighbor resize operation. 

•	In the first Conv2D layer, use valid padding instead of same padding.

If you want to train your own model, or a model with a different style, you can follow the training steps detailed in this https://arxiv.org/abs/1603.08155 or other open source projects like https://github.com/tensorflow/magenta/tree/master/magenta/models/arbitrary_image_stylization#train-a-model-on-a-large-dataset-with-data-augmentation-to-run-on-mobile for reference. Either way, you need to apply the 4 changes we mentioned above. 


