## Training

To train a DNN with 3 fully-connected layers with 128 neurons in each layer, run:

```
python train.py --model_architecture dnn --model_size_info 128 128 128
```
The command line argument *--model_size_info* is used to pass the neural network layer
dimensions such as number of layers, convolution filter size/stride as a list to models.py,
which builds the TensorFlow graph based on the provided model architecture
and layer dimensions.
For more info on *model_size_info* for each network architecture see
[models.py](models.py).
The training commands with all the hyperparameters to reproduce the models shown in the
[paper](https://arxiv.org/pdf/1711.07128.pdf) are given [here](train_commands.txt).

## Testing
To run inference on the trained model from a checkpoint and get accuracy on validation and test sets, run:
```
python test.py --model_architecture dnn --model_size_info 128 128 128 --checkpoint
<checkpoint_path>
```

## Quantization

As part of the update we now use TensorFlow's
[post training quantization](https://www.tensorflow.org/lite/performance/post_training_quantization) to
make quantization of the trained models super simple.

To quantize your trained model (e.g. a DNN) run:
```
python quantize.py --model_architecture dnn --model_size_info 128 128 128 --checkpoint <checkpoint_path>
```

This will produce a quantized tflite file *dnn.quantized.tflite*.
You can test the accuracy of this quantized model on the test set by running:
```
python test_tflite.py --tflite_path dnn_quantized.tflite
```

## Pretrained models

Trained FP32 model checkpoints and trained quantized models (.tflite files)
for different neural network architectures such as DNN,
CNN, and DS-CNN shown in
this [arXiv paper](https://arxiv.org/pdf/1711.07128.pdf) are available in the
[Pretrained_models](../Pretrained_models) folder.

Accuracy of these models matches or surpasses that of the .pb files in the original repository.
