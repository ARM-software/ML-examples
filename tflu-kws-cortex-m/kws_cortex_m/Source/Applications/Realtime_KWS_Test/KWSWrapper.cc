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

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/Applications/Realtime_KWS_Test/KWSWrapper.h"


#define AUDIO_BUFFER_BLOCK_SIZE ((uint32_t)512)
#define AUDIO_BUFFER_TOTAL_SIZE ((uint32_t)16000)

KWSWrapper::KWSWrapper(int recording_win, int sliding_window_len, std::vector<std::string>& outputClass, int detectionThreshold)
        : KWS(recording_win, sliding_window_len),
          audioBufferDMATransferIn(2 * AUDIO_BUFFER_BLOCK_SIZE),
          currentAudioBufferSize(0), outputClass(outputClass), detectionThreshold(detectionThreshold)


{
    this->lcdUtils = std::unique_ptr<PlotUtils>(new PlotUtils(this->numMfccFeatures, this->numFrames));
    this->audioUtils = std::unique_ptr<AudioUtils>(new AudioUtils());

    /* Audio buffer size to be fed into the pre-processing block */
    const size_t audioForIfmSz = ((numFrames - 1) * frameShift) + frameLen;

    this->audioBufferAcc = std::vector<int16_t>(2*audioForIfmSz); /* Twice the size to allow collection of 1s of stereo*/

    this->audioBuffer = std::vector<int16_t>(audioForIfmSz); /* 16K */

}

void KWSWrapper::StartKWS(){
    /* Initialize SDRAM buffers */
    memset(audioBufferDMATransferIn.data(), 0, audioBufferDMATransferIn.size() * sizeof(int16_t));
    memset(audioBufferAcc.data(), 0, audioBufferAcc.size() * sizeof(int16_t));
    memset(audioBuffer.data(), 0, audioBuffer.size() * sizeof(int16_t));

    /* Initialize AudioUtils */
    this->audioUtils->AudioInit((uint16_t *)audioBufferDMATransferIn.data(), AUDIO_BUFFER_BLOCK_SIZE * 2);

    printf("KWS init done.\r\n");
}

void KWSWrapper::RunKWS(){

    printf("*    Extracting features.\r\n");
    this->ExtractFeatures();  // Extract MFCC features.

    printf("**   Classifying.\r\n");
    this->Classify();  // Classify using the model chosen.

    printf("***  Averaging predictions.\r\n");
    this->AveragePredictions();

    int maxIndex = this->GetTopClass(this->averagedOutput);

    std::string lcd_output_string ="";
    if(kwsWrapperPtr->averagedOutput[maxIndex]*100 >= detectionThreshold) {

        std::ostringstream oss;
        oss << (int)(kwsWrapperPtr->averagedOutput[maxIndex] * 100) << "% " << outputClass[maxIndex];
        lcd_output_string = oss.str();
        printf("**** Classified %s.\r\n\n", lcd_output_string.c_str());
        lcdUtils->ClearAll();
        lcdUtils->DisplayStringAtCentreMode(0, 200, lcd_output_string);

    }
    lcdUtils->PlotWaveform(this->audioBuffer, this->audioBlockSize, this->frameLen, this->frameShift);
    lcdUtils->PlotMfcc(this->mfccBuffer, this->numMfccFeatures, this->numFrames);
}

void KWSWrapper::StartAudioRecording()
{
    return this->audioUtils->StartAudioInRecord((uint16_t *)this->audioBufferDMATransferIn.data(),
            this->audioBufferDMATransferIn.size());
}

void KWSWrapper::StopAudioRecording()
{
    return this->audioUtils->StopAudioInRecord();
}


bool KWSWrapper::IsAudioAvailable()
{
    return kwsWrapperPtr->audioUtils->IsAudioAvailable();
}

void KWSWrapper::SetAudioEmpty()
{
    kwsWrapperPtr->audioUtils->SetAudioEmpty();
}

void KWSWrapper::PopulateMonoAudioBuffer()
{
    return this->audioUtils->ConvertToMono(
            this->audioBufferAcc.data(),
            this->audioBuffer.data(),
            this->audioBufferAcc.size() * sizeof(int16_t));
}