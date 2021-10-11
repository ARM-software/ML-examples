import pyarmnn as ann
import numpy as np
import argparse
import cv2
from timeit import default_timer as timer

print(f"Working with ARMNN {ann.ARMNN_VERSION}")

#Load an image 
parser = argparse.ArgumentParser(
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument(
      '--image', help='File path of image file', required=True)
args = parser.parse_args()

image = cv2.imread(args.image)
image = cv2.resize(image, (128, 128))
image = np.array(image, dtype=np.float32) / 255.0
print(image.shape)

# ONNX, Caffe and TF parsers also exist.
parser = ann.ITfLiteParser()  
network = parser.CreateNetworkFromBinaryFile('./fire_detection.tflite')

graph_id = 0
input_names = parser.GetSubgraphInputTensorNames(graph_id)
input_binding_info = parser.GetNetworkInputBindingInfo(graph_id, input_names[0])
input_tensor_id = input_binding_info[0]
input_tensor_info = input_binding_info[1]
print(f"""
tensor id: {input_tensor_id}, 
tensor info: {input_tensor_info}
""")

# Create a runtime object that will perform inference.
options = ann.CreationOptions()
runtime = ann.IRuntime(options)

# Backend choices earlier in the list have higher preference.
preferredBackends = [ann.BackendId('CpuAcc'), ann.BackendId('CpuRef')]
opt_network, messages = ann.Optimize(network, preferredBackends, runtime.GetDeviceSpec(), ann.OptimizerOptions())

# Load the optimized network into the runtime.
net_id, _ = runtime.LoadNetwork(opt_network)
print(f"Loaded network, id={net_id}")
# Create an inputTensor for inference.
input_tensors = ann.make_input_tensors([input_binding_info], [image])

# Get output binding information for an output layer by using the layer name.
output_names = parser.GetSubgraphOutputTensorNames(graph_id)
output_binding_info = parser.GetNetworkOutputBindingInfo(0, output_names[0])
output_tensors = ann.make_output_tensors([output_binding_info])

start = timer()
runtime.EnqueueWorkload(0, input_tensors, output_tensors)
end = timer()
print('Elapsed time is ', (end - start) * 1000, 'ms')

output = ann.workload_tensors_to_ndarray(output_tensors[0][1])
print(output)
j = np.argmax(output)
if j == 0:
    print("Non-Fire")
else:
    print("Fire")

