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

#ifndef __KWS_WRAPPER_H__
#define __KWS_WRAPPER_H__

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/KWS/kws.h"
#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/Applications/Realtime_KWS_Test/PlotUtils.h"
#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/Applications/Realtime_KWS_Test/AudioUtils.h"

#include <string>
#include <vector>

class KWSWrapper : public KWS 
{
public:
    KWSWrapper(int recording_win, int sliding_window_len, std::vector<std::string>& outputClass, int detectionThreshold);

    void StartKWS();
    void RunKWS();
    bool IsAudioAvailable();
    void SetAudioEmpty();
    void StartAudioRecording();
    void StopAudioRecording();
    void PopulateMonoAudioBuffer();

    std::unique_ptr<PlotUtils> lcdUtils;
    std::unique_ptr<AudioUtils> audioUtils;
    std::vector<int16_t> audioBufferDMATransferIn; /* Buffer for raw audio input from the hardware - stereo */
    std::vector<int16_t> audioBufferAcc; /* Buffer for accumulating 1s stereo audio */

    size_t currentAudioBufferSize;  /* Marks how much of the Acc buffer has been populated */

private:
    std::vector<std::string> outputClass;
    int detectionThreshold;
};

extern KWSWrapper* kwsWrapperPtr;

#endif /* __KWS_WRAPPER_H__ */