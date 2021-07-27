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

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/Applications/Realtime_KWS_Test/PlotUtils.h"


#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"




#include <cstring>


PlotUtils::PlotUtils(int numMfccFeatures, int numFrames)
{
    mfccPlotBuffer = std::vector<uint32_t>(numMfccFeatures * numFrames * 10);

    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER);
    BSP_LCD_SetFont(&LCD_DEFAULT_FONT);
    BSP_LCD_Clear(LCD_COLOR_ARM_BLUE);
    BSP_LCD_SetBackColor(LCD_COLOR_ARM_BLUE);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

    screenSizeX = BSP_LCD_GetXSize();
    screenSizeY = BSP_LCD_GetYSize();
    audioPlotBuffer = std::vector<int>(screenSizeX);
    mfccUpdateCounter = 0;

    /* Rectangle for PlotMfcc */
    BSP_LCD_FillRect(0, screenSizeY / 3, screenSizeX, screenSizeY / 3);
    /* Rectangle for PlotWaveform */
    BSP_LCD_FillRect(0, 0, screenSizeX, screenSizeY / 3);
}

void PlotUtils::ClearAll()
{
    BSP_LCD_Clear(LCD_COLOR_ARM_BLUE);
}

void PlotUtils::ClearStringLine(int line)
{
    BSP_LCD_ClearStringLine(line);
}

void PlotUtils::DisplayStringAtCentreMode(uint16_t xPos, uint16_t yPos, std::string text)
{
    BSP_LCD_DisplayStringAt(xPos, yPos, (uint8_t*)(text.c_str()), CENTER_MODE);
}

uint32_t PlotUtils::CalculateRGB(int min, int max, int value)
{
    uint32_t ret = 0xFF000000;
    int mid_point = (min + max) / 2;
    int range = (max - min);
    if (value >= mid_point) {
        uint32_t delta = (value - mid_point) * 512 / range;
        if (delta > 255)
        {
            delta = 255;
        }
        ret = ret | (delta << 16);
        ret = ret | ((255 - delta) << 8);
    }
    else {
        int delta = value * 512 / range;
        if (delta > 255)
        {
            delta = 255;
        }
        ret = ret | (delta << 8);
        ret = ret | (255 - delta);
    }
    return ret;
}

void PlotUtils::PlotMfcc(std::vector<float>& mfccBuffer, int numMfccFeatures, int numFrames)
{
    memcpy(mfccPlotBuffer.data(), mfccPlotBuffer.data() + 2 * numMfccFeatures, 4 * numMfccFeatures * (10 * numFrames - 2));

    int x_step = 1;
    int y_step = 6;

    uint32_t *pBuffer = mfccPlotBuffer.data() + numMfccFeatures * (10 * numFrames - 2);
    int sum = 0;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < numMfccFeatures; j++) {
            int value = mfccBuffer[(numMfccFeatures * (numFrames - 2)) + i * numMfccFeatures + j];
            uint32_t RGB = CalculateRGB(-128, 127, value * 4);
            sum += std::abs(value);
            pBuffer[i * numMfccFeatures + j] = RGB;
        }
    }

    int x_start = (screenSizeX - (numFrames * 10)) / 2;
    x_start = (x_start > 0) ? x_start : 0;
    mfccUpdateCounter++;

    if (mfccUpdateCounter == 10) {
        BSP_LCD_FillRect(0, screenSizeY / 3, screenSizeX, screenSizeY / 3);
        for (int i = 0; i < 10 * numFrames; i++) {
            for (int j = 0; j < numMfccFeatures; j++) {
                for (int x = 0; x < x_step; x++) {
                    for (int y = 0; y < y_step; y++)
                    {
                        BSP_LCD_DrawPixel(x_start + i * x_step + x, 
                                         100 + j * y_step + y, 
                                         mfccPlotBuffer[i * numMfccFeatures + j]);
                    }
                }
            }
        }
        mfccUpdateCounter = 0;
    }
}

void PlotUtils::PlotWaveform(std::vector<int16_t>& audioBuffer, int audioBlockSize, int frameLen, int frameShift)
{
    int stride = (audioBlockSize / screenSizeX);
    int yCenter = screenSizeY / 6;
    int audioMagnitude = yCenter;

    BSP_LCD_FillRect(0, 0, screenSizeX, screenSizeY / 3);
    for (uint32_t i = 0; i < screenSizeX; i++) {
        audioMagnitude = yCenter + (int)(audioBuffer[(frameLen - frameShift) + i * stride] / 8);
        if (audioMagnitude < 0) {
            audioMagnitude = 0;
        }
        if (audioMagnitude > 2 * yCenter) {
            audioMagnitude = 2 * yCenter - 1;
        }
        audioPlotBuffer[i] = audioMagnitude;
    }

    BSP_LCD_SetTextColor(LCD_COLOR_ARM_DARK);

    for (uint32_t i = 0; i < screenSizeX - 1; i++) {
        BSP_LCD_DrawLine(i, audioPlotBuffer[i], i + 1, audioPlotBuffer[i + 1]);
    }

    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
}
