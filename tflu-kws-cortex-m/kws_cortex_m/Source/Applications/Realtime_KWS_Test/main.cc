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
 * Description: Example code for running realtime keyword spotting on Cortex-M boards
 */

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/Applications/Realtime_KWS_Test/KWSWrapper.h"
#include "BufAttributes.h"

#include <string>
#include <vector>

KWSWrapper *kwsWrapperPtr;

int main()
{
    std::vector<std::string> outputClass = {
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

    /* Tune the following three parameters to improve the detection accuracy
    *  and reduce false positives
    *  Longer averaging window and higher threshold reduce false positives
    *  but increase detection latency and reduce true positive detections.
    * (recording_win*frame_shift) is the actual recording window size */
    int recordingWin = 49;
    /* Averaging window for smoothing out the output predictions */
    int averagingWindowLen = 1;
    /* Detection threshold in percent */
    int detectionThreshold = 50;

    printf("Instantiating KWSWrapper object\r\n");
    KWSWrapper kwsObj{recordingWin, averagingWindowLen, outputClass, detectionThreshold};
    kwsWrapperPtr = &kwsObj;
    printf("Starting KWS..\r\n");
    kwsWrapperPtr->StartKWS();
    kwsWrapperPtr->SetAudioEmpty();

    kwsWrapperPtr->StartAudioRecording();

    while (1) {
        /* A dummy loop to wait for the interrupts. */
        __WFI();

        if (kwsWrapperPtr->IsAudioAvailable())
        {
            /*Pause recording until explicitly restarted */
            kwsWrapperPtr->StopAudioRecording();

            kwsWrapperPtr->SetAudioEmpty();
            kwsWrapperPtr->PopulateMonoAudioBuffer();
            kwsWrapperPtr->StartAudioRecording();
            kwsWrapperPtr->RunKWS();
        }
    }
}