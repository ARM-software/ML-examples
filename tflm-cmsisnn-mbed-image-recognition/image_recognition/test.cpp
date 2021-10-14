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

#include "image_recognition/50_cifar_images.h"
#include "image_recognition/image_recognition_model.h"
#include "image_recognition/util.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_time.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "image_recognition/image_dims.h"

//Exact size is 88 kB but added 2 kB for margin.
constexpr int tensor_arena_size = 90 * 1024;
static uint8_t tensor_arena[tensor_arena_size];

int main(int argc, char** argv) {
  tflite::MicroErrorReporter micro_error_reporter;
  tflite::MicroProfiler micro_profiler;

  const tflite::Model* model = ::tflite::GetModel(image_recognition_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(&micro_error_reporter,
                          "Model provided is schema version %d not equal "
                          "to supported version %d.\n",
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
                                      tensor_arena_size,
                                      &micro_error_reporter,
                                      &micro_profiler
                                      );
  interpreter.AllocateTensors();

  TfLiteTensor* input = interpreter.input(0);

  int num_correct = 0;
  int num_images = 50;
  int32_t ticks_count;
  tflite::InitTimer();

    for (int image_num = 0; image_num < num_images; image_num++) {
      printf("ITERATION %d OF %d\n", image_num + 1, num_images);
      printf("-------------------------------------------\n");

      memset(input->data.uint8, 0, input->bytes);
      memset(input->data.int8, 0, input->bytes);

      uint8_t correct_label = 0;
      correct_label =
          test_batch_bin[image_num * ENTRY_BYTES];
      memcpy(input->data.uint8,
            &test_batch_bin
                [image_num * ENTRY_BYTES + LABEL_BYTES],
            IMAGE_BYTES);
      reshape_cifar_image(input->data.uint8, input->data.int8);
      tflite::StartTimer();
      TfLiteStatus invoke_status = interpreter.Invoke();
      ticks_count = micro_profiler.GetTotalTicks();

      if (invoke_status != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(&micro_error_reporter, "Invoke failed\n");
        break;
      }

      TfLiteTensor* output = interpreter.output(0);
      int guess = get_top_prediction(output->data.int8, 10);

      if (correct_label == guess) {
        num_correct++;
      }
      micro_profiler.Log();
      micro_profiler.ClearEvents();
      printf("-------------------------------------------\n");
      printf("NUMBER OF TICKS = %d\n", ticks_count);
      printf("INFERENCE TIME = %d ms\n", tflite::TicksToMs(ticks_count));
      printf("===========================================\n");
    }

    printf("Predicted %d correct out of 50\nAccuracy = %.2f\n", num_correct, num_correct / 50.0);
}

