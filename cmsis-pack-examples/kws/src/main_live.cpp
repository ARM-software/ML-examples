/*
 * SPDX-FileCopyrightText: Copyright 2022-2023 Arm Limited and/or its
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
/**
 * This keyword spotting example is intended to work with the
 * CMSIS pack produced by ml-embedded-eval-kit. The pack consists
 * of platform agnostic end-to-end ML use case API's that can be
 * used to construct ML examples for any target that can support
 * the memory requirements for TensorFlow-Lite-Micro framework and
 * some heap for the API runtime.
 */
#include "AudioUtils.hpp"       /* Generic audio utilities like sliding windows. */
#include "BufAttributes.hpp"    /* Buffer attributes to be applied. */
#include "Classifier.hpp"       /* Classifier for the result. */
#include "KwsProcessing.hpp"    /* Pre and Post Process. */
#include "KwsResult.hpp"        /* KWS results class. */
#include "Labels.hpp"           /* Label Data for the model. */
#include "MicroNetKwsModel.hpp" /* Model API. */

#include <string>
#include <vector>

/* Platform dependent files */
#include "RTE_Components.h"  /* Provides definition for CMSIS_device_header */
#include CMSIS_device_header /* Gives us IRQ num, base addresses. */
#include "BoardInit.hpp"      /* Board initialisation */
#include "log_macros.h"      /* Logging macros (optional) */

#include "BoardAudioUtils.hpp" /* Board specific audio utilities - recording audio. */
#include "BoardPlotUtils.hpp"  /* Board specific display utilities. */

namespace arm {
namespace app {

    /* Tensor arena buffer */
    static uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;
    static int16_t audioBufferDMA[16000]; /* half a second worth of stereo audio or full second worth of mono */
    static int16_t audioBufferForNN[16000]; /* one full second worth of mono audio */

    static audio_buf dmaBuf = {.data       = audioBufferDMA,
                               .n_elements = sizeof(audioBufferDMA) >> 1,
                               .n_bytes    = sizeof(audioBufferDMA)};

    static audio_buf monoBuf = {.data       = audioBufferForNN,
                                .n_elements = sizeof(audioBufferForNN) >> 1,
                                .n_bytes    = sizeof(audioBufferForNN)};

    /* Optional getter function for the model pointer and its size. */
    namespace kws {
        extern uint8_t* GetModelPointer();
        extern size_t GetModelLen();
    } /* namespace kws */
} /* namespace app */
} /* namespace arm */

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

static int32_t CalculateOffset(audio_buf* audioBuffer);

static int32_t CalculateScale(audio_buf* audioBuffer);

static void ApplyGainAndOffset(audio_buf* audioBuffer, int32_t audioOffset, int32_t audioScale);

static void ConvertToMono(audio_buf* stereo,
                          audio_buf* mono,
                          uint32_t stereoStartIdx,
                          uint32_t monoStartIndex);

int main()
{
    BoardInit();

    /* Model object creation and initialisation. */
    arm::app::MicroNetKwsModel model;
    if (!model.Init(arm::app::tensorArena,
                    sizeof(arm::app::tensorArena),
                    arm::app::kws::GetModelPointer(),
                    arm::app::kws::GetModelLen())) {
        printf_err("Failed to initialise model\n");
        return 1;
    }

    constexpr int minTensorDims = static_cast<int>(
        (arm::app::MicroNetKwsModel::ms_inputRowsIdx > arm::app::MicroNetKwsModel::ms_inputColsIdx)
            ? arm::app::MicroNetKwsModel::ms_inputRowsIdx
            : arm::app::MicroNetKwsModel::ms_inputColsIdx);

    const auto mfccFrameLength = 640;
    const auto mfccFrameStride = 320;
    const auto scoreThreshold  = 0.7;

    /* Get Input and Output tensors for pre/post processing. */
    TfLiteTensor* inputTensor  = model.GetInputTensor(0);
    TfLiteTensor* outputTensor = model.GetOutputTensor(0);
    if (!inputTensor->dims) {
        printf_err("Invalid input tensor dims\n");
        return 1;
    } else if (inputTensor->dims->size < minTensorDims) {
        printf_err("Input tensor dimension should be >= %d\n", minTensorDims);
        return 1;
    }

    /* Get input shape for feature extraction. */
    TfLiteIntArray* inputShape     = model.GetInputShape(0);
    const uint32_t numMfccFeatures = inputShape->data[arm::app::MicroNetKwsModel::ms_inputColsIdx];
    const uint32_t numMfccFrames   = inputShape->data[arm::app::MicroNetKwsModel::ms_inputRowsIdx];

    /* We expect to be sampling 1 second worth of data at a time.
     * NOTE: This is only used for time stamp calculation. */
    const float secondsPerSample = 1.0 / arm::app::audio::MicroNetKwsMFCC::ms_defaultSamplingFreq;

    /* Classifier object for results */
    arm::app::Classifier classifier;

    /* Object to hold label strings. */
    std::vector<std::string> labels;

    /* Declare a container to hold results from across the whole audio clip. */
    std::vector<arm::app::kws::KwsResult> finalResults;

    /* Object to hold classification results */
    std::vector<arm::app::ClassificationResult> singleInfResult;

    /* Populate the labels here. */
    GetLabelsVector(labels);

    /* Set up pre and post-processing. */
    arm::app::KwsPreProcess preProcess = arm::app::KwsPreProcess(
        inputTensor, numMfccFeatures, numMfccFrames, mfccFrameLength, mfccFrameStride);

    arm::app::KwsPostProcess postProcess =
        arm::app::KwsPostProcess(outputTensor, classifier, labels, singleInfResult);

    /* Creating a sliding window through the whole audio clip. */
    auto audioDataSlider =
        arm::app::audio::SlidingWindow<const int16_t>(static_cast<int16_t*>(arm::app::monoBuf.data),
                                                      arm::app::monoBuf.n_elements,
                                                      preProcess.m_audioDataWindowSize,
                                                      preProcess.m_audioDataStride);

    AudioUtils audio{};
    audio.AudioInit(&arm::app::dmaBuf);
    audio.StartAudioRecording();

    PlotUtils plot{};
    uint32_t inferenceCount{0};
    std::string lastValidKeywordDetected{};

    constexpr uint32_t scaleOffsetResetFreq = 5;
    uint32_t captureCount                   = 0;
    int32_t audioGain                       = 0;
    int32_t audioOffset                     = 0;

    while (true) {

        audioDataSlider.Reset();

        while (!audio.IsAudioAvailable()) {
            __WFI();
        }
        audio.StopAudioRecording();

        if (0 == captureCount++ % scaleOffsetResetFreq) {
            audioOffset = CalculateOffset(&arm::app::dmaBuf);
            audioGain = CalculateScale(&arm::app::dmaBuf);
        }

        ApplyGainAndOffset(&arm::app::dmaBuf, audioOffset, audioGain);

        /* Copy over second half of previous audio buffer to the beginning */
        memcpy(arm::app::monoBuf.data,
               (void*)((uint8_t*)arm::app::monoBuf.data + arm::app::monoBuf.n_bytes / 2),
               arm::app::monoBuf.n_bytes / 2);

        if (audio.IsStereo()) {
            /* Populate the second half of the mono buffer from the freshly captured audio */
            ConvertToMono(&arm::app::dmaBuf, &arm::app::monoBuf, 0, arm::app::monoBuf.n_elements / 2);
        } else {
            memcpy((void*)((uint8_t*)arm::app::monoBuf.data + arm::app::monoBuf.n_bytes / 2),
                  arm::app::dmaBuf.data,
                  arm::app::monoBuf.n_bytes / 2);
        }

        plot.PlotWaveform(static_cast<int16_t*>(arm::app::monoBuf.data),
                          arm::app::monoBuf.n_elements);

        /* Restart audio capture */
        audio.SetAudioEmpty();
        audio.StartAudioRecording();

        while (audioDataSlider.HasNext()) {
            const int16_t* inferenceWindow = audioDataSlider.Next();

            /* The first window does not have cache ready. */
            preProcess.m_audioWindowIndex = audioDataSlider.Index();

            /* Run the pre-processing, inference and post-processing. */
            if (!preProcess.DoPreProcess(
                    inferenceWindow, arm::app::audio::MicroNetKwsMFCC::ms_defaultSamplingFreq)) {
                printf_err("Pre-processing failed.");
                return 1;
            }

            info("Inference #: %" PRIu32 "\n", ++inferenceCount);

            if (!model.RunInference()) {
                printf_err("Inference failed.");
                return 2;
            }

            if (!postProcess.DoPostProcess()) {
                printf_err("Post-processing failed.");
                return 3;
            }

            /* Add results from this window to our final results vector. */
            finalResults.emplace_back(arm::app::kws::KwsResult(
                singleInfResult,
                audioDataSlider.Index() * secondsPerSample * preProcess.m_audioDataStride,
                audioDataSlider.Index(),
                scoreThreshold));

        } /* while (audioDataSlider.HasNext()) */

        for (const auto& result : finalResults) {

            std::string topKeyword{"<none>"};
            float score = 0.f;
            if (!result.m_resultVec.empty()) {
                topKeyword = result.m_resultVec[0].m_label;
                score      = result.m_resultVec[0].m_normalisedVal;

                if (topKeyword != "<none>" && topKeyword != "_unknown_") {

                    if (lastValidKeywordDetected != topKeyword) {

                        /* Update last keyword. */
                        lastValidKeywordDetected = topKeyword;

                        info("Detected: %s; Prob: %0.2f\n", topKeyword.c_str(), score);
                        plot.ClearStringLine(9);
                        std::string dispStr = " Last Keyword: " + topKeyword;
                        plot.DisplayStringAtLine(9, dispStr);
                    }
                }
            }
        }

        finalResults.clear();
    }

    return 0;
}

static void ConvertToMono(audio_buf* stereo,
                          audio_buf* mono,
                          uint32_t stereoStartIdx,
                          uint32_t monoStartIndex)
{
    int16_t* pIn     = ((int16_t*)stereo->data) + stereoStartIdx;
    int16_t* pOut    = ((int16_t*)mono->data) + monoStartIndex;
    uint32_t sizeIn  = stereo->n_elements - stereoStartIdx;
    uint32_t sizeOut = mono->n_elements - monoStartIndex;

    for (int i = 0, j = 0; j < sizeIn && i < sizeOut; ++i, j += 2, pIn += 2) {
        *pOut++ = ((pIn[0] >> 1) + (pIn[1] >> 1));
    }
}

static int32_t CalculateOffset(audio_buf* audioBuffer)
{
    int16_t audioMean = 0;
    arm_mean_q15((int16_t*)audioBuffer->data, audioBuffer->n_elements, &audioMean);
    return static_cast<int32_t>(0 - audioMean);
}

static int32_t CalculateScale(audio_buf* audioBuffer)
{
    /* Define the desired signal span to scale our input signal to. It can be based on
     * the training data set, or close to std::numeric_limits<int16_t>::max()/2; */
    constexpr int32_t desirableSignalSpan = 18000;

    /* Maximum scaling factor. A factor bigger than this may amplify noise which can
     * lead to false detections. */
    constexpr int32_t maxScale = 25;

    int16_t audioMin = 0;
    arm_min_no_idx_q15((int16_t*)audioBuffer->data, audioBuffer->n_elements, &audioMin);

    int16_t audioMax = 0;
    arm_max_no_idx_q15((int16_t*)audioBuffer->data, audioBuffer->n_elements, &audioMax);

    int32_t audioScale = desirableSignalSpan / (audioMax - audioMin);

    /* We don't want random silence to be amplified too much; we limit
     * the gain */
    if (audioScale > maxScale) {
        audioScale = maxScale;
    } else if (audioScale < 1) {
        audioScale = 1;
    }

    return audioScale;
}

static void ApplyGainAndOffset(audio_buf* audioBuffer, int32_t audioOffset, int32_t audioScale)
{
    debug("Scale: %d; Offset: %d\n", audioScale, audioOffset);
    int16_t* buf = static_cast<int16_t*>(audioBuffer->data);

    /* Apply offset first and then gain */
    for (uint32_t i = 0; i < audioBuffer->n_elements; ++i) {
        auto& sample         = buf[i];
        int32_t modified_val = (static_cast<int32_t>(sample) + audioOffset) * audioScale;

        /* Clip the high end */
        modified_val = std::min<int32_t>(modified_val,
                                         static_cast<int32_t>(std::numeric_limits<int16_t>::max()));

        /* Clip the low end */
        modified_val = std::max<int32_t>(modified_val,
                                         static_cast<int32_t>(std::numeric_limits<int16_t>::min()));

        sample = static_cast<int16_t>(modified_val);
    }
}
