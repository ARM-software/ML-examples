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
 * Description: Keyword spotting example code using MFCC feature extraction
 * and neural network.
 */

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/KWS/kws.h"
#include <float.h>

KWS::KWS(int recordWindow, int slidingWinLen)
{
    if (this->_InitModel()) {
        this->recordingWin = recordWindow;
        this->slidingWindowLen = slidingWinLen;
        this->InitKws();
    }
}

KWS::KWS(int16_t * ptrAudioBuffer, uint32_t nElements)
{
    if (this->_InitModel()) {
        this->audioBuffer = std::move(std::vector<int16_t>(
                                            ptrAudioBuffer,
                                            ptrAudioBuffer + nElements));
        this->recordingWin = model->GetNumFrames();
        this->slidingWindowLen = 1;
        this->InitKws();
    }
}

void KWS::InitKws()
{
    if (!model->IsInited()) {
        printf("Warning: model has not been initialised\r\n");
        model->Init();
    }

    numMfccFeatures = model->GetNumMfccFeatures();
    numFrames = model->GetNumFrames();
    frameLen = model->GetFrameLen();
    frameShift = model->GetFrameShift();
    numOutClasses = model->GetOutputShape()->data[1];  // Output shape should be [1, numOutClasses].

    // Following are for debug purposes.
    printf("Initialising KWS object..\r\n");
    printf("numMfccFeatures: %d\r\n", numMfccFeatures);
    printf("numFrames: %d\r\n", numFrames);
    printf("frameLen: %d\r\n", frameLen);
    printf("frameShift: %d\r\n", frameShift);
    printf("numOutClasses: %d\r\n", numOutClasses);

    mfcc =  std::unique_ptr<MFCC>(new MFCC(numMfccFeatures, frameLen));
    mfccBuffer = std::vector<float>(numFrames * numMfccFeatures, 0.0);
    output = std::vector<float>(numOutClasses, 0.0);
    averagedOutput = std::vector<float>(numOutClasses, 0.0);
    predictions = std::vector<float>(slidingWindowLen * numOutClasses, 0.0);
    audioBlockSize = recordingWin * frameShift;
    audioBufferSize = audioBlockSize + frameLen - frameShift;
}

void KWS::ExtractFeatures()
{
    if (numFrames > recordingWin) {
        // Move old features left.
        memmove(mfccBuffer.data(),
                mfccBuffer.data() + (recordingWin * numMfccFeatures),
                (numFrames - recordingWin) * numMfccFeatures * sizeof(float));
    }
    // Compute features only for the newly recorded audio.
    int32_t mfccBufferHead = (numFrames - recordingWin) * numMfccFeatures;
    for (uint16_t f = 0; f < recordingWin; f++) {
        mfcc->MfccCompute(audioBuffer.data() + (f * frameShift), &mfccBuffer[mfccBufferHead]);
        mfccBufferHead += numMfccFeatures;
    }
}

void KWS::Classify()
{
    // Copy mfcc features into the TfLite tensor.
    float* inTensorData = tflite::GetTensorData<float>(model->GetInputTensor());
    memcpy(inTensorData, mfccBuffer.data(), numFrames * numMfccFeatures * sizeof(float));

    // Run inference on this data.
    model->RunInference();

    // Get output from the TfLite tensor.
    float* outTensorData = tflite::GetTensorData<float>(model->GetOutputTensor());
    memcpy(output.data(), outTensorData, numOutClasses * sizeof(float));
}

int KWS::GetTopClass(const std::vector<float>& prediction)
{
    int maxInd = 0;
    float maxVal = FLT_MIN;
    for (int i = 0; i < numOutClasses; i++) {
        if (maxVal < prediction[i]) {
            maxVal = prediction[i];
            maxInd = i;
        }
    }
    return maxInd;
}

void KWS::AveragePredictions()
{
    // Shift the old predictions left.
    memmove(predictions.data(),
            predictions.data() + numOutClasses,
            (slidingWindowLen - 1) * numOutClasses * sizeof(float));

    // Add new predictions at the end.
    memmove((predictions.data() + (slidingWindowLen - 1 ) * numOutClasses),
            output.data(),
            numOutClasses * sizeof(float));

    // Compute averages.
    int sum;
    for (int j = 0; j < numOutClasses; j++) {
        sum = 0;
        for(int i = 0; i < slidingWindowLen; i++) {
            sum += predictions[i*numOutClasses + j];
        }
        averagedOutput[j] = (sum / slidingWindowLen);
    }
}
