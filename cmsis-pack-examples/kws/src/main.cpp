/*
 * Copyright (c) 2022, Arm Limited and affiliates.
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
#include "AudioUtils.hpp"
#include "BufAttributes.hpp" /* Buffer attributes to be applied */
#include "Classifier.hpp"    /* Classifier for the result */
#include "InputFiles.hpp"    /* Baked-in input (not needed for live data) */
#include "InputFiles.hpp"
#include "KwsProcessing.hpp" /* Pre and Post Process */
#include "KwsResult.hpp"
#include "Labels.hpp" /* Label Data for the model */
#include "MicroNetKwsMfcc.hpp"
#include "MicroNetKwsModel.hpp" /* Model API */

/* Platform dependent files */
#include "SSE300MPS3.h"    /* Gives us IRQ num, base addresses. */
#include "ethosu_driver.h" /* Arm Ethos-U driver header */
#include "log_macros.h"    /* Logging macros (optional) */
#include "uart_stdout.h"   /* UART stdout functionality to enable prints */

#if defined(ETHOS_U_CACHE_BUF_SZ) && (ETHOS_U_CACHE_BUF_SZ > 0)
static uint8_t cache_arena[ETHOS_U_CACHE_BUF_SZ] CACHE_BUF_ATTRIBUTE;
#else  /* defined (ETHOS_U_CACHE_BUF_SZ) && (ETHOS_U_CACHE_BUF_SZ > 0) */
static uint8_t* cache_arena = nullptr;
#endif /* defined (ETHOS_U_CACHE_BUF_SZ) && (ETHOS_U_CACHE_BUF_SZ > 0) */

static uint8_t* get_cache_arena()
{
    return cache_arena;
}

static size_t get_cache_arena_size()
{
#if defined(ETHOS_U_CACHE_BUF_SZ) && (ETHOS_U_CACHE_BUF_SZ > 0)
    return sizeof(cache_arena);
#else  /* defined (ETHOS_U_CACHE_BUF_SZ) && (ETHOS_U_CACHE_BUF_SZ > 0) */
    return 0;
#endif /* defined (ETHOS_U_CACHE_BUF_SZ) && (ETHOS_U_CACHE_BUF_SZ > 0) */
}

/** @brief   Defines the Ethos-U interrupt handler: just a wrapper around the default
 *           implementation. */
static void arm_ethosu_npu_irq_handler(void);

/** @brief  Initialises the NPU IRQ */
static void arm_ethosu_npu_irq_init(void);

/** @brief  Initialises the NPU */
static int arm_ethosu_npu_init(void);

namespace arm {
namespace app {
    /* Tensor arena buffer */
    static uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;

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

int main()
{
    /* Initialise the UART module to allow printf related functions (if using retarget) */
    UartStdOutInit();

    /* Initialise the NPU */
    arm_ethosu_npu_init();

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
        arm::app::audio::SlidingWindow<const int16_t>(get_audio_array(0),
                                                      get_audio_array_size(0),
                                                      preProcess.m_audioDataWindowSize,
                                                      preProcess.m_audioDataStride);

    while (audioDataSlider.HasNext()) {
        const int16_t* inferenceWindow = audioDataSlider.Next();

        /* The first window does not have cache ready. */
        preProcess.m_audioWindowIndex = audioDataSlider.Index();

        info(
            "Inference %zu/%zu\n", audioDataSlider.Index() + 1, audioDataSlider.TotalStrides() + 1);

        /* Run the pre-processing, inference and post-processing. */
        if (!preProcess.DoPreProcess(inferenceWindow,
                                     arm::app::audio::MicroNetKwsMFCC::ms_defaultSamplingFreq)) {
            printf_err("Pre-processing failed.");
            return 1;
        }

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

#if VERIFY_TEST_OUTPUT
        DumpTensor(outputTensor);
#endif /* VERIFY_TEST_OUTPUT */
    }  /* while (audioDataSlider.HasNext()) */

    for (const auto& result : finalResults) {

        std::string topKeyword{"<none>"};
        float score = 0.f;
        if (!result.m_resultVec.empty()) {
            topKeyword = result.m_resultVec[0].m_label;
            score      = result.m_resultVec[0].m_normalisedVal;
        }

        if (result.m_resultVec.empty()) {
            info("For timestamp: %f (inference #: %" PRIu32 "); label: %s; threshold: %f\n",
                 result.m_timeStamp,
                 result.m_inferenceNumber,
                 topKeyword.c_str(),
                 result.m_threshold);
        } else {
            for (uint32_t j = 0; j < result.m_resultVec.size(); ++j) {
                info("For timestamp: %f (inference #: %" PRIu32
                     "); label: %s, score: %f; threshold: %f\n",
                     result.m_timeStamp,
                     result.m_inferenceNumber,
                     result.m_resultVec[j].m_label.c_str(),
                     result.m_resultVec[j].m_normalisedVal,
                     result.m_threshold);
            }
        }
    }

    return 0;
}

struct ethosu_driver ethosu_drv; /* Default Ethos-U device driver */

static void arm_ethosu_npu_irq_handler(void)
{
    /* Call the default interrupt handler from the NPU driver */
    ethosu_irq_handler(&ethosu_drv);
}

static void arm_ethosu_npu_irq_init(void)
{
    const IRQn_Type ethosu_irqnum = (IRQn_Type)ETHOS_U55_IRQn;

    /* Register the EthosU IRQ handler in our vector table.
     * Note, this handler comes from the EthosU driver */
    NVIC_SetVector(ethosu_irqnum, (uint32_t)arm_ethosu_npu_irq_handler);

    /* Enable the IRQ */
    NVIC_EnableIRQ(ethosu_irqnum);

    debug("EthosU IRQ#: %u, Handler: 0x%p\n", ethosu_irqnum, arm_ethosu_npu_irq_handler);
}

static int arm_ethosu_npu_init(void)
{
    int err = 0;

    /* Initialise the IRQ */
    arm_ethosu_npu_irq_init();

    /* Initialise Ethos-U device */
    const void* ethosu_base_address = (void*)(ETHOS_U55_APB_BASE_S);

    if (0 != (err = ethosu_init(&ethosu_drv,         /* Ethos-U driver device pointer */
                                ethosu_base_address, /* Ethos-U NPU's base address. */
                                get_cache_arena(),   /* Pointer to fast mem area - NULL for U55. */
                                get_cache_arena_size(), /* Fast mem region size. */
                                1,                      /* Security enable. */
                                1)))                    /* Privilege enable. */
    {
        printf_err("failed to initialise Ethos-U device\n");
        return err;
    }

    info("Ethos-U device initialised\n");

    /* Get Ethos-U version */
    struct ethosu_driver_version driver_version;
    struct ethosu_hw_info hw_info;

    ethosu_get_driver_version(&driver_version);
    ethosu_get_hw_info(&ethosu_drv, &hw_info);

    info("Ethos-U version info:\n");
    info("\tArch:       v%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
         hw_info.version.arch_major_rev,
         hw_info.version.arch_minor_rev,
         hw_info.version.arch_patch_rev);
    info("\tDriver:     v%" PRIu8 ".%" PRIu8 ".%" PRIu8 "\n",
         driver_version.major,
         driver_version.minor,
         driver_version.patch);
    info("\tMACs/cc:    %" PRIu32 "\n", (uint32_t)(1 << hw_info.cfg.macs_per_cc));
    info("\tCmd stream: v%" PRIu32 "\n", hw_info.cfg.cmd_stream_version);

    return 0;
}
