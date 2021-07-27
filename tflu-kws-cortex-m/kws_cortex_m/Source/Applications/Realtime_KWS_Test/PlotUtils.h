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

#ifndef __PLOT_UTILS_H__
#define __PLOT_UTILS_H__

#include <string>
#include <vector>

#define LCD_COLOR_ARM_BLUE ((uint32_t) 0xFF00C1DE)
#define LCD_COLOR_ARM_DARK ((uint32_t) 0xFF333E48)

class PlotUtils {

public:
    PlotUtils(int numMfccFeatures, int numFrames);
    ~PlotUtils() = default;

    void ClearAll();
    void ClearStringLine(int line);
    void DisplayStringAtCentreMode(uint16_t xPos, uint16_t yPos, std::string text);
    void PlotMfcc(std::vector<float>& mfccBuffer, int numMfccFeatures, int numFrames);
    void PlotWaveform(std::vector<int16_t>& audioBuffer, int audioBlockSize, int frameLen, int frameShift);

private:
    int mfccUpdateCounter;
    uint32_t screenSizeX;
    uint32_t screenSizeY;
    std::vector<uint32_t> mfccPlotBuffer;
    std::vector<int> audioPlotBuffer;

    uint32_t CalculateRGB(int min, int max, int value);
};

#endif /* __PLOT_UTILS_H__ */
