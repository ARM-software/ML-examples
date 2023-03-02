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

#ifndef GENERATED_AUDIOCLIPS_H
#define GENERATED_AUDIOCLIPS_H

#include <cstdint>
#include <stddef.h>

#define NUMBER_OF_FILES (1U)

extern const int16_t audio0[16000];

const char* get_filename(const uint32_t idx);
const int16_t* get_audio_array(const uint32_t idx);
uint32_t get_audio_array_size(const uint32_t idx);

#endif /* GENERATED_AUDIOCLIPS_H */