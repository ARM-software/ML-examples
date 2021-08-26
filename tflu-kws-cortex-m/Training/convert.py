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
"""Functions for converting and quantizing a trained keyword spotting
   model and saving to TFLite."""

import argparse

import tensorflow as tf

import data
import models
from test_tflite import tflite_test

NUM_REP_DATA_SAMPLES = 100  # How many samples to use for post training quantization.


def convert(model_settings, audio_processor, checkpoint, quantize, inference_type, tflite_path):
    """Load our trained floating point model and convert it.

    TFLite conversion or post training quantization is performed and the
    resulting model is saved as a TFLite file.
    We use samples from the validation set to do post training quantization.

    Args:
        model_settings: Dictionary of common model settings.
        audio_processor: Audio processor class object.
        checkpoint: Path to training checkpoint to load.
        quantize: Whether to quantize the model or convert to fp32 TFLite model.
        inference_type: Input/output type of the quantized model.
        tflite_path: Output TFLite file save path.
    """
    model = models.create_model(model_settings, FLAGS.model_architecture, FLAGS.model_size_info, False)
    model.load_weights(checkpoint).expect_partial()

    val_data = audio_processor.get_data(audio_processor.Modes.VALIDATION).batch(1)

    def _rep_dataset():
        """Generator function to produce representative dataset."""
        i = 0
        for mfcc, label in val_data:
            if i > NUM_REP_DATA_SAMPLES:
                break
            i += 1
            yield [mfcc]

    if quantize:
        # Quantize model and save to disk.
        tflite_model = post_training_quantize(model, inference_type, _rep_dataset)
        with open(tflite_path, 'wb') as f:
            f.write(tflite_model)
        print(f'Quantized model saved to {tflite_path}.')
    else:
        converter = tf.lite.TFLiteConverter.from_keras_model(model)
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        tflite_model = converter.convert()
        with open(tflite_path, 'wb') as f:
            f.write(tflite_model)
        print(f'Converted model saved to {tflite_path}.')


def post_training_quantize(keras_model, inference_type, rep_dataset):
    """Perform post training quantization and returns the TFLite model ready for saving.

    See https://www.tensorflow.org/lite/performance/post_training_quantization#full_integer_quantization for
    more details.

    Args:
        keras_model: The trained tf Keras model used for post training quantization.
        inference_type: Input/output type of the quantized model.
        rep_dataset: Function to use as a representative dataset, must be callable.

    Returns:
        Quantized TFLite model ready for saving to disk.
    """
    converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]

    if inference_type=='int8':
        converter.inference_input_type = tf.int8
        converter.inference_output_type = tf.int8

    # Int8 post training quantization needs representative dataset.
    converter.representative_dataset = rep_dataset
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]

    tflite_model = converter.convert()

    return tflite_model


def main():
    model_settings = models.prepare_model_settings(len(data.prepare_words_list(FLAGS.wanted_words.split(','))),
                                                   FLAGS.sample_rate, FLAGS.clip_duration_ms, FLAGS.window_size_ms,
                                                   FLAGS.window_stride_ms, FLAGS.dct_coefficient_count)

    audio_processor = data.AudioProcessor(data_url=FLAGS.data_url,
                                          data_dir=FLAGS.data_dir,
                                          silence_percentage=FLAGS.silence_percentage,
                                          unknown_percentage=FLAGS.unknown_percentage,
                                          wanted_words=FLAGS.wanted_words.split(','),
                                          validation_percentage=FLAGS.validation_percentage,
                                          testing_percentage=FLAGS.testing_percentage,
                                          model_settings=model_settings)

    if FLAGS.quantize:
        tflite_path = f'{FLAGS.model_architecture}_quantized.tflite'
    else:
        tflite_path = f'{FLAGS.model_architecture}.tflite'

    # Load floating point model from checkpoint and convert it.
    convert(model_settings, audio_processor, FLAGS.checkpoint,
            FLAGS.quantize, FLAGS.inference_type, tflite_path)

    # Test the newly converted model on the test set.
    tflite_test(model_settings, audio_processor, tflite_path)


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
        '--wanted_words',
        type=str,
        default='yes,no,up,down,left,right,on,off,stop,go',
        help='Words to use (others will be added to an unknown label)',)
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
    parser.add_argument(
        '--checkpoint',
        type=str,
        help='Checkpoint to load the weights from.')
    parser.add_argument(
        '--quantize',
        dest='quantize',
        action="store_true",
        default=True,
        help='Whether to quantize the model or convert to fp32 TFLite model. Defaults to True.')
    parser.add_argument(
        '--no-quantize',
        dest='quantize',
        action="store_false",
        help='Whether to quantize the model or convert to fp32 TFLite model. Defaults to True.')
    parser.add_argument(
        '--inference_type',
        type=str,
        default='fp32',
        help='If quantize is true, whether the model input and output is float32 or int8')

    FLAGS, _ = parser.parse_known_args()
    main()
