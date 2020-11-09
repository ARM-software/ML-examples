/*
 * Copyright (C) 2020 Arm Limited or its affiliates. All rights reserved.
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
 * Description: Example code for running keyword spotting on Cortex-M boards
 */

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/KWS/kws.h"
#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/Applications/Simple_KWS_Test/wav_data.h"
#include "BufAttributes.h"
#include <vector>

int main()
{
    const char outputClass[12][8] = {
        "Silence",
        "Unknown",
        "yes",
        "no",
        "up",
        "down",
        "left",
        "right",
        "on",
        "off",
        "stop",
        "go"};

    int16_t audioBuffer[] ALIGNMENT_ATTRIBUTE = WAVE_DATA;
    const uint32_t audioBufferElements = sizeof(audioBuffer) / sizeof(int16_t);

    printf("KWS simple example; build timestamp: %s:%s\n", __DATE__, __TIME__);

    printf("Initialising KWS object. Wav data has %lu elements\r\n",
        audioBufferElements);
    KWS kws(audioBuffer, audioBufferElements);

    printf("Extracting features.. \r\n");
    kws.ExtractFeatures();  // Extract MFCC features.

    printf("Classifying..\r\n");
    kws.Classify();  // Classify the extracted features.

    int maxIndex = kws.GetTopClass(kws.output);

    printf("Detected %s (%d%%)\r\n", outputClass[maxIndex],
        (static_cast<int>(kws.output[maxIndex]*100)));

    return 0;
}