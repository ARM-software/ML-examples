# CMSIS-NN CIFAR10 Example

This example shows how to quantize a trained Caffe model to 8-bits and deploy it on Arm Cortex-M CPUs using [CMSIS-NN](https://github.com/ARM-software/CMSIS_5).

## Getting started
Code generation from a trained Caffe model is performed in the following steps: 
1. Network parsing from caffe prototxt 
2. Quantization to 8-bit weights and activations
3. Code generation using optimized NN Functions

*nn_quantizer.py:* Needs Caffe model definition (.prototxt) used for training/testing the model that consists of valid paths to datasets (lmdb) and trained model file (.caffemodel). It parses the network graph connectivity, quantize the caffemodel to 8-bit weights/activations layer-by-layer incrementally with minimal loss in accuracy on the test dataset. It dumps the network graph connectivity, quantization parameters into a pickle file.

*code_gen.py:* Gets the quantization parameters and network graph connectivity from previous step and generates the code consisting of NN function calls. Supported layers: convolution, innerproduct, pooling (max/average) and relu. It generates (a) weights.h (b) parameter.h: consisting of quantization ranges and (c) main.cpp: the network code.  

**Note:** Make sure caffe is installed and it's python path is added in $PYTHONPATH environment variable.

## Usage
1. Update the dataset path (lmdb) in .prototxt file and run *nn_quantizer.py* to parse and quantize the network. This step takes a while if run on CPU as it quantizes the network layer-by-layer while validating the accuracy on test dataset
```bash
python nn_quantizer.py --model models/cifar10_m4_train_test.prototxt \ 
  --weights models/cifar10_m4_iter_70000.caffemodel.h5 \
  --save models/cifar10_m4.pkl
```
**Note:** To enable GPU for quantization sweeps, use *--gpu* argument.

2. Generate code to run on Arm Cortex-M CPUs.
```bash
python code_gen.py --model models/cifar10_m4.pkl --out_dir code/m4
```

### Common Problems 
1. `ImportError: No module named caffe`
Add Caffe python installation path to $PYTHONPATH environment variable, e.g., `export PYTHONPATH="/home/ubuntu_user/caffe/python:$PYTHONPATH"`
2. `F0906 15:49:48.701362 11933 db_lmdb.hpp:15] Check failed: mdb_status == 0 (2 vs. 0) No such file or directory`
Make sure valid dataset (lmdb) is present in the model prototxt definition, as the dataset is required to find the quantization ranges for activations.

### Known Limitations 
1. Parser supports conv, pool, relu, fc layers only.
2. Quantizer supports only networks with feed-forward structures (e.g. conv-relu-pool-fc)  without branch-out/branch-in (as in inception/squeezeNet, etc.).
