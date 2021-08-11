/*
 * Copyright 2019 The TensorFlow Authors. All Rights Reserved.
 * Copyright (c) 2019-2021 Arm Limited. All rights reserved.
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

#include <stdint.h>

void init_lcd();

void display_image_rgb888(int x_dim, int y_dim, const int8_t* image_data,
                          int x_loc, int y_loc);

void display_image_rgb565(int x_dim, int y_dim, const uint8_t* image_data,
                          int x_loc, int y_loc);

void print_prediction(const char* prediction);

void print_confidence(int8_t max_score);

