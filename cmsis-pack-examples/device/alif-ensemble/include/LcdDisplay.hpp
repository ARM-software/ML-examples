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
#ifndef LCD_DISPLAY_HPP
#define LCD_DISPLAY_HPP

#include <stdint.h>
#include <stdbool.h>
#include "RTE_Device.h"

#define DIMAGE_X            RTE_PANEL_HACTIVE_TIME
#define DIMAGE_Y            RTE_PANEL_VACTIVE_LINE
#define RGB_BYTES           3

namespace arm {
namespace app {

enum class ColourFormat {
    BGR = 0,
    RGB,
    RAW16,
    RAW12
};

/**
     * @brief Rotate an image in-place 90 degrees clockwise
     *
     * @param[in/out] img       Pointer to the image data
     * @param[in] width         Width in pixels.
     * @param[in] height        Height in pixels.
     */
void RotateClockwise90(uint8_t* img, uint32_t width, uint32_t height);

/**
 * @brief Initialises the LCD Display
 *
 * @param[in] lcdImageBuffer    Buffer for LCD display image.
 * @param[in] lcdWidth          Width of the LCD image buffer image.
 * @param[in] lcdHeight         Height of the LCD image buffer image.
 * @return bol True if successful, false otherwise.
 */
bool LcdDisplayInit(
    uint8_t* lcdImageBuffer,
    uint32_t lcdWidth,
    uint32_t lcdHeight);

/**
 * @brief Populates the LCD frame buffer from a given RGB image. Both images are
 *        expected to be 24 bit depth (RGB888).
 *
 * @param[in] rgbData         RGB image pointer (source).
 * @param[in] rgbWidth        RGB image width.
 * @param[in] rgbHeight       RGB image height.
 * @param[in] rgbFormat       RGB colour format (RGB/BGR)
 * @param[in] lcdColOffset    Starting column of the LCD where RGB image should be placed.
 * @param[in] lcdRowOffset    Starting row of the LCD where RGB image should be placed.
 *
 * @return True if successful, false otherwise.
 */
bool LcdDisplayImage(
    const uint8_t* rgbData,
    uint32_t rgbWidth,
    uint32_t rgbHeight,
    ColourFormat rgbFormat,
    uint32_t lcdColOffset,
    uint32_t lcdRowOffset);

/**
 * @brief Clears the section of the screen.
 *
 * @param[in] width         Section width.
 * @param[in] height        Section height.
 * @param[in] colOffset    Starting column of the section to clear.
 * @param[in] rowOffset    Starting row of the section to clear.
 * @return True if successful, false otherwise.
 */
bool LcdClearSection(
    uint32_t width,
    uint32_t height,
    uint32_t colOffset,
    uint32_t rowOffset);

} /* namespace app */
} /* namepsace arm */

#endif /* LCD_DISPLAY_HPP */
