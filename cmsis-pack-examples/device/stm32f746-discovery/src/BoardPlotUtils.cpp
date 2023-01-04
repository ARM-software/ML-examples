/*
 * SPDX-FileCopyrightText: Copyright 2021-2022 Arm Limited and/or its
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
#include "BoardPlotUtils.hpp"

#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"

#include <cstring>
#include <limits>

PlotUtils::PlotUtils()
{
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER);
    BSP_LCD_SetFont(&LCD_DEFAULT_FONT);
    BSP_LCD_Clear(LCD_COLOR_ARM_BLUE);
    BSP_LCD_SetBackColor(LCD_COLOR_ARM_BLUE);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

    m_screenSizeX = BSP_LCD_GetXSize();
    m_screenSizeY = BSP_LCD_GetYSize();

    /* Rectangle for PlotMfcc */
    BSP_LCD_FillRect(0, m_screenSizeY / 3, m_screenSizeX, m_screenSizeY / 3);
    /* Rectangle for PlotWaveform */
    BSP_LCD_FillRect(0, 0, m_screenSizeX, m_screenSizeY / 3);
}

void PlotUtils::ClearAll()
{
    BSP_LCD_Clear(LCD_COLOR_ARM_BLUE);
}

void PlotUtils::ClearStringLine(int line)
{
    BSP_LCD_ClearStringLine(line);
}

void PlotUtils::DisplayStringAtLine(uint16_t line, std::string& text)
{
    BSP_LCD_DisplayStringAtLine(line, (uint8_t*)(text.c_str()));
}

/**
 * @brief Clamps the integer value between the provided min and max
 * @param[in] valueIn   integer value in
 * @param[in] valueMin  minimum value
 * @param[in] valueMax  maximum value
 * @return clamped output.
 */
static inline int ClampAudioMag(int valueIn, int valueMin, int valueMax)
{
    if (valueIn < valueMin) {
        valueIn = valueMin;
    } else if (valueIn > valueMax) {
        valueIn = valueMax;
    }
    return valueIn;
}

void PlotUtils::PlotWaveform(const int16_t* data, uint32_t nElements)
{
    const int stride        = (nElements / m_screenSizeX);
    const int yWaveformSpan = m_screenSizeY * 2 / 3;
    const int yScale        = std::numeric_limits<int16_t>::max() / yWaveformSpan;
    const int yCenter       = m_screenSizeY / 3;
    int currentValue        = yCenter;

    BSP_LCD_FillRect(0, 0, m_screenSizeX, yWaveformSpan);
    BSP_LCD_SetTextColor(LCD_COLOR_ARM_DARK);

    int prevValue = yCenter + static_cast<int>(data[0] / yScale);
    prevValue     = ClampAudioMag(prevValue, 0, yWaveformSpan);

    for (uint32_t i = 1, j = stride; i < m_screenSizeX; ++i, j += stride) {

        currentValue = yCenter + static_cast<int>(data[j] / yScale);
        currentValue = ClampAudioMag(currentValue, 0, yWaveformSpan);
        BSP_LCD_DrawLine(i - 1, prevValue, i, currentValue);
    }

    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
}
