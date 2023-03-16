/*
 * SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BOARD_AUDIO_UTILS_HPP
#define BOARD_AUDIO_UTILS_HPP

#include <string>
#include <vector>

/**
 * @brief   Audio buffer descriptor
 */
extern "C" typedef struct _audio_buf {
    void* data;          /**< Pointer to buffer data. */
    uint32_t n_elements; /**< Number of elements in this buffer. */
    uint32_t n_bytes;    /**< Total number of bytes occupied by this buffer. */
} audio_buf;

/**
 * @brief Audio utility class.
 */
class AudioUtils {
public:
    AudioUtils();
    ~AudioUtils();

    /**
     * @brief       Sets the input volume
     * @param[in]   vol Volume to be set (value between 0-min and 100-max)
     */
    void SetVolumeIn(uint8_t vol);

    /**
     * @brief       Sets the output volume
     * @param[in]   vol Volume to be set (value between 0-min and 100-max)
     */
    void SetVolumeOut(uint8_t vol);

    /**
     * @brief       Initialises the audio input interface.
     * @param[in]   audioBufferIn Buffer descriptor for the audio interface to use.
     * @return      True if successful, false otherwise.
     */
    bool AudioInit(audio_buf* audioBufferIn);

    /**
     * @brief   Checks if the audio buffer has been populated.
     * @return  True if buffer is full, false otherwise.
     */
    bool IsAudioAvailable();

    /**
     * @brief   Sets the audio buffer as empty - useful to restart populating
     *          the audio buffer.
     */
    void SetAudioEmpty();

    /**
     * @brief   Starts recording the audio stream into the buffer provided at initialisation.
     */
    void StartAudioRecording();

    /**
     * @brief   Stops recording the audio stream.
     */
    void StopAudioRecording();

    /**
     * @brief   Gets if the recorded audio is stereo
     */
    bool IsStereo() const;
};

#endif /* BOARD_AUDIO_UTILS_HPP */
