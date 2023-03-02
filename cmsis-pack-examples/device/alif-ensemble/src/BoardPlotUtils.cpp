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
#include "BoardPlotUtils.hpp"

PlotUtils::PlotUtils()
{}

void PlotUtils::ClearAll()
{}

void PlotUtils::ClearStringLine(int line)
{}

void PlotUtils::DisplayStringAtLine(uint16_t line, std::string& text)
{
    (void)line;
    (void)text;
}

void PlotUtils::PlotWaveform(const int16_t* data, uint32_t nElements)
{
    (void)data;
    (void)nElements;
}
