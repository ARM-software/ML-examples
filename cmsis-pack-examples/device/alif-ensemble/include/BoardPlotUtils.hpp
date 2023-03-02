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
#ifndef PLOT_UTILS_HPP
#define PLOT_UTILS_HPP

#include <cstdint>
#include <string>

/** Class for plotting/displaying to the on-board LCD on Alif Ensemble Boards */
class PlotUtils {

public:
    /**
     * Constructors and destructors
     */
    PlotUtils();
    ~PlotUtils() = default;

    /**
     *  @brief  Clears the LCD
     */
    void ClearAll();

    /**
     * @brief Clears the string at a specific line
     * @param[in]   line    Line number for the string be be cleared at.
     * @note line number limits depend on the font being used and the
     *       vertical span of the LCD. For default font the number
     *       should be between 0 and 9. For smaller fonts available,
     *       it could be [0-19] or [0-29].
     * @return none
     */
    void ClearStringLine(int line);

    /**
     * @brief Displays the given string at the specified location.
     * @param[in]   line   Line number to display the text at.
     * @param[in]   text   Text to be displayed.
     * @note line number limits depend on the font being used and the
     *       vertical span of the LCD. For default font the number
     *       should be between 0 and 9. For smaller fonts available,
     *       it could be [0-19] or [0-29].
     * @return none
     */
    void DisplayStringAtLine(uint16_t line, std::string& text);

    /**
     * @brief Plots an audio waveform (or any sequence of 16-bit signed integers).
     * @param[in]   data        Pointer to the buffer.
     * @param[in]   nElements   Number of elements in the buffer.
     * @return none
     */
    void PlotWaveform(const int16_t* data, uint32_t nElements);
};

#endif /* PLOT_UTILS_HPP */
