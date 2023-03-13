/*
 * SPDX-FileCopyrightText: Copyright 2022 Arm Limited and/or its
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

#include "BufAttributes.hpp"

#include <string>
#include <vector>

static const char* labelsVec[] LABELS_ATTRIBUTE = {
    "down",
    "go",
    "left",
    "no",
    "off",
    "on",
    "right",
    "stop",
    "up",
    "yes",
    "_silence_",
    "_unknown_",
};

bool GetLabelsVector(std::vector<std::string>& labels)
{
    constexpr size_t labelsSz = 12;
    labels.clear();

    if (!labelsSz) {
        return false;
    }

    labels.reserve(labelsSz);

    for (size_t i = 0; i < labelsSz; ++i) {
        labels.emplace_back(labelsVec[i]);
    }

    return true;
}
