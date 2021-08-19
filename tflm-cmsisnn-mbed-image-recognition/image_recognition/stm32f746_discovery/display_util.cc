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
#include <stdio.h>
#include "image_recognition/stm32f746_discovery/display_util.h"
#include "BSP/Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_lcd.h"

extern "C" {
// defined in stm32746g_discovery_camera.c
extern DCMI_HandleTypeDef hDcmiHandler;
void DCMI_IRQHandler(void) { HAL_DCMI_IRQHandler(&hDcmiHandler); }
void DMA2_Stream1_IRQHandler(void) {
  HAL_DMA_IRQHandler(hDcmiHandler.DMA_Handle);
}
}

static char lcd_output_string[50];


void init_lcd() {
  BSP_LCD_Init();

  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));

  BSP_LCD_DisplayOn();

  BSP_LCD_SelectLayer(0);
  BSP_LCD_Clear(LCD_COLOR_BLACK);

  BSP_LCD_SelectLayer(1);
  BSP_LCD_Clear(LCD_COLOR_BLACK);

  BSP_LCD_SetFont(&LCD_DEFAULT_FONT);

  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);

  BSP_LCD_Clear(LCD_COLOR_WHITE);
}

void display_image_rgb888(int x_dim, int y_dim, const int8_t* image_data,
                          int x_loc, int y_loc) {
  for (int y = 0; y < y_dim; ++y) {
    for (int x = 0; x < x_dim; ++x, image_data += 3) {
      uint8_t a = 0xFF;
      auto r = image_data[0] + 128;
      auto g = image_data[1] + 128;
      auto b = image_data[2] + 128;
      int pixel = a << 24 | r << 16 | g << 8 | b;
      BSP_LCD_DrawPixel(x_loc + x, y_loc + y, pixel);
    }
  }
}

void display_image_rgb565(int x_dim, int y_dim, const uint8_t* image_data,
                          int x_loc, int y_loc) {
  for (int y = 0; y < y_dim; ++y) {
    for (int x = 0; x < x_dim; ++x, image_data += 2) {
      uint8_t a = 0xFF;
      uint8_t pix_lo = image_data[0];
      uint8_t pix_hi = image_data[1];
      uint8_t r = (0xF8 & pix_hi);
      uint8_t g = ((0x07 & pix_hi) << 5) | ((0xE0 & pix_lo) >> 3);
      uint8_t b = (0x1F & pix_lo) << 3;
      int pixel = a << 24 | r << 16 | g << 8 | b;
      // inverted image, so draw from bottom-right to top-left
      BSP_LCD_DrawPixel(x_loc + (x_dim - x), y_loc + (y_dim - y), pixel);
    }
  }
}

void print_prediction(const char* prediction) {
  // NOLINTNEXTLINE
  sprintf(lcd_output_string, "  Prediction: %s       ", prediction);
  BSP_LCD_DisplayStringAt(0, LINE(8), (uint8_t*)lcd_output_string, LEFT_MODE);
}

void print_confidence(int8_t max_score) {
  // NOLINTNEXTLINE
  sprintf(lcd_output_string, "  Confidence: %.1f%%   ",
          ((max_score + 128) / 255.0) * 100.0);
  BSP_LCD_DisplayStringAt(0, LINE(9), (uint8_t*)lcd_output_string, LEFT_MODE);
}
