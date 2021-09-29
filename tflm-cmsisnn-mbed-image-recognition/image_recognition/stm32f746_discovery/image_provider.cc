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

#include "image_recognition/image_provider.h"
#include "BSP/Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_camera.h"

TfLiteStatus InitCamera(tflite::ErrorReporter* error_reporter) {
  if (BSP_CAMERA_Init(RESOLUTION_R160x120) != CAMERA_OK) {
    TF_LITE_REPORT_ERROR(error_reporter, "Failed to init camera.\n");
    return kTfLiteError;
  }

  return kTfLiteOk;
}

TfLiteStatus GetImage(tflite::ErrorReporter* error_reporter, int frame_width,
                      int frame_height, int channels, uint8_t* frame) {
  (void)error_reporter;
  (void)frame_width;
  (void)frame_height;
  (void)channels;
  BSP_CAMERA_ContinuousStart(frame);
  return kTfLiteOk;
}
