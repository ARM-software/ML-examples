/*
 * SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its
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

#include "CameraCapture.hpp"
#include <cstring>
#include <cstdbool>
#include <array>
#include <functional>

/* Approximations for colour correction */
#define CLAMP_UINT8(x)          (x > 255 ? 255 : x < 0 ? 0 : x)
#define RED_WITH_CCM(r, g, b)   \
    CLAMP_UINT8(((int)r << 1) - ((int)g * 7) / 19 - (((int)b * 14) / 22))
#define GREEN_WITH_CCM(r, g, b) \
    CLAMP_UINT8(((int)g * 13 / 10) - ((int)r >> 1) + (((int)b * 6) / 37))
#define BLUE_WITH_CCM(r, g, b)  \
    CLAMP_UINT8(((int)b * 3) - (((int)r * 5) / 36) - (((int)g << 1) / 3))

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include "RTE_Components.h"
#include CMSIS_device_header
#include "Driver_Camera_Controller.h"
#include "Driver_Common.h"
#include "Driver_GPIO.h"
#include "Driver_PINMUX_AND_PINPAD.h"
#include "log_macros.h"

extern ARM_DRIVER_GPIO Driver_GPIO1;
extern ARM_DRIVER_CAMERA_CONTROLLER Driver_CAMERA0;

static struct arm_camera_status {
    bool frame_complete: 1;
    bool camera_error: 1;
} camera_status;

static void camera_event_cb(uint32_t event)
{
    if(event & ARM_CAMERA_CONTROLLER_EVENT_CAMERA_FRAME_VSYNC_DETECTED) {
        camera_status.frame_complete = true;
    }

    if(event & ARM_CAMERA_CONTROLLER_EVENT_ERR_CAMERA_FIFO_OVERRUN) {
        camera_status.camera_error = true;
    }

    if(event & ARM_CAMERA_CONTROLLER_EVENT_ERR_CAMERA_FIFO_UNDERRUN) {
        camera_status.camera_error = true;
    }

    if(event & ARM_CAMERA_CONTROLLER_EVENT_MIPI_CSI2_ERROR) {
        camera_status.camera_error = true;
    }
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

__attribute__((noreturn)) static void CameraErrorLoop(const char* errorStr)
{
    printf_err("%s\n", errorStr);
    while(true) {
        Driver_GPIO1.SetValue(PIN_NUMBER_14, GPIO_PIN_OUTPUT_STATE_LOW);
        PMU_delay_loop_us(300000);
        Driver_GPIO1.SetValue(PIN_NUMBER_14, GPIO_PIN_OUTPUT_STATE_HIGH);
        PMU_delay_loop_us(300000);
    }
}

int arm::app::CameraCaptureInit(ARM_CAMERA_RESOLUTION resolution)
{
    if (0 != Driver_CAMERA0.Initialize(resolution, camera_event_cb)) {
        CameraErrorLoop("Camera initialisation failed.\n");
    }

    if (0 != Driver_CAMERA0.PowerControl(ARM_POWER_FULL)) {
        CameraErrorLoop("Camera power up failed.\n");
    }

    if (0 != Driver_CAMERA0.Control(CAMERA_SENSOR_CONFIGURE, resolution)) {
        CameraErrorLoop("Camera configuration failed.\n");
    }

    info("Camera initialised.\n");
    Driver_GPIO1.SetValue(PIN_NUMBER_14, GPIO_PIN_OUTPUT_STATE_HIGH);
    return 0;
}

static inline void CameraStatusReset()
{
    NVIC_DisableIRQ((IRQn_Type) CAMERA0_IRQ);
    camera_status.frame_complete = false;
    camera_status.camera_error = false;
    NVIC_EnableIRQ((IRQn_Type) CAMERA0_IRQ);
}

int arm::app::CameraCaptureStart(uint8_t* rawImage)
{
    CameraStatusReset();

    /* NOTE: This is a blocking call at the moment; doesn't need to be.
     *       It slows down the whole pipeline considerably. */
    Driver_CAMERA0.CaptureFrame(rawImage);
    return 0;
}

void arm::app::CameraCaptureWaitForFrame()
{
    while (camera_status.frame_complete != true) {
        __WFI();
    }

    if (camera_status.camera_error) {
        printf_err("Camera error detected!\n");
    }
}

/**
 * @brief   Populates the destination RGB pixel values from source expecting
 *          a BGGR tile pattern.
 * @param[in]   pSrc        Source pointer for raw image.
 * @param[out]  pDst        Starting address for the RGB image pixel to be
 *                          populated.
 * @param[in]   rawImgStep  Bytes to jump to the next row in the raw image.
 */
static inline void PopulateRGBFromBGGR(const uint8_t* pSrc,
                                       uint8_t* pDst,
                                       const uint32_t rawImgStep)
{
    int32_t b = pSrc[0];
    int32_t g = (pSrc[1] + pSrc[rawImgStep]) >> 1;
    int32_t r = pSrc[rawImgStep + 1];

    pDst[0] = RED_WITH_CCM(r,g,b);
    pDst[1] = GREEN_WITH_CCM(r,g,b);
    pDst[2] = BLUE_WITH_CCM(r,g,b);
}

/**
 * @brief   Populates the destination RGB pixel values from source expecting
 *          a GBRG tile pattern.
 * @param[in]   pSrc        Source pointer for raw image.
 * @param[out]  pDst        Starting address for the RGB image pixel to be
 *                          populated.
 * @param[in]   rawImgStep  Bytes to jump to the next row in the raw image.
 */
static inline void PopulateRGBFromGBRG(const uint8_t* pSrc,
                                       uint8_t* pDst,
                                       const uint32_t rawImgStep)
{
    int32_t g = (pSrc[0] + pSrc[rawImgStep + 1]) >> 1;
    int32_t b = pSrc[1];
    int32_t r = pSrc[rawImgStep];

    pDst[0] = RED_WITH_CCM(r,g,b);
    pDst[1] = GREEN_WITH_CCM(r,g,b);
    pDst[2] = BLUE_WITH_CCM(r,g,b);
}

/**
 * @brief   Populates the destination RGB pixel values from source expecting
 *          a GRBG tile pattern.
 * @param[in]   pSrc        Source pointer for raw image.
 * @param[out]  pDst        Starting address for the RGB image pixel to be
 *                          populated.
 * @param[in]   rawImgStep  Bytes to jump to the next row in the raw image.
 */
static inline void PopulateRGBFromGRBG(const uint8_t* pSrc,
                                       uint8_t* pDst,
                                       const uint32_t rawImgStep)
{
    int32_t g = (pSrc[0] + pSrc[rawImgStep + 1]) >> 1;
    int32_t r = pSrc[1];
    int32_t b = pSrc[rawImgStep];

    pDst[0] = RED_WITH_CCM(r,g,b);
    pDst[1] = GREEN_WITH_CCM(r,g,b);
    pDst[2] = BLUE_WITH_CCM(r,g,b);
}

/**
 * @brief   Populates the destination RGB pixel values from source expecting
 *          a RGGB tile pattern.
 * @param[in]   pSrc        Source pointer for raw image.
 * @param[out]  pDst        Starting address for the RGB image pixel to be
 *                          populated.
 * @param[in]   rawImgStep  Bytes to jump to the next row in the raw image.
 */
static inline void PopulateRGBFromRGGB(const uint8_t* pSrc,
                                       uint8_t* pDst,
                                       const uint32_t rawImgStep)
{
    int32_t r = pSrc[0];
    int32_t g = (pSrc[1] + pSrc[rawImgStep]) >> 1;
    int32_t b = pSrc[rawImgStep + 1];

    pDst[0] = RED_WITH_CCM(r,g,b);
    pDst[1] = GREEN_WITH_CCM(r,g,b);
    pDst[2] = BLUE_WITH_CCM(r,g,b);
}

/** Give a name to the function signature that can populate parts of the RGB image */
typedef std::function<void(const uint8_t*, uint8_t*, const uint32_t)> DebayerRowPopulateFunction;

/**
 * @brief   Gets the starting tile bayer tile pattern given the original bayer
 *          pattern and the offsets in the raw image.
 * @param[in]   format  Original RAW bayer format.
 * @param[in]   offsetX X-axis offset for the raw image.
 * @param[in]   offsetY Y-axis offset for the raw image.
 * @return      Tile pattern at the given offsets expressed as `ColourFilter`.
 */
static inline arm::app::ColourFilter GetStartingTilePattern(
    const arm::app::ColourFilter format,
    const uint32_t offsetX,
    const uint32_t offsetY)
{
    using namespace arm::app;

    /** Get the indication for how many odd offsets are there and what's the pattern  */
    const uint32_t oddOffsetsScore = ((offsetX & 1) + ((offsetY & 1)<<1)) & 0x3 ;

    ColourFilter startingPattern{ColourFilter::Invalid};

    switch (format) {
        case ColourFilter::BGGR:
            switch (oddOffsetsScore) {
                case 0: startingPattern = ColourFilter::BGGR; break;
                case 1: startingPattern = ColourFilter::GBRG; break;
                case 2: startingPattern = ColourFilter::GRBG; break;
                case 3: startingPattern = ColourFilter::RGGB; break;
                default: startingPattern = ColourFilter::Invalid;
            }
            break;

        case ColourFilter::GBRG:
            switch (oddOffsetsScore) {
                case 0: startingPattern = ColourFilter::GBRG; break;
                case 1: startingPattern = ColourFilter::BGGR; break;
                case 2: startingPattern = ColourFilter::RGGB; break;
                case 3: startingPattern = ColourFilter::GRBG; break;
                default: startingPattern = ColourFilter::Invalid;
            }
            break;
        break;

        case ColourFilter::GRBG:
            switch (oddOffsetsScore) {
                case 0: startingPattern = ColourFilter::GRBG; break;
                case 1: startingPattern = ColourFilter::RGGB; break;
                case 2: startingPattern = ColourFilter::BGGR; break;
                case 3: startingPattern = ColourFilter::GBRG; break;
                default: startingPattern = ColourFilter::Invalid;
            }
            break;
        break;

        case ColourFilter::RGGB:
            switch (oddOffsetsScore) {
                case 0: startingPattern = ColourFilter::RGGB; break;
                case 1: startingPattern = ColourFilter::GRBG; break;
                case 2: startingPattern = ColourFilter::GBRG; break;
                case 3: startingPattern = ColourFilter::BGGR; break;
                default: startingPattern = ColourFilter::Invalid;
            }
            break;

        default:
            startingPattern = ColourFilter::Invalid;
    }

    return startingPattern;
}

/**
 * @brief Get the order in which debayering functions need to be called.
 * @param[in]   tilePattern             RAW image tile pattern at starting
 *                                      pixel.
 * @param[out]   deyaringFunctionArray  Array of functions to be populated.
 * @return true if successful, false otherwise.
 */
static bool GetDebayeringFunctionOrder(
    const arm::app::ColourFilter tilePattern,
    std::array<DebayerRowPopulateFunction, 4>& deyaringFunctionArray)
{
    switch (tilePattern) {
        case arm::app::ColourFilter::BGGR:
            deyaringFunctionArray[0] = PopulateRGBFromBGGR;
            deyaringFunctionArray[1] = PopulateRGBFromGBRG;
            deyaringFunctionArray[2] = PopulateRGBFromGRBG;
            deyaringFunctionArray[3] = PopulateRGBFromRGGB;
            break;
        case arm::app::ColourFilter::GBRG:
            deyaringFunctionArray[0] = PopulateRGBFromGBRG;
            deyaringFunctionArray[1] = PopulateRGBFromBGGR;
            deyaringFunctionArray[2] = PopulateRGBFromRGGB;
            deyaringFunctionArray[3] = PopulateRGBFromGRBG;
            break;
        case arm::app::ColourFilter::GRBG:
            deyaringFunctionArray[0] = PopulateRGBFromGRBG;
            deyaringFunctionArray[1] = PopulateRGBFromRGGB;
            deyaringFunctionArray[2] = PopulateRGBFromBGGR;
            deyaringFunctionArray[3] = PopulateRGBFromGBRG;
            break;
        case arm::app::ColourFilter::RGGB:
            deyaringFunctionArray[0] = PopulateRGBFromRGGB;
            deyaringFunctionArray[1] = PopulateRGBFromGRBG;
            deyaringFunctionArray[2] = PopulateRGBFromGBRG;
            deyaringFunctionArray[3] = PopulateRGBFromBGGR;
            break;
        default:
            return false;
    }

    return true;
}

bool arm::app::CropAndDebayer(
    const uint8_t* rawImgData,
    uint32_t rawImgWidth,
    uint32_t rawImgHeight,
    uint32_t rawImgCropOffsetX,
    uint32_t rawImgCropOffsetY,
    uint8_t* rgbImgData,
    uint32_t rgbImgWidth,
    uint32_t rgbImgHeight,
    ColourFilter bayerFormat)
{
    const uint32_t rawImgStep = rawImgWidth;
    const uint32_t rgbImgStep = rgbImgWidth * 3;

    /* Infer the tile pattern at which we will begin based on offsets. */
    arm::app::ColourFilter startingPattern =
        GetStartingTilePattern(bayerFormat,
                               rawImgCropOffsetX,
                               rawImgCropOffsetY);

    if (arm::app::ColourFilter::Invalid == startingPattern) {
        printf_err("Invalid bayer pattern\n");
        return false;
    }

    /* Get the order in which the row populating functions will be called. */
    std::array<DebayerRowPopulateFunction, 4> functionArray;
    if (!GetDebayeringFunctionOrder(startingPattern, functionArray)) {
        printf_err("Failed to get debayering function sequence\n");
        return false;
    }

    /* Traverse through the raw and populate the RGB image. */
    for (uint32_t j = 0; j < rgbImgHeight; j += 2) {

        const uint8_t *pSrc = rawImgData + rawImgCropOffsetX +
                             (rawImgStep * (rawImgCropOffsetY + j));
        uint8_t* pDst = rgbImgData + (rgbImgStep * j);


        for (uint32_t i = 0; i < rgbImgWidth; i += 2) {

            functionArray[0](pSrc, pDst, rawImgStep);
            ++pSrc;
            pDst += 3;

            functionArray[1](pSrc, pDst, rawImgStep);
            ++pSrc;
            pDst += 3;
        }

        pSrc = rawImgData + rawImgCropOffsetX +
               (rawImgStep * (rawImgCropOffsetY + j + 1));
        pDst = rgbImgData + (rgbImgStep * (j + 1));

        for (uint32_t i = 0; i < rgbImgWidth; i += 2) {

            functionArray[2](pSrc, pDst, rawImgStep);
            ++pSrc;
            pDst += 3;

            functionArray[3](pSrc, pDst, rawImgStep);
            ++pSrc;
            pDst += 3;
        }
    }

    return true;
}
