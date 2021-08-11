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
#include <string.h>
#include <stdlib.h>
#include "image_recognition/image_dims.h"

int get_top_prediction(const int8_t* predictions, int num_categories) {
  int max_score = predictions[0];
  int guess = 0;

  for (int category_index = 1; category_index < num_categories;
       category_index++) {
    const int8_t category_score = predictions[category_index];
    if (category_score > max_score) {
      max_score = category_score;
      guess = category_index;
    }
  }

  return guess;
}

void reshape_cifar_image(uint8_t* image_data, int8_t* signed_image_data) {
  int8_t* temp_data = new int8_t[IMAGE_BYTES];
  memcpy(temp_data, image_data, IMAGE_BYTES);
  int k = 0;
  for (int i = 0; i < CHANNEL_SIZE; i++) {
    int r_ind = R_CHANNEL_OFFSET + i;
    int g_ind = G_CHANNEL_OFFSET + i;
    int b_ind = B_CHANNEL_OFFSET + i;
    signed_image_data[k] = temp_data[r_ind] - 128;
    k++;
    signed_image_data[k] = temp_data[g_ind] - 128;
    k++;
    signed_image_data[k] = temp_data[b_ind] - 128;
    k++;
  }
  delete [] temp_data;
}

