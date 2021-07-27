## Training

To train a DNN with 3 fully-connected layers with 128 neurons in each layer, run:

```
python train.py --model_architecture dnn --model_size_info 128 128 128
```
The command line argument *--model_size_info* is used to pass the neural network layer
dimensions such as number of layers, convolution filter size/stride as a list to models.py,
which builds the TensorFlow graph based on the provided model architecture
and layer dimensions. For more info on *model_size_info* for each network architecture see
[models.py](models.py).

The training commands with all the hyperparameters to reproduce the models shown in the
[paper](https://arxiv.org/pdf/1711.07128.pdf) are given [here](train_commands.txt).

## Testing
To run inference on the trained model from a checkpoint and get accuracy on validation and test sets, run:
```
python test.py --model_architecture dnn --model_size_info 128 128 128 --checkpoint <checkpoint_path>
```
The parameters used here should match those used in the Training step.

## Optimization

We introduce a new *optional* step to optimize the trained keyword spotting model for deployment.

Here we use TensorFlow's [weight clustering API](https://www.tensorflow.org/model_optimization/guide/clustering) to reduce the compressed model size and optimize inference on supported hardware. 32 weight clusters and kmeans++ cluster intialization method are used as the clustering hyperparameters.

To optimize your trained model (e.g. a DNN), a trained model checkpoint is needed to run clustering and fine-tuning on.
You can use the pre-trained checkpoints provided, or train your own model and use the resulting checkpoint.

To apply the optimization and fine-tuning, run the following command:
```
python optimize.py --model_architecture dnn --model_size_info 128 128 128 --checkpoint <checkpoint_path>
```
The parameters used here should match those used in the Training step, except for the number of training steps.
The number of training steps is reduced since the optimization step only requires fine-tuning.

This will generate a clustered model checkpoint that can be used in the quantization step to generate a quantized and clustered TFLite model.

## Quantization and TFLite Conversion

As part of the update we now use TensorFlow's
[post training quantization](https://www.tensorflow.org/lite/performance/post_training_quantization) to
make quantization of the trained models super simple.

To quantize your trained model (e.g. a DNN) run:
```
python convert.py --model_architecture dnn --model_size_info 128 128 128 --checkpoint <checkpoint_path> [--inference_type int8]
```
The parameters used here should match those used in the Training step.

The inference_type parameter is *optional* and to be used if a fully quantized model with inputs and outputs of type int8 is needed. It defaults to fp32.

This step will produce a quantized TFLite file *dnn_quantized.tflite*.
You can test the accuracy of this quantized model on the test set by running:
```
python test_tflite.py --tflite_path dnn_quantized.tflite
```
The parameters used here should match those used in the Training step.

`convert.py` uses post-training quantization to generate a quantized model by default. If you wish to convert to a floating point TFLite model, use the command below:

```
python convert.py --model_architecture dnn --model_size_info 128 128 128 --checkpoint <checkpoint_path> --no-quantize
```

This will produce a floating point TFLite file *dnn.tflite*. You can test the accuracy of this floating point model using `test_tflite.py` as above.

## Pre-trained models

Trained floating point model checkpoints and trained quantized models (.tflite files)
for different neural network architectures such as DNN,
CNN, and DS-CNN shown in
this [arXiv paper](https://arxiv.org/pdf/1711.07128.pdf) are available in the
[Pretrained_models](../Pretrained_models) folder.

Accuracy of these models matches or surpasses that of the .pb files in the original repository.
