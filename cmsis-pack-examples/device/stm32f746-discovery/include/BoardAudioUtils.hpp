/*
 * SPDX-FileCopyrightText: Copyright 2021-2022 Arm Limited and/or its
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

/*
 * Description: Keyword spotting wrapper class.
 */

#ifndef __AUDIO_UTILS_HPP__
#define __AUDIO_UTILS_HPP__

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
     * @param[in]   audioBufferInStereo Buffer descriptor for the audio interface to use.
     * @return      True if successful, false otherwise.
     */
    bool AudioInit(audio_buf* audioBufferInStereo);

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
    void StartAudioInRecord();

    /**
     * @brief   Stops recording the audio stream.
     */
    void StopAudioInRecord();

private:
    /**
     * @brief   Low level initialisation required for audio streaming initialisation.
     */
    uint8_t SetSysClock_PLL_HSE_200MHz();
};

#endif /* __AUDIO_UTILS_H__ */
