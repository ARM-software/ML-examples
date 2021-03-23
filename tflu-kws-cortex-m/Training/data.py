# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
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
# ==============================================================================
#
# Modifications Copyright 2020 Arm Inc. All Rights Reserved.
# Modified to use TensorFlow 2.0 and data pipelines.
#
"""Functions for loading and preparing data for keyword spotting."""

import os
import re
import sys
import urllib
from pathlib import Path
import tarfile
import hashlib
import random
import math
from enum import Enum

import numpy as np
import tensorflow as tf
from tensorflow.python.ops import gen_audio_ops as audio_ops

MAX_NUM_WAVS_PER_CLASS = 2**27 - 1  # ~134M
RANDOM_SEED = 59185
BACKGROUND_NOISE_DIR_NAME = '_background_noise_'
SILENCE_LABEL = '_silence_'
SILENCE_INDEX = 0
UNKNOWN_WORD_INDEX = 1
UNKNOWN_WORD_LABEL = '_unknown_'


def load_wav_file(wav_filename, desired_samples):
    """Loads and then decodes a given 16bit PCM wav file.

    Decoded audio is scaled to the range [-1, 1] and padded or cropped to the desired number of samples.

    Args:
        wav_filename: 16bit PCM wav file to load.
        desired_samples: Number of samples wanted from the audio file.

    Returns:
        Tuple consisting of the decoded audio and sample rate.
    """
    wav_file = tf.io.read_file(wav_filename)
    decoded_wav = audio_ops.decode_wav(wav_file, desired_channels=1, desired_samples=desired_samples)

    return decoded_wav.audio, decoded_wav.sample_rate


def calculate_mfcc(audio_signal, audio_sample_rate, window_size, window_stride, num_mfcc):
    """Returns Mel Frequency Cepstral Coefficients (MFCC) for a given audio signal.

    Args:
        audio_signal: Raw audio signal in range [-1, 1]
        audio_sample_rate: Audio signal sample rate
        window_size: Window size in samples for calculating spectrogram
        window_stride: Window stride in samples for calculating spectrogram
        num_mfcc: The number of MFCC features wanted.

    Returns:
        Calculated mffc features.
    """
    spectrogram = audio_ops.audio_spectrogram(input=audio_signal, window_size=window_size, stride=window_stride,
                                              magnitude_squared=True)

    mfcc_features = audio_ops.mfcc(spectrogram, audio_sample_rate, dct_coefficient_count=num_mfcc)

    return mfcc_features


def which_set(filename, validation_percentage, testing_percentage):
    """Determines which data partition the file should belong to.

    We want to keep files in the same training, validation, or testing sets even
    if new ones are added over time. This makes it less likely that testing
    samples will accidentally be reused in training when long runs are restarted
    for example. To keep this stability, a hash of the filename is taken and used
    to determine which set it should belong to. This determination only depends on
    the name and the set proportions, so it won't change as other files are added.
    It's also useful to associate particular files as related (for example words
    spoken by the same person), so anything after '_nohash_' in a filename is
    ignored for set determination. This ensures that 'bobby_nohash_0.wav' and
    'bobby_nohash_1.wav' are always in the same set, for example.

    Args:
        filename: File path of the data sample.
        validation_percentage: How much of the data set to use for validation.
        testing_percentage: How much of the data set to use for testing.

    Returns:
        String, one of 'training', 'validation', or 'testing'.
    """
    base_name = os.path.basename(filename)
    # We want to ignore anything after '_nohash_' in the file name when
    # deciding which set to put a wav in, so the data set creator has a way of
    # grouping wavs that are close variations of each other.
    hash_name = re.sub(r'_nohash_.*$', '', base_name)
    # This looks a bit magical, but we need to decide whether this file should
    # go into the training, testing, or validation sets, and we want to keep
    # existing files in the same set even if more files are subsequently
    # added.
    # To do that, we need a stable way of deciding based on just the file name
    # itself, so we do a hash of that and then use that to generate a
    # probability value that we use to assign it.
    hash_name_hashed = hashlib.sha1(tf.compat.as_bytes(hash_name)).hexdigest()
    percentage_hash = ((int(hash_name_hashed, 16) %
                       (MAX_NUM_WAVS_PER_CLASS + 1)) *
                       (100.0 / MAX_NUM_WAVS_PER_CLASS))
    if percentage_hash < validation_percentage:
        result = 'validation'
    elif percentage_hash < (testing_percentage + validation_percentage):
        result = 'testing'
    else:
        result = 'training'
    return result


def prepare_words_list(wanted_words):
    """Prepends common tokens to the custom word list.

    Args:
        wanted_words: List of strings containing custom words to spot.

    Returns:
        List of words with silence and unknown tokens added.
    """
    return [SILENCE_LABEL, UNKNOWN_WORD_LABEL] + wanted_words


class AudioProcessor:
    """Handles loading, partitioning, and preparing audio training data."""

    class Modes(Enum):
        TRAINING = 1
        VALIDATION = 2
        TESTING = 3

    def __init__(self, data_url, data_dir, silence_percentage, unknown_percentage,
                 wanted_words, validation_percentage, testing_percentage, model_settings):
        self.data_dir = Path(data_dir)
        self.model_settings = model_settings
        self.words_list = prepare_words_list(wanted_words)

        self._tf_datasets = {}
        self.background_data = []
        self._set_size = {'training': 0, 'validation': 0, 'testing': 0}

        self._download_and_extract_data(data_url, data_dir)
        self._prepare_datasets(silence_percentage, unknown_percentage, wanted_words,
                               validation_percentage, testing_percentage)
        self._prepare_background_data()

    def get_data(self, mode, background_frequency=0, background_volume_range=0, time_shift=0):
        """Returns the train, validation or test set for KWS as a TF Dataset.

        Args:
            mode: The set to return, see AudioProcessor.Modes enumeration.
            background_frequency: How many of the samples have background noise mixed in.
            background_volume_range: How loud the background noise should be, between 0 and 1.
            time_shift: Range to randomly shift the training audio by in time.

        Returns:
            TF dataset that will generate tuples containing an mfcc and corresponding label.

        Raises:
            ValueError: If mode is not recognised.
        """
        if mode == AudioProcessor.Modes.TRAINING:
            dataset = self._tf_datasets['training']
        elif mode == AudioProcessor.Modes.VALIDATION:
            dataset = self._tf_datasets['validation']
        elif mode == AudioProcessor.Modes.TESTING:
            dataset = self._tf_datasets['testing']
        else:
            ValueError("Incorrect dataset type given")

        use_background = (self.background_data != []) and (mode == AudioProcessor.Modes.TRAINING)
        dataset = dataset.map(lambda path, label: self._process_path(path, label, self.model_settings,
                                                                     background_frequency, background_volume_range,
                                                                     time_shift, use_background, self.background_data),
                              num_parallel_calls=tf.data.experimental.AUTOTUNE)

        return dataset

    def set_size(self, mode):
        """Get the number of samples in the requested dataset partition

        Args:
            mode: Which partition, see AudioProcessor.Modes enumeration.

        Returns:
            Number of samples in the partition

        Raises:
            ValueError: If mode is not recognised.
        """
        if mode == AudioProcessor.Modes.TRAINING:
            return self._set_size['training']
        elif mode == AudioProcessor.Modes.VALIDATION:
            return self._set_size['validation']
        elif mode == AudioProcessor.Modes.TESTING:
            return self._set_size['testing']
        else:
            ValueError('Incorrect dataset type given')

    @staticmethod
    def _process_path(path, label, model_settings, background_frequency, background_volume_range, time_shift_samples,
                      use_background, background_data):
        """Load wav files and calculate mfcc features.

        Random shifting of samples and adding in background noise is done within this function as well.
        This function is meant to be mapped onto a TF Dataset by using a lambda function.

        Args:
            path: Path to the wav file to load.
            label: Integer label for classifying the audio clip.
            model_settings: Dictionary of settings for model being trained.
            background_frequency: How many clips will have background noise, 0.0 to 1.0.
            background_volume_range: How loud the background noise will be.
            time_shift_samples: How much to randomly shift the clips by.
            use_background: Add in background noise to audio clips or not.
            background_data: Ragged tensor of loaded background noise samples.

        Returns:
            Tuple of calculated flattened mfcc and its class label.
        """

        desired_samples = model_settings['desired_samples']
        audio, sample_rate = load_wav_file(path, desired_samples=desired_samples)

        # Make our own silence audio data.
        if label == SILENCE_INDEX:
            audio = tf.multiply(audio, 0)

        # Shift samples start position and pad any gaps with zeros.
        if time_shift_samples > 0:
            time_shift_amount = tf.random.uniform(shape=(), minval=-time_shift_samples, maxval=time_shift_samples,
                                                  dtype=tf.int32)
        else:
            time_shift_amount = 0
        if time_shift_amount > 0:
            time_shift_padding = [[time_shift_amount, 0], [0, 0]]
            time_shift_offset = [0, 0]
        else:
            time_shift_padding = [[0, -time_shift_amount], [0, 0]]
            time_shift_offset = [-time_shift_amount, 0]

        padded_foreground = tf.pad(audio, time_shift_padding, mode='CONSTANT')
        sliced_foreground = tf.slice(padded_foreground, time_shift_offset, [desired_samples, -1])

        # Get a random section of background noise.
        if use_background:
            background_index = tf.random.uniform(shape=(), maxval=len(background_data), dtype=tf.int32)

            background_sample = background_data[background_index]
            background_offset = tf.random.uniform(shape=(), maxval=len(background_sample)-desired_samples,
                                                  dtype=tf.int32)
            background_clipped = background_sample[background_offset:(background_offset + desired_samples)]
            background_reshaped = tf.reshape(background_clipped, [desired_samples, 1])
            if tf.random.uniform(shape=(), maxval=1) < background_frequency:
                background_volume = tf.random.uniform(shape=(), maxval=background_volume_range)
            else:
                background_volume = tf.constant(0, dtype='float32')
        else:
            background_reshaped = np.zeros([desired_samples, 1], dtype=np.float32)
            background_volume = tf.constant(0, dtype='float32')

        # Mix in background noise.
        background_mul = tf.multiply(background_reshaped, background_volume)
        background_add = tf.add(background_mul, sliced_foreground)
        background_clamp = tf.clip_by_value(background_add, -1.0, 1.0)

        mfcc = calculate_mfcc(background_clamp, sample_rate, model_settings['window_size_samples'],
                              model_settings['window_stride_samples'],
                              model_settings['dct_coefficient_count'])
        mfcc = tf.reshape(mfcc, [-1])

        return mfcc, label

    def _download_and_extract_data(self, data_url, target_directory):
        """Downloads and extracts file to target directory.

        If the file does not already exist download it and then untar into the target directory.

        Args:
            data_url: Web link to the tarred data to download.
            target_directory: Directory to download and extract to.
        """
        target_directory = Path(target_directory)
        target_directory.mkdir(exist_ok=True)

        filename = data_url.split('/')[-1]
        filepath = target_directory / filename

        if not filepath.exists():
            def _report_hook(block_num, block_size, total_size):
                """Function to track download progress in urllib"""
                read_so_far = block_num * block_size
                percent = (read_so_far / total_size) * 100.0

                s = f"\rDownloading {filename} {percent:.1f}%"

                sys.stdout.write(s)
                sys.stdout.flush()

            filepath, _ = urllib.request.urlretrieve(data_url, filepath, _report_hook)
            print()

        print(f'Untarring {filename}...')
        tarfile.open(filepath, 'r:gz').extractall(target_directory)

    def _prepare_datasets(self, silence_percentage, unknown_percentage, wanted_words,
                          validation_percentage, testing_percentage):
        """Split the data into train, validation and testing sets.

        Silence and unknown data is added, then sets are converted to TF Datasets.

        Args:
            silence_percentage: Percent of words should be silence.
            unknown_percentage: Percent of words that should be unknown.
            wanted_words: List of words wanted to classify.
            validation_percentage: Percent to split off for validation.
            testing_percentage: Percent to split off for testing.
        """
        # Make sure the shuffling and picking of unknowns is deterministic.
        random.seed(RANDOM_SEED)
        wanted_words_index = {}

        for index, wanted_word in enumerate(wanted_words):
            wanted_words_index[wanted_word] = index + 2

        # Find all wav files in subfolders.
        search_path = self.data_dir / '*' / '*.wav'
        data_index, unknown_index, all_words = self._find_and_sort_wavs(search_path, validation_percentage,
                                                                        testing_percentage, wanted_words_index)

        for index, wanted_word in enumerate(wanted_words):
            if wanted_word not in all_words:
                raise Exception(f'Tried to find {wanted_word} in labels but only found: {", ".join(all_words.keys())}')

        word_to_index = {}
        for word in all_words:
            if word in wanted_words_index:
                word_to_index[word] = wanted_words_index[word]
            else:
                word_to_index[word] = UNKNOWN_WORD_INDEX
        word_to_index[SILENCE_LABEL] = SILENCE_INDEX

        # We need an arbitrary file to load as the input for the silence samples.
        # It's multiplied by zero later, so the content doesn't matter.
        silence_wav_path = data_index['training'][0]['file']
        for set_index in ['validation', 'testing', 'training']:
            set_size = len(data_index[set_index])  # Size before adding silence and unknown samples.
            silence_size = int(math.ceil(set_size * silence_percentage / 100))
            for _ in range(silence_size):
                data_index[set_index].append({
                    'label': SILENCE_LABEL,
                    'file': silence_wav_path
                })
            # Pick some unknowns to add to each partition of the data set.
            random.shuffle(unknown_index[set_index])
            unknown_size = int(math.ceil(set_size * unknown_percentage / 100))
            data_index[set_index].extend(unknown_index[set_index][:unknown_size])

            self._set_size[set_index] = len(data_index[set_index])  # Size after adding silence and unknown samples.

            # Make sure the ordering is random.
            random.shuffle(data_index[set_index])

            # Transform into TF Datasets ready for easier processing later.
            labels, paths = list(zip(*[d.values() for d in data_index[set_index]]))
            labels = [word_to_index[label] for label in labels]
            self._tf_datasets[set_index] = tf.data.Dataset.from_tensor_slices((list(paths), labels))

    def _find_and_sort_wavs(self, search_pattern, validation_percentage, testing_percentage, wanted_words_index):
        """Find and sort wav files into known and unknown word sets.

        Known words are files containing words in the list of wanted words.
        Any other clip goes to the unknown label set. Labels come from the folder names.
        All clips are also assigned to train, test and validation sets.

        Args:
            search_pattern: Path pattern used by glob to find wav files.
            validation_percentage: Percent to split off for validation.
            testing_percentage: Percent to split off for testing.
            wanted_words_index: Dict mapping wanted words to their label index.

        Returns:
            3-tuple of known words, unknown words and mapping of all word labels.
        """
        data_index = {'validation': [], 'testing': [], 'training': []}
        unknown_index = {'validation': [], 'testing': [], 'training': []}
        all_words = {}

        for wav_path in tf.io.gfile.glob(str(search_pattern)):
            word = Path(wav_path).parent.name.lower()

            # Treat the '_background_noise_' folder as a special case, since we expect
            # it to contain long audio samples we mix in to improve training.
            if word == BACKGROUND_NOISE_DIR_NAME:
                continue

            all_words[word] = True
            set_index = which_set(wav_path, validation_percentage, testing_percentage)
            # If it's a known class, store its detail, otherwise add it to the list
            # we'll use to train the unknown label.
            if word in wanted_words_index:
                data_index[set_index].append({'label': word, 'file': wav_path})
            else:
                unknown_index[set_index].append({'label': word, 'file': wav_path})
        if not all_words:
            raise Exception('No .wavs found at ' + search_pattern)

        return data_index, unknown_index, all_words

    def _prepare_background_data(self):
        """Searches a folder for background noise audio, and loads it into memory.

        It's expected that the background audio samples will be in a subdirectory
        named '_background_noise_' inside the 'data_dir' folder, as .wavs that match
        the sample rate of the training data, but can be much longer in duration.

        If the '_background_noise_' folder doesn't exist at all, this isn't an
        error, it's just taken to mean that no background noise augmentation should
        be used. If the folder does exist, but it's empty, that's treated as an
        error.

        Returns:
          Ragged tensor of raw PCM-encoded audio samples of background noise.
          None if '_background_noise_' folder doesnt exist.

        Raises:
          Exception: If files aren't found in the folder.
        """
        background_data = []
        background_dir = Path(self.data_dir / BACKGROUND_NOISE_DIR_NAME)
        if not background_dir.exists():
            return

        search_path = Path(background_dir / '*.wav')
        for wav_path in tf.io.gfile.glob(str(search_path)):
            wav_data, _ = load_wav_file(wav_path, desired_samples=-1)
            # Because the wav_data tensors can have unequal ranks, which will make 
            # tf.ragged.stack to throw an error, we only stack the first n elements,
            # of each tensor, where n is the number of elements in the first wav tensor.
            if background_data:
                background_data.append(tf.reshape(wav_data[:tf.shape(background_data[0])[0]], [-1]))
            else:
                background_data.append(tf.reshape(wav_data, [-1]))

        if not background_data:
            raise Exception('No background wav files were found in ' + search_path)

        # Ragged tensor as we cant use lists in tf dataset map functions.
        self.background_data = tf.ragged.stack(background_data)
