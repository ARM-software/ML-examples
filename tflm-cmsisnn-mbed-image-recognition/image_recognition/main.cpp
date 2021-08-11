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

#include "mbed-os/mbed.h"
#include "mbed-os/targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_hal.h"
#include "image_recognition/image_provider.h"
#include "image_recognition/image_recognition_model.h"
#include "image_recognition/stm32f746_discovery/display_util.h"
#include "image_recognition/stm32f746_discovery/image_util.h"
#include "image_recognition/util.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "image_recognition/image_dims.h"

static uint8_t camera_buffer[NUM_IN_CH * IN_IMG_WIDTH * IN_IMG_HEIGHT]
    __attribute__((aligned(4)));
static const char* labels[] = {"Plane", "Car",  "Bird",  "Cat",  "Deer",
                               "Dog",   "Frog", "Horse", "Ship", "Truck"};

//Exact size is 88 kB but added 2 kB for margin.
constexpr int tensor_arena_size = 90 * 1024;
static uint8_t tensor_arena[tensor_arena_size];

int main(int argc, char** argv) {
  init_lcd();
  HAL_Delay(100);

  tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = &micro_error_reporter;

  if (InitCamera(error_reporter) != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Failed to init camera.");
    return 1;
  }

  const tflite::Model* model = ::tflite::GetModel(image_recognition_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return 1;
  }

  tflite::MicroMutableOpResolver<8> micro_op_resolver;

  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddStridedSlice();
  micro_op_resolver.AddMul();
  micro_op_resolver.AddAdd();
  micro_op_resolver.AddRelu6();
  micro_op_resolver.AddPad();
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddReshape();

  tflite::MicroInterpreter interpreter(model, micro_op_resolver, tensor_arena,
                                       tensor_arena_size, error_reporter);
  interpreter.AllocateTensors();

  while (true) {
    TfLiteTensor* input = interpreter.input(0);

    GetImage(error_reporter, IN_IMG_WIDTH, IN_IMG_HEIGHT, NUM_OUT_CH,
             camera_buffer);
    ResizeConvertImage(error_reporter, IN_IMG_WIDTH, IN_IMG_HEIGHT, NUM_IN_CH,
                       CNN_IMG_SIZE, CNN_IMG_SIZE, NUM_OUT_CH, camera_buffer,
                       input->data.int8);
    display_image_rgb565(IN_IMG_WIDTH, IN_IMG_HEIGHT, camera_buffer, 40, 40);
    display_image_rgb888(CNN_IMG_SIZE, CNN_IMG_SIZE, input->data.int8, 300,
                         100);

    if (input->type != kTfLiteInt8) {
      TF_LITE_REPORT_ERROR(error_reporter, "Wrong input type.");
      break;
    }

    TfLiteStatus invoke_status = interpreter.Invoke();
    if (invoke_status != kTfLiteOk) {
      TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed.");
      break;
    }

    TfLiteTensor* output = interpreter.output(0);

    int top_ind = get_top_prediction(output->data.int8, 10);
    print_prediction(labels[top_ind]);
    print_confidence(output->data.int8[top_ind]);
  }

  return 0;
}
