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

#include "LcdDisplay.hpp"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

#include "RTE_Components.h"
#include "RTE_Device.h"
#include CMSIS_device_header
#include "log_macros.h"
#include "Driver_Common.h"
#include "Driver_CDC200.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern ARM_DRIVER_CDC200 Driver_CDC200;


#define AtIndex(image, width, height, row, col) ((image) + ((row) * ((width<<1)+width)) + ((col<<1)+col))

static struct lcd_display_params {
    uint8_t*    buffer;
    uint32_t    bytes;
    uint32_t    height;
    uint32_t    width;
    uint32_t    bytes_per_pixel;
} lcd_params;

static bool s_display_error = false;
static void cdc_event_handler(uint32_t event)
{
    if(event & ARM_CDC_DSI_ERROR_EVENT) {
        s_display_error = true;
    }
}

static void clear_display_error(void)
{
    /* Clear the error. */
    NVIC_DisableIRQ((IRQn_Type)MIPI_DSI_IRQ);
    s_display_error = false;
    NVIC_EnableIRQ((IRQn_Type)MIPI_DSI_IRQ);
}

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)


namespace arm {
namespace app {

    /**
     * @brief Rotate an image in-place 90 degrees clockwise
     *
     * @param[in/out] img       Pointer to the image data
     * @param[in] width         Width in pixels.
     * @param[in] height        Height in pixels.
     */
    void RotateClockwise90(uint8_t* img, uint32_t width, uint32_t height)
    {
        uint8_t val[3];

        const uint32_t newWidth        = height;
        const uint32_t newHeight       = width;
        const uint32_t transposeStride = newWidth * 3;

        uint8_t* ptr1 = nullptr;
        uint8_t* ptr2 = nullptr;

        /* Transpose */
        for (uint32_t j = 0; j < height; ++j) {
            ptr1 =
                AtIndex(img, width, height, (j + 1), j); /* Traversing this orthogonally (col-wise)*/
            ptr2 = AtIndex(img, height, width, j, (j + 1)); /* Traversing this normally (row-wise)*/

            for (uint32_t i = j + 1; i < width; ++i) {

                memcpy(val, ptr1, 3);
                memcpy(ptr1, ptr2, 3);
                memcpy(ptr2, val, 3);

                ptr1 += transposeStride;
                ptr2 += 3;
            }
        }

        /* Flip on vertical axis */
        for (uint32_t j = 0; j < newHeight; ++j) {
            ptr1 = AtIndex(img, height, width, j, 0);
            ptr2 = ptr1 + transposeStride - 3;
            for (uint32_t i = 0; i < newWidth / 2; ++i) {
                memcpy(val, ptr1, 3);
                memcpy(ptr1, ptr2, 3);
                memcpy(ptr2, val, 3);

                ptr1 += 3;
                ptr2 -= 3;
            }
        }
    }

    /* Function to test the rotation function */
    void RotationTest()
    {
        uint8_t image[3][3][3] = {
            {{0, 0, 0}, {1, 1, 1}, {2, 2, 2}},
            {{3, 3, 3}, {4, 4, 4}, {5, 5, 5}},
            {{6, 6, 6}, {7, 7, 7}, {8, 8, 8}},
        };

        uint8_t expected[3][3][3] = {
            {{6, 6, 6}, {3, 3, 3}, {0, 0, 0}},
            {{7, 7, 7}, {4, 4, 4}, {1, 1, 1}},
            {{8, 8, 8}, {5, 5, 5}, {2, 2, 2}},
        };

        RotateClockwise90(&image[0][0][0], 3, 3);

        for (uint32_t j = 0; j < 3; ++j) {
            for (uint32_t i = 0; i < 3; ++i) {
                if (image[j][i][0] != expected[j][i][0]) {
                    printf("Test failed\n");
                }
            }
        }
    }

    bool LcdDisplayInit(
        uint8_t* lcdImageBuffer,
        uint32_t lcdWidth,
        uint32_t lcdHeight)
    {
        int32_t ret = Driver_CDC200.Initialize(cdc_event_handler);
        if(ret != ARM_DRIVER_OK) {
            printf_err("Driver_CDC200.Initialize: %d \n", ret);
            return false;
        }

        ret = Driver_CDC200.PowerControl(ARM_POWER_FULL);
        if(ret != ARM_DRIVER_OK) {
            printf_err("Driver_CDC200.PowerControl: %d\n", ret);
            return false;
        }
        ret = Driver_CDC200.Control(CDC200_CONFIGURE_DISPLAY, (uint32_t)lcdImageBuffer);
        if(ret != ARM_DRIVER_OK) {
            printf_err("Driver_CDC200.Control: %d\n", ret);
            return false;
        }
        ret = Driver_CDC200.Start();
        if(ret != ARM_DRIVER_OK) {
            printf_err("Driver_CDC200.Start: %d\n", ret);
            return false;
        }

        lcd_params.buffer = lcdImageBuffer;
        lcd_params.height = lcdHeight;
        lcd_params.width = lcdWidth;
        lcd_params.bytes_per_pixel = RGB_BYTES;
        lcd_params.bytes = lcdHeight * lcdWidth * RGB_BYTES;

        return true;
    }

    bool LcdClearSection(
        uint32_t width,
        uint32_t height,
        uint32_t colOffset,
        uint32_t rowOffset)
    {
        if (rowOffset + height > lcd_params.height) {
            printf("Invalid height/offset params\n");
            return false;
        }
        if (colOffset + width > lcd_params.width) {
            printf("Invalid width/offset params\n");
            return false;
        }
        for (uint32_t rowRgb = 0; rowRgb < height; ++rowRgb, ++rowOffset) {
            uint8_t* lcdPtr = lcd_params.buffer +
                            (lcd_params.width * lcd_params.bytes_per_pixel * rowOffset) +
                            (colOffset * lcd_params.bytes_per_pixel);

            for (uint32_t point = 0; point < width * RGB_BYTES; point++) {
                lcdPtr[point] = 0;
            }
        }
        if (s_display_error) {
            printf_err("Display error detected\n");
            clear_display_error();
        }
        return true;
    }

    bool LcdDisplayImage(
        const uint8_t* rgbData,
        uint32_t rgbWidth,
        uint32_t rgbHeight,
        ColourFormat rgbFormat,
        uint32_t lcdColOffset,
        uint32_t lcdRowOffset)
    {
        uint32_t rowLcd = lcdRowOffset, rowRgb = 0;
        uint32_t colLcd = lcdColOffset, colRgb = 0;

        if (lcdRowOffset + rgbHeight > lcd_params.height) {
            printf("Invalid height/offset params\n");
            return false;
        }
        if (lcdColOffset + rgbWidth > lcd_params.width) {
            printf("Invalid width/offset params\n");
            return false;
        }

        if (rgbFormat == ColourFormat::BGR) {
            for (; rowRgb < rgbHeight; ++rowRgb, ++rowLcd) {
                uint8_t* lcdPtr = lcd_params.buffer +
                                (lcd_params.width * lcd_params.bytes_per_pixel * rowLcd) +
                                (lcdColOffset * lcd_params.bytes_per_pixel);
                const uint8_t* rgb_ptr = rgbData + (rgbWidth * RGB_BYTES * rowRgb);

                for (colRgb = 0; colRgb < rgbWidth * RGB_BYTES; colRgb += 3, lcdPtr += 3) {
                    lcdPtr[2] = *rgb_ptr++;
                    lcdPtr[1] = *rgb_ptr++;
                    lcdPtr[0] = *rgb_ptr++;
                }
            }
        } else if (rgbFormat == ColourFormat::RGB) {
            for (; rowRgb < rgbHeight; ++rowRgb, ++rowLcd) {
                uint8_t* lcdPtr = lcd_params.buffer +
                                (lcd_params.width * lcd_params.bytes_per_pixel * rowLcd) +
                                (lcdColOffset * lcd_params.bytes_per_pixel);
                const uint8_t* rgb_ptr = rgbData + (rgbWidth * RGB_BYTES * rowRgb);

                memcpy(lcdPtr, rgb_ptr, rgbWidth * RGB_BYTES);
            }
        } else {
            printf_err("Unsupported format\n");
            return false;
        }

        if (s_display_error) {
            printf_err("Display error detected\n");
            clear_display_error();
        }

        return true;
    }
} /* namespace app */
} /* namepsace arm */
