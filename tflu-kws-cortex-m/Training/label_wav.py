# Copyright © 2021 Arm Ltd. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from data import load_wav_file, calculate_mfcc
from test_tflite import tflite_inference

import tensorflow as tf
import argparse


def load_labels(filename):
    """Read in labels, one label per line."""
    f = open(filename, "r")
    return f.read().splitlines()


def main():
    window_size_samples = int(FLAGS.sample_rate * FLAGS.window_size_ms / 1000)
    window_stride_samples = int(FLAGS.sample_rate * FLAGS.window_stride_ms / 1000)
    decoded, sample = load_wav_file(FLAGS.wav, FLAGS.sample_rate)
    x = calculate_mfcc(decoded, sample, window_size_samples, window_stride_samples, FLAGS.dct_coefficient_count)
    x = tf.reshape(x, [1, -1])
    predictions = tflite_inference(x, FLAGS.tflite_path)

    # Sort to show labels in order of confidence
    top_k = predictions[0].argsort()[-1:][::-1]
    for node_id in top_k:
        human_string = load_labels(FLAGS.labels)[int(node_id)]
        score = predictions[0,node_id]
        print(f'model predicted: {human_string} with score {score:.5f}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--wav', type=str, default='', help='Audio file to be identified.')
    parser.add_argument(
        '--labels', type=str, default='', help='Path to file containing labels.')
    parser.add_argument(
        '--sample_rate',
        type=int,
        default=16000,
        help='Expected sample rate of the wavs', )
    parser.add_argument(
        '--window_size_ms',
        type=float,
        default=30.0,
        help='How long each spectrogram timeslice is', )
    parser.add_argument(
        '--window_stride_ms',
        type=float,
        default=10.0,
        help='How long each spectrogram timeslice is', )
    parser.add_argument(
        '--dct_coefficient_count',
        type=int,
        default=40,
        help='How many bins to use for the MFCC fingerprint', )
    parser.add_argument(
        '--tflite_path',
        type=str,
        default='',
        help='Path to TFLite file to use for testing.')
    FLAGS, unparsed = parser.parse_known_args()
    main()