/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 Arm Limited and/or its
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
#include "YoloFastestModel.hpp"       /* Model API */
#include "CameraCapture.hpp"          /* Live camera capture API */
#include "LcdDisplay.hpp"             /* LCD display */
#include "GpioSignal.hpp"             /* GPIO signals to drive LEDs */

/* Platform dependent files */
#include "RTE_Components.h"  /* Provides definition for CMSIS_device_header */
#include CMSIS_device_header /* Gives us IRQ num, base addresses. */
#include "BoardInit.hpp"      /* Board initialisation */
#include "log_macros.h"      /* Logging macros (optional) */

#define CROPPED_IMAGE_WIDTH     192
#define CROPPED_IMAGE_HEIGHT    192
#define CROPPED_IMAGE_SIZE      (CROPPED_IMAGE_WIDTH * CROPPED_IMAGE_HEIGHT * 3)

namespace arm {
namespace app {
    /* Tensor arena buffer */
    static uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;

    /* RGB image buffer - cropped/scaled version of the original + debayered. */
    static uint8_t rgbImage[CROPPED_IMAGE_SIZE] __attribute__((section("rgb_buf"), aligned(16)));

    /* RAW image buffer. */
    static uint8_t rawImage[CAMERA_IMAGE_RAW_SIZE] __attribute__((section("raw_buf"), aligned(16)));

    /* LCD image buffer */
    static uint8_t lcdImage[DIMAGE_Y][DIMAGE_X][LCD_BYTES_PER_PIXEL] __attribute__((section("lcd_buf"), aligned(16)));

    /* Optional getter function for the model pointer and its size. */
    namespace object_detection {
        extern uint8_t* GetModelPointer();
        extern size_t GetModelLen();
    } /* namespace object_detection */
} /* namespace app */
} /* namespace arm */

typedef arm::app::object_detection::DetectionResult OdResults;

/**
 * @brief Draws a boxes in the image using the object detection results vector.
 *
 * @param[out] rgbImage     Pointer to the start of the image.
 * @param[in]  width        Image width.
 * @param[in]  height       Image height.
 * @param[in]  results      Vector of object detection results.
 */
static void DrawDetectionBoxes(uint8_t* rgbImage,
                               const uint32_t imageWidth,
                               const uint32_t imageHeight,
                               const std::vector<OdResults>& results);

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

int main()
{
    /* Initialise the UART module to allow printf related functions (if using retarget) */
    BoardInit();

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

    std::vector<OdResults> results;
    const arm::app::object_detection::PostProcessParams postProcessParams{
        inputImgRows,
        inputImgCols,
        arm::app::object_detection::originalImageSize,
        arm::app::object_detection::anchor1,
        arm::app::object_detection::anchor2};
    arm::app::DetectorPostProcess postProcess =
        arm::app::DetectorPostProcess(outputTensor0, outputTensor1, results, postProcessParams);

    const size_t imgSz = inputTensor->bytes < CROPPED_IMAGE_SIZE ?
                         inputTensor->bytes : CROPPED_IMAGE_SIZE;

    if (0 != arm::app::CameraCaptureInit()) {
        printf_err("Failed to initalise camera\n");
        return 2;
    }

    if (sizeof(arm::app::rgbImage) < imgSz) {
        printf_err("RGB buffer is insufficient\n");
        return 3;
    }

    /* Initalise the LCD  */
    arm::app::LcdDisplayInit(&arm::app::lcdImage[0][0][0], DIMAGE_X, DIMAGE_Y);

    /* LCD initialisation */
    arm::app::GpioSignal statusLED {arm::app::SignalPort::Port12,
                                    arm::app::SignalPin::Port12_LED0_R,
                                    arm::app::SignalDirection::DirectionOutput};

    /* Start the camera */
    if (0 != arm::app::CameraCaptureStart(arm::app::rawImage)) {
        printf_err("Failed to start camera capture\n");
        return 4;
    }

    auto dstPtr = static_cast<uint8_t*>(inputTensor->data.uint8);

    uint32_t imgCount = 0;

    while (true) {
        results.clear();

        arm::app::CameraCaptureWaitForFrame();

        auto debayerState = arm::app::CropAndDebayer(
                                arm::app::rawImage,
                                CAMERA_FRAME_WIDTH,
                                CAMERA_FRAME_HEIGHT,
                                (CAMERA_FRAME_WIDTH - inputImgCols)/2,
                                (CAMERA_FRAME_HEIGHT - inputImgRows)/2,
                                arm::app::rgbImage,
                                inputImgCols,
                                inputImgRows,
                                arm::app::ColourFilter::GRBG);


        if (!debayerState) {
            printf_err("Debayering failed\n");
            return 1;
        }

        if (0 != arm::app::CameraCaptureStart(arm::app::rawImage)) {
            printf_err("Failed to start camera capture\n");
        }

        /* Run the pre-processing, inference and post-processing. */
        if (!preProcess.DoPreProcess(arm::app::rgbImage, imgSz)) {
            printf_err("Pre-processing failed.\n");
            return 1;
        }

        /* Run inference over this image. */
        if (!(imgCount++ & 0xF)) {
            printf("\rImage %" PRIu32 "; ", imgCount);
        }

        statusLED.Send(true);
        if (!model.RunInference()) {
            printf_err("Inference failed.\n");
            statusLED.Send(false);
            return 2;
        }
        statusLED.Send(false);

        if (!postProcess.DoPostProcess()) {
            printf_err("Post-processing failed.\n");
            return 3;
        }

        DrawDetectionBoxes(arm::app::rgbImage, inputImgCols, inputImgRows, results);

        arm::app::LcdDisplayImage(arm::app::rgbImage,
                        inputImgCols,
                        inputImgRows,
                        arm::app::ColourFormat::RGB,
                        (DIMAGE_X - inputImgCols)/2,
                        (DIMAGE_Y - inputImgRows)/2);
    }

    return 0;
}

/**
 * @brief Draws a box in the image using the object detection result object.
 *
 * @param[out] imageData    Pointer to the start of the image.
 * @param[in]  width        Image width.
 * @param[in]  height       Image height.
 * @param[in]  result       Object detection result.
 */
static void DrawBox(uint8_t* imageData,
                    const uint32_t width,
                    const uint32_t height,
                    const OdResults& result)
{
    const auto x = result.m_x0;
    const auto y = result.m_y0;
    const auto w = result.m_w;
    const auto h = result.m_h;

    const uint32_t step = width * 3;
    uint8_t* const imStart = imageData + (y * step) + (x * 3);

    uint8_t* dst_0 = imStart;
    uint8_t* dst_1 = imStart + (h * step);

    for (uint32_t i = 0; i < w; ++i) {
        *dst_0 = 255;
        *dst_1 = 255;

        dst_0 += 3;
        dst_1 += 3;
    }

    dst_0 = imStart;
    dst_1 = imStart + (w * 3);

    for (uint32_t j = 0; j < h; ++j) {
        *dst_0 = 255;
        *dst_1 = 255;

        dst_0 += step;
        dst_1 += step;
    }
}

static void DrawDetectionBoxes(uint8_t* rgbImage,
                               const uint32_t imageWidth,
                               const uint32_t imageHeight,
                               const std::vector<OdResults>& results)
{
    for (const auto& result : results) {
        DrawBox(rgbImage, imageWidth, imageHeight, result);
        printf("Detection :: [%" PRIu32 ", %" PRIu32
                         ", %" PRIu32 ", %" PRIu32 "]\n",
                result.m_x0,
                result.m_y0,
                result.m_w,
                result.m_h);
    }
}
