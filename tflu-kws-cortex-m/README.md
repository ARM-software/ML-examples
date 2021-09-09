# NEW Keyword spotting for Microcontrollers

This repository is an update to the original
[Keyword spotting for Microcontrollers](https://github.com/ARM-software/ML-KWS-for-MCU) repository.
It consists of updated versions of the TensorFlow models and training scripts used
in the paper:
[Hello Edge: Keyword spotting on Microcontrollers](https://arxiv.org/pdf/1711.07128.pdf).
The original scripts are adapted from
[Tensorflow examples](https://github.com/tensorflow/tensorflow/tree/master/tensorflow/examples/speech_commands).

The main update to the training scripts is a move from TensorFlow 1 to TensorFlow 2 APIs.

This repository also includes updates to the deployment code used for deploying trained models to Arm Cortex-M
development boards. In this case the main update is the use of
[TensorFlow Lite for Microcontrollers](https://www.tensorflow.org/lite/microcontrollers) to make deployment easier.

## Training and Quantization
Code and instructions on how to train the keyword spotting models from the [paper](https://arxiv.org/pdf/1711.07128.pdf)
is provided [here](Training). Currently only the DNN, CNN, DS-CNN and Basic_LSTM architectures from the paper are supported.

Code for doing quantization and producing models ready for deployment is also found in the same directory.

## Optimization
New *optional* code for performing clustering of the keyword spotting models has also been added [here](Training).
After clustering has been performed the models produced will have their compressed model size reduced, and inference times
on compatible hardware will be optimized.

## Deployment on Microcontrollers
The example code for deploying a trained and quantized DNN or DS-CNN model on a Cortex-M development board is also
provided [here](kws_cortex_m).
