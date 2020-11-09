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

#ifndef __KWS_H__
#define __KWS_H__

#include <vector>

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/NN/Model.h"
#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/MFCC/mfcc.h"

class KWS {

public:
    KWS(int recordingWin, int slidingWindowLen);
    KWS(int16_t * ptrAudioBuffer, uint32_t nElements);

    ~KWS() = default;

    void ExtractFeatures();
    void Classify();
    void AveragePredictions();
    int GetTopClass(const std::vector<float>& prediction);

    std::vector<int16_t> audioBuffer;
    std::vector<float> mfccBuffer;
    std::vector<float> output;
    std::vector<float> predictions;
    std::vector<float> averagedOutput;
    int numFrames;
    int numMfccFeatures;
    int frameLen;
    int frameShift;
    int numOutClasses;
    int audioBlockSize;
    int audioBufferSize;

protected:
    /** @brief Initialises the model */
    bool _InitModel();
    void InitKws();
    std::unique_ptr<MFCC> mfcc;
    std::unique_ptr<Model> model;
    int mfccBufferSize;
    int recordingWin;
    int slidingWindowLen;
};

#endif /* __KWS_H__ */
