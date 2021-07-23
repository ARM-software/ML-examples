/*
 * Copyright (C) 2021 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Description: Keyword spotting wrapper class.
 */

#ifndef __AUDIO_UTILS_H__
#define __AUDIO_UTILS_H__

#include <string>
#include <vector>

class AudioUtils
{
public:
    AudioUtils();

    void SetVolumeIn(uint8_t vol);
    void SetVolumeOut(uint8_t vol);
    void AudioInit(uint16_t *audioBufferIn, uint32_t nbrOfBytesIn);
    bool IsAudioAvailable();
    void SetAudioEmpty();

    void StartAudioInRecord(uint16_t *audioBufferIn, uint32_t nbrOfBytesIn);
    void StopAudioInRecord();

    // Convert stereo audio to mono
    void ConvertToMono(int16_t* audioS, int16_t* audioM, uint32_t nbrBytesStereo);

    uint32_t  audioRecBufferState;

private:
    uint8_t SetSysClock_PLL_HSE_200MHz();
};


#endif /* __AUDIO_UTILS_H__ */