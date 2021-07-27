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
"""Functions for optimizing simple keyword spotting models using clustering API."""

import argparse
from pathlib import Path

import tensorflow as tf
import numpy as np
import tensorflow_model_optimization as tfmot

import data
import models


def print_model_weight_clusters(model):

    for layer in model.layers:
        if isinstance(layer, tf.keras.layers.Wrapper):
            weights = layer.trainable_weights
        else:
            weights = layer.weights
        for weight in weights:
            if "kernel" in weight.name:
                unique_count = len(np.unique(weight))
                print(
                    f"{layer.name}/{weight.name}: {unique_count} clusters "
                )


def optimize():
    model_settings = models.prepare_model_settings(len(data.prepare_words_list(FLAGS.wanted_words.split(','))),
                                                   FLAGS.sample_rate, FLAGS.clip_duration_ms, FLAGS.window_size_ms,
                                                   FLAGS.window_stride_ms, FLAGS.dct_coefficient_count)

    # Create the model to optimize from checkpoint.
    model = models.create_model(model_settings, FLAGS.model_architecture, FLAGS.model_size_info)
    model.load_weights(FLAGS.checkpoint).expect_partial()

    audio_processor = data.AudioProcessor(data_url=FLAGS.data_url,
                                          data_dir=FLAGS.data_dir,
                                          silence_percentage=FLAGS.silence_percentage,
                                          unknown_percentage=FLAGS.unknown_percentage,
                                          wanted_words=FLAGS.wanted_words.split(','),
                                          validation_percentage=FLAGS.validation_percentage,
                                          testing_percentage=FLAGS.testing_percentage,
                                          model_settings=model_settings)

    # We decay learning rate in a constant piecewise way to help learning.
    training_steps_list = list(map(int, FLAGS.how_many_training_steps.split(',')))
    learning_rates_list = list(map(float, FLAGS.learning_rate.split(',')))
    lr_boundary_list = training_steps_list[:-1]  # Only need the values at which to change lr.
    lr_schedule = tf.keras.optimizers.schedules.PiecewiseConstantDecay(boundaries=lr_boundary_list,
                                                                       values=learning_rates_list)

    cluster_weights = tfmot.clustering.keras.cluster_weights
    CentroidInitialization = tfmot.clustering.keras.CentroidInitialization

    clustering_params = {
        'number_of_clusters': 32,
        'cluster_centroids_init': CentroidInitialization.KMEANS_PLUS_PLUS}

    clustered_model = cluster_weights(model, **clustering_params)

    # Specify the optimizer configurations.
    optimizer = tf.keras.optimizers.Adam(learning_rate=lr_schedule)
    clustered_model.compile(optimizer=optimizer,
                            loss=tf.keras.losses.SparseCategoricalCrossentropy(),
                            metrics=['accuracy'])

    train_data = audio_processor.get_data(audio_processor.Modes.TRAINING,
                                          FLAGS.background_frequency, FLAGS.background_volume,
                                          int((FLAGS.time_shift_ms * FLAGS.sample_rate) / 1000))
    train_data = train_data.repeat().batch(FLAGS.batch_size).prefetch(tf.data.AUTOTUNE)
    val_data = audio_processor.get_data(audio_processor.Modes.VALIDATION)
    val_data = val_data.batch(FLAGS.batch_size).prefetch(tf.data.AUTOTUNE)

    # We train for a max number of iterations so need to calculate how many 'epochs' this will be.
    training_steps_max = np.sum(training_steps_list)
    training_epoch_max = int(np.ceil(training_steps_max / FLAGS.eval_step_interval))

    # Train the model with clustering applied.
    clustered_model.fit(x=train_data,
                        steps_per_epoch=FLAGS.eval_step_interval,
                        epochs=training_epoch_max,
                        validation_data=val_data)

    stripped_clustered_model = tfmot.clustering.keras.strip_clustering(clustered_model)

    print_model_weight_clusters(stripped_clustered_model)

    # Save the clustered model weights
    train_dir = Path(FLAGS.train_dir) / "optimized"
    train_dir.mkdir(parents=True, exist_ok=True)

    stripped_clustered_model.save_weights((train_dir /
                                          (FLAGS.model_architecture +
                                           "_clustered_ckpt")))

    # Test the model.
    test_data = audio_processor.get_data(audio_processor.Modes.TESTING)
    test_data = test_data.batch(FLAGS.batch_size)

    stripped_clustered_model.compile(optimizer=optimizer,
                                     loss=tf.keras.losses.SparseCategoricalCrossentropy(),
                                     metrics=['accuracy'])

    test_loss, test_acc = stripped_clustered_model.evaluate(x=test_data)
    print(f'Final test accuracy: {test_acc*100:.2f}%')


if __name__ == "__main__":
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
        '--background_volume',
        type=float,
        default=0.1,
        help="""\
        How loud the background noise should be, between 0 and 1.
        """)
    parser.add_argument(
        '--background_frequency',
        type=float,
        default=0.8,
        help="""\
        How many of the training samples have background noise mixed in.
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
        '--time_shift_ms',
        type=float,
        default=100.0,
        help="""\
        Range to randomly shift the training audio by in time.
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
        '--how_many_training_steps',
        type=str,
        default='3750,750',
        help='How many training loops to run',)
    parser.add_argument(
        '--eval_step_interval',
        type=int,
        default=400,
        help='How often to evaluate the training results.')
    parser.add_argument(
        '--learning_rate',
        type=str,
        default='0.001,0.0001',
        help='How large a learning rate to use when training.')
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
        '--train_dir',
        type=str,
        default='/tmp/speech_commands_train',
        help='Directory to write event logs and checkpoint.')
    parser.add_argument(
        '--save_step_interval',
        type=int,
        default=100,
        help='Save model checkpoint every save_steps.')
    parser.add_argument(
        '--checkpoint',
        type=str,
        help='Checkpoint to load the weights from before fine-tuning.')
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
    optimize()
