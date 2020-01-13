import tensorflow as tf

import argparse
import io
import time
import cv2
import numpy as np
from timeit import default_timer as timer

# Load TFLite model and allocate tensors.
interpreter = tf.lite.Interpreter(model_path="./fire_detection.tflite")
interpreter.allocate_tensors()

# Get input and output tensors.
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
_,height,width,_ = input_details[0]['shape']
floating_model = False
if input_details[0]['dtype'] == np.float32:
    floating_model = True

parser = argparse.ArgumentParser(
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument(
      '--image', help='File path of image file', required=True)
args = parser.parse_args()

image = cv2.imread(args.image)
image = cv2.resize(image, (width, height))
image = np.expand_dims(image, axis=0)
if floating_model:
    image = np.array(image, dtype=np.float32) / 255.0
print(image.shape)

# Test model on image.
interpreter.set_tensor(input_details[0]['index'], image)
start = timer()
interpreter.invoke()
end = timer()
print('Elapsed time is ', (end-start)*1000, 'ms')

# The function `get_tensor()` returns a copy of the tensor data.
# Use `tensor()` in order to get a pointer to the tensor.
output_data = interpreter.get_tensor(output_details[0]['index'])
print(output_data)
j = np.argmax(output_data)
if j == 0:
    print("Non-Fire")
else:
    print("Fire")
