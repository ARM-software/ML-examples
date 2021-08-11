/*
 * Copyright (c) 2019-2021 Arm Limited. All rights reserved.
 * Copyright 2019 The TensorFlow Authors. All Rights Reserved.
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

#define CHANNEL_SIZE 1024
#define R_CHANNEL_OFFSET 0
#define G_CHANNEL_OFFSET (CHANNEL_SIZE)
#define B_CHANNEL_OFFSET (CHANNEL_SIZE * 2)
#define IMAGE_BYTES 3072
#define LABEL_BYTES 1
#define ENTRY_BYTES (IMAGE_BYTES + LABEL_BYTES)
#define NUM_OUT_CH 3
#define CNN_IMG_SIZE 32
#define NUM_IN_CH 2
#define IN_IMG_WIDTH 160
#define IN_IMG_HEIGHT 120
