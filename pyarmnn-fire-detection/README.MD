# Accelerating ML inference on Raspberry Pi with PyArmNN

## Overview 

A neural network, trained to recognize images that include a fire or flames, can make fire-detection systems more reliable and cost-effective. This tutorial shows how to use the newly released Python APIs for Arm NN inference engine, to classify images as “Fire” versus “Non-Fire.”

## What is Arm NN and PyArmNN? 
Arm NN is an inference engine for CPUs, GPUs and NPUs. It executes ML model on-device in order to make predictions based on input data. Arm NN enables efficient translation of existing neural network frameworks, such as TensorFlow Lite, TensorFlow, ONNX and Caffe, allowing them to run efficiently, without modification, across Arm Cortex-A CPUs, Arm Mali GPUs and Arm Ethos NPUs.

PyArmNN is a newly developed python extension for Arm NN SDK. In this tutorial, we are going to use PyArmNN APIs to run a fire detection image classification model fire_detection.tflite and compare the inference performance with TensorFlow Lite on a Raspberry Pi. 

Arm NN provides TFLite parser armnnTfLiteParser, which is a library for loading neural networks defined by TensorFlow Lite FlatBuffers files into the Arm NN runtime. We are going to use the TFLite parser to parse our fire detection model for “Fire” vs. “Non-Fire” image classification. 

## What do we need?

We will use a Raspberry Pi 3 / 4 device. Pi device is powered by an Arm CPU with Neon architecture. Neon is an optimization architecture extension for Arm processors. It is designed specifically for faster video processing, image processing, speech recognition and machine learning. This hardware optimization supports Single Instruction Multiple Data (SIMD), where multiple processing elements in the pipeline perform operations on multiple data points simultaneously. Arm NN presents you the APIs to harness the power of Neon backend. 

•	A Raspberry Pi 3/4. I am testing with a Raspberry Pi 4 with Raspbian 10 OS. 

•	Before you proceed with the project setup, you will need to check out and build Arm NN version 19.11 or newer for your Raspberry Pi. Instruction is at https://developer.arm.com/solutions/machine-learning-on-arm/developer-material/how-to-guides/configuring-the-arm-nn-sdk-build-environment-for-tensorflow-lite. Please enable Neon support when you build Arm NN. 

•	PyArmNN package. Install the pyarmnn-19.11.0.tar.gz in this directory. 

     pip3 install pyarmnn-19.11.0.tar.gz

•	fire_detection.tflite, generated from this tutorial https://www.pyimagesearch.com/2019/11/18/fire-and-smoke-detection-with-keras-and-deep-learning/ and converted to a TensorFlow Lite model. For your convenience, we included the fire_detection.tflite model in this directory.

## Run ML Inference with PyArmNN
To run a ML model on device, our main steps are:

•	Import pyarmnn module

•	Load an input image

•	Create a parser and load the network

•	Choose backends, create runtime and optimize the model

•	Perform inference

•	Interpret and report the output

### Import pyarmnn module

Use the variables below to define the location of our model, image and label file. 
       
    import pyarmnn as ann
    import numpy as np
    import cv2

    print(f"Working with ARMNN {ann.ARMNN_VERSION}")


### Load and pre-process an input image

Load the image specified in the command line, resize it to the model input dimension. In our case, our model accepts 128x128 input images. The input image is wrapped in a const tensor and bound to the input tensor. 

Our model is a floating-point model. We must scale the input image values to a range of -1 to 1.

    parser = argparse.ArgumentParser(
       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
      '--image', help='File path of image file', required=True)
    args = parser.parse_args()

    #Load an image.
    image = cv2.imread(args.image)
    image = cv2.resize(image, (128, 128))
    image = np.array(image, dtype=np.float32) / 255.0
    print(image.size)

### Create a parser and load the network

The next step when working with Armn NN is to create a parser object that will be used to load the network file. Arm NN has parsers for a variety of model file types, including TFLite, ONNX, Caffe etc. Parsers handle creation of the underlying Arm NN graph so you don't need to construct your model graph by hand. 

In this example, we will create a TfLite parser to load our TensorFlow Lite model from the specified path:

    parser = ann.ITfLiteParser()
    network = parser.CreateNetworkFromBinaryFile('./fire_detection.tflite')

### Get Input Binding Info
Once created, the parser is used to extract the input information for the network.

We can extract all the input names by calling GetSubgraphInputTensorNames() and then use them get the input binding information. For this example, since our model only has one input layer, we use input_names[0] to obtain the input tensor, then use this string to retrieve the input binding info.

The input binding info contains all the essential information about the input, it is a tuple consisting of integer identifiers for bindable layers (inputs, outputs) and the tensor info (data type, quantization information, number of dimensions, total number of elements).

    graph_id = 0
    input_names = parser.GetSubgraphInputTensorNames(graph_id)
    input_binding_info = parser.GetNetworkInputBindingInfo(graph_id, input_names[0])
    input_tensor_id = input_binding_info[0]
    input_tensor_info = input_binding_info[1]
    print(f"""
     tensor id: {input_tensor_id},
     tensor info: {input_tensor_info}
    """)

### Choose backends, create runtime and optimize the model
Specify the backend list you can optimize the network.

    options = ann.CreationOptions()
    runtime = ann.IRuntime(options)
    preferredBackends = [ann.BackendId('CpuAcc'), ann.BackendId('CpuRef')]
    opt_network, messages = ann.Optimize(network, preferredBackends, runtime.GetDeviceSpec(), ann.OptimizerOptions())

### Load optimized network into the runtime

Load the optimized network in the runtime context. LoadNetwork() creates the backend-specific workloads for the layers. 

    net_id, _ = runtime.LoadNetwork(opt_network)
    print(f"Loaded network, id={net_id}")
    input_tensors = ann.make_input_tensors([input_binding_info], [image])

### Get output binding info and make output tensor
Similar to the input binding info, we can retrieve from the parser the output tensor names and get the binding information.
In our sample, it is considered that an image classification model has only one output, hence it's used only the first name from the list returned, it can easily be extended to multiple output looping on the output_names.

    output_names = parser.GetSubgraphOutputTensorNames(graph_id)
    output_binding_info = parser.GetNetworkOutputBindingInfo(0, output_names[0])
    output_tensors = ann.make_output_tensors([output_binding_info])

### Perform Inference
Performance Inference EnqueueWorkload() function of the runtime context executes the inference for the network loaded.

    runtime.EnqueueWorkload(0, input_tensors, output_tensors)
    output, output_tensor_info = ann.from_output_tensor(output_tensors[0][1])
    print(f"Output tensor info: {output_tensor_info}")

## Run the Python script from command line:

    $ python3 predict_pyarmnn.py --image ./images/opencountry_land663.jpg 
    Working with ARMNN 20190800

    tensor id: 15616, 
    tensor info: TensorInfo{DataType: 1, IsQuantized: 0, QuantizationScale: 0.000000, QuantizationOffset: 0, NumDimensions: 4, NumElements: 49152}

    (128, 128, 3)
    Output tensor info: TensorInfo{DataType: 1, IsQuantized: 0, QuantizationScale: 0.000000, QuantizationOffset: 0, NumDimensions: 2, NumElements: 2}
    [0.9967675, 0.00323252]
    Non-Fire

In our example, class 0’s possibility is 0.9967675, vs. class 1’s possibility is 0.00323252. Fire is not detected in the image. 

## PyArmNN vs. TensorFlow Lite Performance Comparison

As the next step, we compare PyArmNN and TensorFlow Lite Python APIs performance on a Raspberry Pi. 

TensorFlow Lite uses an interpreter to perform inferences. The interpreter uses a static graph ordering and a custom(less-dynamic) memory allocator. To understand how to load and run a model with Python API, please refer to TensorFlow Lite documentation.

For performance comparison, inference is carried out with our fire detection model. In our example, we only run inference once. We can also run the model multiple times and take the average inferencing time. 

    $ python3 predict_tflite.py --image ./images/746.jpg 
    2020-01-01 11:32:33.609188: E 
    Elapsed time is  38.02500700112432 ms
    [[9.9076563e-04 9.9900925e-01]]
    Fire

We extend predict_pyarmmnn.py with the same code for inference benchmarking. 
      
    start = timer()
    runtime.EnqueueWorkload(0, input_tensors, output_tensors)
    end = timer()
    print('Elapsed time is ', (end - start) * 1000, 'ms')

Run the Python script again:

    $ python3 predict_pyarmnn.py --image ./images/746.jpg 
    Working with ARMNN 20190800
    (128, 128, 3)

    tensor id: 15616, 
     tensor info: TensorInfo{DataType: 1, IsQuantized: 0, QuantizationScale: 0.000000, QuantizationOffset: 0, NumDimensions: 4, NumElements: 49152}

    Loaded network, id=0
    Elapsed time is  21.224445023108274 ms
    Output tensor info: TensorInfo{DataType: 1, IsQuantized: 0, QuantizationScale: 0.000000, QuantizationOffset: 0, NumDimensions: 2, NumElements: 2}
    [0.0009907632, 0.99900925]
    Fire
From the result, you can observe an inference performance enhancement by using Arm NN Neon optimized computational backend.   

## Next Steps and Helpful Resources 
This tutorial shows how to use the Arm NN Python APIs to classify images as “Fire” versus “Non-Fire.” You also can use it as a starting point to handle other types of neural networks. 

To learn more about Arm NN, check out the following resources: 

•	Arm Software Developer Kit (SDK)

•	Project Trillium Framework


