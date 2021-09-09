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
"""Functions for testing trained keyword spotting models from checkpoint files."""

import argparse

import numpy as np
import tensorflow as tf

import data
import models


def test():
    """Calculate accuracy and confusion matrices on validation and test sets.

    Model is created and weights loaded from supplied command line arguments.
    """
    model_settings = models.prepare_model_settings(len(data.prepare_words_list(FLAGS.wanted_words.split(','))),
                                                   FLAGS.sample_rate, FLAGS.clip_duration_ms, FLAGS.window_size_ms,
                                                   FLAGS.window_stride_ms, FLAGS.dct_coefficient_count)

    model = models.create_model(model_settings, FLAGS.model_architecture, FLAGS.model_size_info, False)

    audio_processor = data.AudioProcessor(data_url=FLAGS.data_url,
                                          data_dir=FLAGS.data_dir,
                                          silence_percentage=FLAGS.silence_percentage,
                                          unknown_percentage=FLAGS.unknown_percentage,
                                          wanted_words=FLAGS.wanted_words.split(','),
                                          validation_percentage=FLAGS.validation_percentage,
                                          testing_percentage=FLAGS.testing_percentage,
                                          model_settings=model_settings)

    model.load_weights(FLAGS.checkpoint).expect_partial()

    # Evaluate on validation set.
    print("Running testing on validation set...")
    val_data = audio_processor.get_data(audio_processor.Modes.VALIDATION).batch(FLAGS.batch_size)
    expected_indices = np.concatenate([y for x, y in val_data])

    predictions = model.predict(val_data)
    predicted_indices = tf.argmax(predictions, axis=1)

    val_accuracy = calculate_accuracy(predicted_indices, expected_indices)
    confusion_matrix = tf.math.confusion_matrix(expected_indices, predicted_indices,
                                                num_classes=model_settings['label_count'])
    print(confusion_matrix.numpy())
    print(f'Validation accuracy = {val_accuracy * 100:.2f}%'
          f'(N={audio_processor.set_size(audio_processor.Modes.VALIDATION)})')

    # Evaluate on testing set.
    print("Running testing on test set...")
    test_data = audio_processor.get_data(audio_processor.Modes.TESTING).batch(FLAGS.batch_size)
    expected_indices = np.concatenate([y for x, y in test_data])

    predictions = model.predict(test_data)
    predicted_indices = tf.argmax(predictions, axis=1)

    test_accuracy = calculate_accuracy(predicted_indices, expected_indices)
    confusion_matrix = tf.math.confusion_matrix(expected_indices, predicted_indices,
                                                num_classes=model_settings['label_count'])
    print(confusion_matrix.numpy())
    print(f'Test accuracy = {test_accuracy * 100:.2f}%'
          f'(N={audio_processor.set_size(audio_processor.Modes.TESTING)})')


def calculate_accuracy(predicted_indices, expected_indices):
    """Calculates and returns accuracy.

    Args:
        predicted_indices: List of predicted integer indices.
        expected_indices: List of expected integer indices.

    Returns:
        Accuracy value between 0 and 1.
    """
    correct_prediction = tf.equal(predicted_indices, expected_indices)
    accuracy = tf.reduce_mean(tf.cast(correct_prediction, tf.float32))
    return accuracy


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--data_url',
        type=str,
        default='http://download.tensorflow.org/data/speech_commands_v0.02.tar.gz',
        help='Location of speech training data archive on the web.')
    parser.add_argument(
        '--data_dir',
        type=str,
        default='/tmp/speech_dataset/',
        help="""\
        Where to download the speech training data to.
        """)
    parser.add_argument(
        '--silence_percentage',
        type=float,
        default=10.0,
        help="""\
        How much of the training data should be silence.
        """)
    parser.add_argument(
        '--unknown_percentage',
        type=float,
        default=10.0,
        help="""\
        How much of the training data should be unknown words.
        """)
    parser.add_argument(
        '--testing_percentage',
        type=int,
        default=10,
        help='What percentage of wavs to use as a test set.')
    parser.add_argument(
        '--validation_percentage',
        type=int,
        default=10,
        help='What percentage of wavs to use as a validation set.')
    parser.add_argument(
        '--sample_rate',
        type=int,
        default=16000,
        help='Expected sample rate of the wavs',)
    parser.add_argument(
        '--clip_duration_ms',
        type=int,
        default=1000,
        help='Expected duration in milliseconds of the wavs',)
    parser.add_argument(
        '--window_size_ms',
        type=float,
        default=30.0,
        help='How long each spectrogram timeslice is',)
    parser.add_argument(
        '--window_stride_ms',
        type=float,
        default=10.0,
        help='How long each spectrogram timeslice is',)
    parser.add_argument(
        '--dct_coefficient_count',
        type=int,
        default=40,
        help='How many bins to use for the MFCC fingerprint',)
    parser.add_argument(
        '--batch_size',
        type=int,
        default=100,
        help='How many items to train with at once',)
    parser.add_argument(
        '--wanted_words',
        type=str,
        default='yes,no,up,down,left,right,on,off,stop,go',
        help='Words to use (others will be added to an unknown label)',)
    parser.add_argument(
        '--checkpoint',
        type=str,
        help='Checkpoint to load the weights from.')
    parser.add_argument(
        '--model_architecture',
        type=str,
        default='dnn',
        help='What model architecture to use')
    parser.add_argument(
        '--model_size_info',
        type=int,
        nargs="+",
        default=[128, 128, 128],
        help='Model dimensions - different for various models')

    FLAGS, _ = parser.parse_known_args()
    test()
