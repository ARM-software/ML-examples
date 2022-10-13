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
 * This object detection example is intended to work with the
 * CMSIS pack produced by ml-embedded-eval-kit. The pack consists
 * of platform agnostic end-to-end ML use case API's that can be
 * used to construct ML examples for any target that can support
 * the memory requirements for TensorFlow-Lite-Micro framework and
 * some heap for the API runtime.
 */
#include "BufAttributes.hpp" /* Buffer attributes to be applied */
#include "Classifier.hpp"    /* Classifier for the result */
#include "DetectionResult.hpp"
#include "DetectorPostProcessing.hpp" /* Post Process */
#include "DetectorPreProcessing.hpp"  /* Pre Process */
#include "InputFiles.hpp"             /* Baked-in input (not needed for live data) */
#include "YoloFastestModel.hpp"       /* Model API */

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
    namespace object_detection {
        extern uint8_t* GetModelPointer();
        extern size_t GetModelLen();
    } /* namespace object_detection */
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
    arm::app::YoloFastestModel model;
    if (!model.Init(arm::app::tensorArena,
                    sizeof(arm::app::tensorArena),
                    arm::app::object_detection::GetModelPointer(),
                    arm::app::object_detection::GetModelLen())) {
        printf_err("Failed to initialise model\n");
        return 1;
    }

    auto initialImgIdx = 0;

    TfLiteTensor* inputTensor   = model.GetInputTensor(0);
    TfLiteTensor* outputTensor0 = model.GetOutputTensor(0);
    TfLiteTensor* outputTensor1 = model.GetOutputTensor(1);

    if (!inputTensor->dims) {
        printf_err("Invalid input tensor dims\n");
        return 1;
    } else if (inputTensor->dims->size < 3) {
        printf_err("Input tensor dimension should be >= 3\n");
        return 1;
    }

    TfLiteIntArray* inputShape = model.GetInputShape(0);

    const int inputImgCols = inputShape->data[arm::app::YoloFastestModel::ms_inputColsIdx];
    const int inputImgRows = inputShape->data[arm::app::YoloFastestModel::ms_inputRowsIdx];

    /* Set up pre and post-processing. */
    arm::app::DetectorPreProcess preProcess =
        arm::app::DetectorPreProcess(inputTensor, true, model.IsDataSigned());

    std::vector<arm::app::object_detection::DetectionResult> results;
    const arm::app::object_detection::PostProcessParams postProcessParams{
        inputImgRows,
        inputImgCols,
        arm::app::object_detection::originalImageSize,
        arm::app::object_detection::anchor1,
        arm::app::object_detection::anchor2};
    arm::app::DetectorPostProcess postProcess =
        arm::app::DetectorPostProcess(outputTensor0, outputTensor1, results, postProcessParams);

    /* Strings for presentation/logging. */
    std::string str_inf{"Running inference... "};

    const uint8_t* currImage = get_img_array(0);

    auto dstPtr = static_cast<uint8_t*>(inputTensor->data.uint8);
    const size_t copySz =
        inputTensor->bytes < IMAGE_DATA_SIZE ? inputTensor->bytes : IMAGE_DATA_SIZE;

    /* Run the pre-processing, inference and post-processing. */
    if (!preProcess.DoPreProcess(currImage, copySz)) {
        printf_err("Pre-processing failed.");
        return 1;
    }

    /* Run inference over this image. */
    info("Running inference on image %" PRIu32 " => %s\n", 0, get_filename(0));

    if (!model.RunInference()) {
        printf_err("Inference failed.");
        return 2;
    }

    if (!postProcess.DoPostProcess()) {
        printf_err("Post-processing failed.");
        return 3;
    }

    /* Log the results. */
    for (uint32_t i = 0; i < results.size(); ++i) {
        info("Detection at index %" PRIu32 ", at x-coordinate %" PRIu32 ", y-coordinate %" PRIu32
             ", width %" PRIu32 ", height %" PRIu32 "\n",
             i,
             results[i].m_x0,
             results[i].m_y0,
             results[i].m_w,
             results[i].m_h);
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
