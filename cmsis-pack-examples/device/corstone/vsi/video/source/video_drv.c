/*
 * Copyright (c) 2023-2024 Arm Limited. All rights reserved.
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
#include "video_drv.h"
#ifdef _RTE_
#include "RTE_Components.h"
#endif
#include CMSIS_device_header
#include "arm_vsi.h"

// Video channel definitions
#ifndef VIDEO_INPUT_CHANNELS
#define VIDEO_INPUT_CHANNELS    1
#endif
#if    (VIDEO_INPUT_CHANNELS > 2)
#error "Maximum 2 Video Input channels are supported!"
#endif
#ifndef VIDEO_OUTPUT_CHANNELS
#define VIDEO_OUTPUT_CHANNELS   1
#endif
#if    (VIDEO_OUTPUT_CHANNELS > 2)
#error "Maximum 2 Video Output channels are supported!"
#endif

// Video peripheral definitions
#define VideoI0                 ARM_VSI4                // Video Input channel 0 access struct
#define VideoI0_IRQn            ARM_VSI4_IRQn           // Video Input channel 0 Interrupt number
#define VideoI0_Handler         ARM_VSI4_Handler        // Video Input channel 0 Interrupt handler
#define VideoI1                 ARM_VSI5                // Video Input channel 1 access struct
#define VideoI1_IRQn            ARM_VSI5_IRQn           // Video Input channel 1 Interrupt number
#define VideoI1_Handler         ARM_VSI5_Handler        // Video Input channel 1 Interrupt handler
#define VideoO0                 ARM_VSI6                // Video Output channel 0 access struct
#define VideoO0_IRQn            ARM_VSI6_IRQn           // Video Output channel 0 Interrupt number
#define VideoO0_Handler         ARM_VSI6_Handler        // Video Output channel 0 Interrupt handler
#define VideoO1                 ARM_VSI7                // Video Output channel 1 access struct
#define VideoO1_IRQn            ARM_VSI7_IRQn           // Video Output channel 1 Interrupt number
#define VideoO1_Handler         ARM_VSI7_Handler        // Video Output channel 1 Interrupt handler

// Video Peripheral registers
#define Reg_MODE                Regs[0]  // Mode: 0=Input, 1=Output
#define Reg_CONTROL             Regs[1]  // Control: enable, continuos, flush
#define Reg_STATUS              Regs[2]  // Status: active, buf_empty, buf_full, overflow, underflow, eos
#define Reg_FILENAME_LEN        Regs[3]  // Filename length
#define Reg_FILENAME_CHAR       Regs[4]  // Filename character
#define Reg_FILENAME_VALID      Regs[5]  // Filename valid flag
#define Reg_FRAME_WIDTH         Regs[6]  // Requested frame width
#define Reg_FRAME_HEIGHT        Regs[7]  // Requested frame height
#define Reg_COLOR_FORMAT        Regs[8]  // Color format
#define Reg_FRAME_RATE          Regs[9]  // Frame rate
#define Reg_FRAME_INDEX         Regs[10] // Frame index
#define Reg_FRAME_COUNT         Regs[11] // Frame count
#define Reg_FRAME_COUNT_MAX     Regs[12] // Frame count maximum

// Video MODE register defintions
#define Reg_MODE_IO_Pos                 0U
#define Reg_MODE_IO_Msk                 (1UL << Reg_MODE_IO_Pos)
#define Reg_MODE_Input                  (0UL << Reg_MODE_IO_Pos)
#define Reg_MODE_Output                 (1UL << Reg_MODE_IO_Pos)

// Video CONTROL register definitions
#define Reg_CONTROL_ENABLE_Pos          0U
#define Reg_CONTROL_ENABLE_Msk          (1UL << Reg_CONTROL_ENABLE_Pos)
#define Reg_CONTROL_CONTINUOS_Pos       1U
#define Reg_CONTROL_CONTINUOS_Msk       (1UL << Reg_CONTROL_CONTINUOS_Pos)
#define Reg_CONTROL_BUF_FLUSH_Pos       2U
#define Reg_CONTROL_BUF_FLUSH_Msk       (1UL << Reg_CONTROL_BUF_FLUSH_Pos)

// Video STATUS register definitions
#define Reg_STATUS_ACTIVE_Pos           0U
#define Reg_STATUS_ACTIVE_Msk           (1UL << Reg_STATUS_ACTIVE_Pos)
#define Reg_STATUS_BUF_EMPTY_Pos        1U
#define Reg_STATUS_BUF_EMPTY_Msk        (1UL << Reg_STATUS_BUF_EMPTY_Pos)
#define Reg_STATUS_BUF_FULL_Pos         2U
#define Reg_STATUS_BUF_FULL_Msk         (1UL << Reg_STATUS_BUF_FULL_Pos)
#define Reg_STATUS_OVERFLOW_Pos         3U
#define Reg_STATUS_OVERFLOW_Msk         (1UL << Reg_STATUS_OVERFLOW_Pos)
#define Reg_STATUS_UNDERFLOW_Pos        4U
#define Reg_STATUS_UNDERFLOW_Msk        (1UL << Reg_STATUS_UNDERFLOW_Pos)
#define Reg_STATUS_EOS_Pos              5U
#define Reg_STATUS_EOS_Msk              (1UL << Reg_STATUS_EOS_Pos)

// IRQ Status register definitions
#define Reg_IRQ_Status_FRAME_Pos        0U
#define Reg_IRQ_Status_FRAME_Msk        (1UL << Reg_IRQ_Status_FRAME_Pos)
#define Reg_IRQ_Status_OVERFLOW_Pos     1U
#define Reg_IRQ_Status_OVERFLOW_Msk     (1UL << Reg_IRQ_Status_OVERFLOW_Pos)
#define Reg_IRQ_Status_UNDERFLOW_Pos    2U
#define Reg_IRQ_Status_UNDERFLOW_Msk    (1UL << Reg_IRQ_Status_UNDERFLOW_Pos)
#define Reg_IRQ_Status_EOS_Pos          3U
#define Reg_IRQ_Status_EOS_Msk          (1UL << Reg_IRQ_Status_EOS_Pos)

#define Reg_IRQ_Status_Msk              Reg_IRQ_Status_FRAME_Msk     | \
                                        Reg_IRQ_Status_OVERFLOW_Msk  | \
                                        Reg_IRQ_Status_UNDERFLOW_Msk | \
                                        Reg_IRQ_Status_EOS_Msk

// Video peripheral access structure
static ARM_VSI_Type * const pVideo[4] = { VideoI0, VideoO0, VideoI1, VideoO1 };

// Driver State
static uint8_t  Initialized = 0U;
static uint8_t  Configured[4] = { 0U, 0U, 0U, 0U };

// Event Callback
static VideoDrv_Event_t CB_Event = NULL;

// Video Interrupt Handler
static void Video_Handler (uint32_t channel) {
  uint32_t irq_status;
  uint32_t event;

  irq_status = pVideo[channel]->IRQ.Status;
  pVideo[channel]->IRQ.Clear = irq_status;
  __DSB();
  __ISB();

  event = 0U;
  if (irq_status & Reg_IRQ_Status_FRAME_Msk) {
    event |= VIDEO_DRV_EVENT_FRAME;
  }
  if (irq_status & Reg_IRQ_Status_OVERFLOW_Msk) {
    event |= VIDEO_DRV_EVENT_OVERFLOW;
  }
  if (irq_status & Reg_IRQ_Status_UNDERFLOW_Msk) {
    event |= VIDEO_DRV_EVENT_UNDERFLOW;
  }
  if (irq_status & Reg_IRQ_Status_EOS_Msk) {
    event |= VIDEO_DRV_EVENT_EOS;
  }

  if (CB_Event != NULL) {
    CB_Event(channel, event);
  }
}

// Video channel 0 Interrupt Handler
#if (VIDEO_INPUT_CHANNELS >= 1)
void VideoI0_Handler (void);
void VideoI0_Handler (void) {
  Video_Handler(0U);
}
#endif

// Video channel 1 Interrupt Handler
#if (VIDEO_OUTPUT_CHANNELS >= 1)
void VideoO0_Handler (void);
void VideoO0_Handler (void) {
  Video_Handler(1U);
}
#endif

// Video channel 2 Interrupt Handler
#if (VIDEO_INPUT_CHANNELS >= 2)
void VideoI1_Handler (void);
void VideoI1_Handler (void) {
  Video_Handler(2U);
}
#endif

// Video channel 3 Interrupt Handler
#if (VIDEO_OUTPUT_CHANNELS >= 2)
void VideoO1_Handler (void);
void VideoO1_Handler (void) {
  Video_Handler(3U);
}
#endif

// Initialize Video Interface
int32_t VideoDrv_Initialize (VideoDrv_Event_t cb_event) {

  CB_Event = cb_event;

  // Initialize Video Input channel 0
  #if (VIDEO_INPUT_CHANNELS >= 1)
    VideoI0->Timer.Control = 0U;
    VideoI0->DMA.Control   = 0U;
    VideoI0->IRQ.Clear     = Reg_IRQ_Status_Msk;
    VideoI0->IRQ.Enable    = Reg_IRQ_Status_Msk;
    VideoI0->Reg_MODE      = Reg_MODE_Input;
    VideoI0->Reg_CONTROL   = 0U;
//  NVIC_EnableIRQ(VideoI0_IRQn);
    NVIC->ISER[(((uint32_t)VideoI0_IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)VideoI0_IRQn) & 0x1FUL));
  #endif
  Configured[0] = 0U;

  // Initialize Video Output channel 0
  #if (VIDEO_OUTPUT_CHANNELS >= 1)
    VideoO0->Timer.Control = 0U;
    VideoO0->DMA.Control   = 0U;
    VideoO0->IRQ.Clear     = Reg_IRQ_Status_Msk;
    VideoO0->IRQ.Enable    = Reg_IRQ_Status_Msk;
    VideoO0->Reg_MODE      = Reg_MODE_Output;
    VideoO0->Reg_CONTROL   = 0U;
//  NVIC_EnableIRQ(VideoO0_IRQn);
    NVIC->ISER[(((uint32_t)VideoO0_IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)VideoO0_IRQn) & 0x1FUL));
  #endif
  Configured[1] = 0U;

  // Initialize Video Input channel 1
  #if (VIDEO_INPUT_CHANNELS >= 2)
    VideoI1->Timer.Control = 0U;
    VideoI1->DMA.Control   = 0U;
    VideoI1->IRQ.Clear     = Reg_IRQ_Status_Msk;
    VideoI1->IRQ.Enable    = Reg_IRQ_Status_Msk;
    VideoI1->Reg_MODE      = Reg_MODE_Input;
    VideoI1->Reg_CONTROL   = 0U;
//  NVIC_EnableIRQ(VideoI1_IRQn);
    NVIC->ISER[(((uint32_t)VideoI1_IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)VideoI1_IRQn) & 0x1FUL));
  #endif
  Configured[2] = 0U;

  // Initialize Video Output channel 1
  #if (VIDEO_OUTPUT_CHANNELS >= 2)
    VideoO1->Timer.Control = 0U;
    VideoO1->DMA.Control   = 0U;
    VideoO1->IRQ.Clear     = Reg_IRQ_Status_Msk;
    VideoO1->IRQ.Enable    = Reg_IRQ_Status_Msk;
    VideoO1->Reg_MODE      = Reg_MODE_Output;
    VideoO1->Reg_CONTROL   = 0U;
//  NVIC_EnableIRQ(VideoO1_IRQn);
    NVIC->ISER[(((uint32_t)VideoO1_IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)VideoO1_IRQn) & 0x1FUL));
  #endif
  Configured[3] = 0U;

  __DSB();
  __ISB();

  Initialized = 1U;

  return VIDEO_DRV_OK;
}

// De-initialize Video Interface
int32_t VideoDrv_Uninitialize (void) {

  // De-initialize Video Input channel 0
  #if (VIDEO_INPUT_CHANNELS >= 1)
//  NVIC_DisableIRQ(VideoI0_IRQn);
    NVIC->ICER[(((uint32_t)VideoI0_IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)VideoI0_IRQn) & 0x1FUL));
    VideoI0->Timer.Control = 0U;
    VideoI0->DMA.Control   = 0U;
    VideoI0->IRQ.Clear     = Reg_IRQ_Status_Msk;
    VideoI0->IRQ.Enable    = 0U;
    VideoI0->Reg_CONTROL   = 0U;
  #endif

  // De-initialize Video Output channel 0
  #if (VIDEO_OUTPUT_CHANNELS >= 1)
//  NVIC_DisableIRQ(VideoO0_IRQn);
    NVIC->ICER[(((uint32_t)VideoO0_IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)VideoO0_IRQn) & 0x1FUL));
    VideoO0->Timer.Control = 0U;
    VideoO0->DMA.Control   = 0U;
    VideoO0->IRQ.Clear     = Reg_IRQ_Status_Msk;
    VideoO0->IRQ.Enable    = 0U;
    VideoO0->Reg_CONTROL   = 0U;
  #endif

  // De-initialize Video Input channel 1
  #if (VIDEO_INPUT_CHANNELS >= 2)
//  NVIC_DisableIRQ(VideoI1_IRQn);
    NVIC->ICER[(((uint32_t)VideoI1_IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)VideoI1_IRQn) & 0x1FUL));
    VideoI1->Timer.Control = 0U;
    VideoI1->DMA.Control   = 0U;
    VideoI1->IRQ.Clear     = Reg_IRQ_Status_Msk;
    VideoI1->IRQ.Enable    = 0U;
    VideoI1->Reg_CONTROL   = 0U;
  #endif

  // De-initialize Video Output channel 1
  #if (VIDEO_OUTPUT_CHANNELS >= 2)
//  NVIC_DisableIRQ(VideoO1_IRQn);
    NVIC->ICER[(((uint32_t)VideoO1_IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)VideoO1_IRQn) & 0x1FUL));
    VideoO1->Timer.Control = 0U;
    VideoO1->DMA.Control   = 0U;
    VideoO1->IRQ.Clear     = Reg_IRQ_Status_Msk;
    VideoO1->IRQ.Enable    = 0U;
    VideoO1->Reg_CONTROL   = 0U;
  #endif

  __DSB();
  __ISB();

  Initialized = 0U;

  return VIDEO_DRV_OK;
}

// Set Video Interface file
int32_t VideoDrv_SetFile (uint32_t channel, const char *name) {
  const char    *p;
        uint32_t n;

  if ((((channel & 1U) == 0U) && ((channel >> 1) >= VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) >= VIDEO_OUTPUT_CHANNELS)) ||
      (name == NULL)) {
    return VIDEO_DRV_ERROR_PARAMETER;
  }

  if (Initialized == 0U) {
    return VIDEO_DRV_ERROR;
  }

  if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_ACTIVE_Msk) != 0U) {
    return VIDEO_DRV_ERROR;
  }

  // Register Video filename
  n = strlen(name);
  pVideo[channel]->Reg_FILENAME_LEN = n;
  for (p = name; n != 0U; n--) {
    pVideo[channel]->Reg_FILENAME_CHAR = *p++;
  }
  if (pVideo[channel]->Reg_FILENAME_VALID == 0U) {
    return VIDEO_DRV_ERROR;
  }

  return VIDEO_DRV_OK;
}

// Configure Video Interface
int32_t VideoDrv_Configure (uint32_t channel, uint32_t frame_width, uint32_t frame_height, uint32_t color_format, uint32_t frame_rate) {
  uint32_t pixel_size;
  uint32_t block_size;

  if ((((channel & 1U) == 0U) && ((channel >> 1) >= VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) >= VIDEO_OUTPUT_CHANNELS)) ||
      (frame_width  == 0U) ||
      (frame_height == 0U) ||
      (frame_rate   == 0U) ||
      (color_format <= VIDEO_DRV_COLOR_FORMAT_BEGIN) ||
      (color_format >= VIDEO_DRV_COLOR_FORMAT_END)) {
    return VIDEO_DRV_ERROR_PARAMETER;
  }

  switch (color_format) {
    case VIDEO_DRV_COLOR_GRAYSCALE8:
      pixel_size = 8U;
      break;
    case VIDEO_DRV_COLOR_YUV420:
      pixel_size = 12U;
      break;
    case VIDEO_DRV_COLOR_BGR565:
      pixel_size = 16U;
      break;
    case VIDEO_DRV_COLOR_RGB888:
    case VIDEO_DRV_COLOR_NV12:
    case VIDEO_DRV_COLOR_NV21:
      pixel_size = 24U;
      break;
    default:
      return VIDEO_DRV_ERROR_PARAMETER;
  }

  block_size = (((frame_width * frame_height) * pixel_size) + 7U) / 8U;
  block_size = (block_size + 3U) & ~3U;

  if (Initialized == 0U) {
    return VIDEO_DRV_ERROR;
  }

  if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_ACTIVE_Msk) != 0U) {
    return VIDEO_DRV_ERROR;
  }

  pVideo[channel]->Reg_FRAME_WIDTH  = frame_width;
  pVideo[channel]->Reg_FRAME_HEIGHT = frame_height;
  pVideo[channel]->Reg_COLOR_FORMAT = color_format;
  pVideo[channel]->Reg_FRAME_RATE   = frame_rate;
  pVideo[channel]->Timer.Interval   = 1000000U / frame_rate;
  pVideo[channel]->DMA.BlockSize    = block_size;

  Configured[channel] = 1U;

  return VIDEO_DRV_OK;
}

// Set Video Interface buffer
int32_t VideoDrv_SetBuf (uint32_t channel, void *buf, uint32_t buf_size) {
  uint32_t block_num;

  if ((((channel & 1U) == 0U) && ((channel >> 1) >= VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) >= VIDEO_OUTPUT_CHANNELS)) ||
      (buf      == NULL) ||
      (buf_size == 0U)) {
    return VIDEO_DRV_ERROR_PARAMETER;
  }

  if ((Initialized         == 0U) ||
      (Configured[channel] == 0U)) {
    return VIDEO_DRV_ERROR;
  }

  if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_ACTIVE_Msk) != 0U) {
    return VIDEO_DRV_ERROR;
  }

  block_num = buf_size / pVideo[channel]->DMA.BlockSize;
  if (block_num == 0U) {
    return VIDEO_DRV_ERROR;
  }

  pVideo[channel]->Reg_FRAME_COUNT_MAX = block_num;
  pVideo[channel]->DMA.BlockNum        = block_num;

  pVideo[channel]->DMA.Address = (uint32_t)buf;

  Configured[channel] = 2U;

  return VIDEO_DRV_OK;
}

// Flush Video Interface buffer
int32_t VideoDrv_FlushBuf (uint32_t channel) {

  if ((((channel & 1U) == 0U) && ((channel >> 1) >= VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) >= VIDEO_OUTPUT_CHANNELS))) {
    return VIDEO_DRV_ERROR_PARAMETER;
  }

  if (Initialized == 0U) {
    return VIDEO_DRV_ERROR;
  }

  if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_ACTIVE_Msk) != 0U) {
    return VIDEO_DRV_ERROR;
  }

  pVideo[channel]->Reg_CONTROL = Reg_CONTROL_BUF_FLUSH_Msk;

  return VIDEO_DRV_OK;
}

// Start Stream on Video Interface
int32_t VideoDrv_StreamStart (uint32_t channel, uint32_t mode) {
  uint32_t control;

  if ((((channel & 1U) == 0U) && ((channel >> 1) >= VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) >= VIDEO_OUTPUT_CHANNELS)) ||
      (mode > VIDEO_DRV_MODE_CONTINUOS)) {
    return VIDEO_DRV_ERROR_PARAMETER;
  }

  if ((Initialized         == 0U) ||
      (Configured[channel] <  2U)) {
    return VIDEO_DRV_ERROR;
  }

  if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_ACTIVE_Msk) != 0U) {
    return VIDEO_DRV_OK;
  }

  control = Reg_CONTROL_ENABLE_Msk;
  if (mode == VIDEO_DRV_MODE_CONTINUOS) {
    control |= Reg_CONTROL_CONTINUOS_Msk;
  }
  pVideo[channel]->Reg_CONTROL = control;

  if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_ACTIVE_Msk) == 0U) {
    return VIDEO_DRV_ERROR;
  }

  control = ARM_VSI_DMA_Enable_Msk;
  if ((channel & 1U) == 0U) {
    control |= ARM_VSI_DMA_Direction_P2M;
  } else {
    control |= ARM_VSI_DMA_Direction_M2P;
  }
  pVideo[channel]->DMA.Control = control;

  control = ARM_VSI_Timer_Run_Msk      |
            ARM_VSI_Timer_Trig_DMA_Msk |
            ARM_VSI_Timer_Trig_IRQ_Msk;
  if (mode == VIDEO_DRV_MODE_CONTINUOS) {
    control |= ARM_VSI_Timer_Periodic_Msk;
  }
  pVideo[channel]->Timer.Control = control;

  return VIDEO_DRV_OK;
}

// Stop Stream on Video Interface
int32_t VideoDrv_StreamStop (uint32_t channel) {

  if ((((channel & 1U) == 0U) && ((channel >> 1) >= VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) >= VIDEO_OUTPUT_CHANNELS))) {
    return VIDEO_DRV_ERROR_PARAMETER;
  }

  if ((Initialized         == 0U) ||
      (Configured[channel] <  2U)) {
    return VIDEO_DRV_ERROR;
  }

  if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_ACTIVE_Msk) == 0U) {
    return VIDEO_DRV_OK;
  }

  pVideo[channel]->Timer.Control = 0U;
  pVideo[channel]->DMA.Control   = 0U;
  pVideo[channel]->Reg_CONTROL   = 0U;

  return VIDEO_DRV_OK;
}

// Get Video Frame buffer
void *VideoDrv_GetFrameBuf (uint32_t channel) {
  void *frame = NULL;

  if ((((channel & 1U) == 0U) && ((channel >> 1) >= VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) >= VIDEO_OUTPUT_CHANNELS))) {
    return NULL;
  }

  if ((Initialized         == 0U) ||
      (Configured[channel] <  2U)) {
    return NULL;
  }

  if ((pVideo[channel]->Reg_MODE & Reg_MODE_IO_Msk) == Reg_MODE_Input) {
    // Input
    if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_BUF_EMPTY_Msk) != 0U) {
      return NULL;
    }
  } else {
    // Output
    if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_BUF_FULL_Msk) != 0U) {
      return NULL;
    }
  }

  frame = (void *)(pVideo[channel]->DMA.Address + (pVideo[channel]->Reg_FRAME_INDEX * pVideo[channel]->DMA.BlockSize));

  return frame;
}

// Release Video Frame
int32_t VideoDrv_ReleaseFrame (uint32_t channel) {

  if ((((channel & 1U) == 0U) && ((channel >> 1) >= VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) >= VIDEO_OUTPUT_CHANNELS))) {
    return VIDEO_DRV_ERROR_PARAMETER;
  }

  if ((Initialized         == 0U) ||
      (Configured[channel] <  2U)) {
    return VIDEO_DRV_ERROR;
  }

  if ((pVideo[channel]->Reg_MODE & Reg_MODE_IO_Msk) == Reg_MODE_Input) {
    // Input
    if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_BUF_EMPTY_Msk) != 0U) {
      return VIDEO_DRV_ERROR;
    }
  } else {
    // Output
    if ((pVideo[channel]->Reg_STATUS & Reg_STATUS_BUF_FULL_Msk) != 0U) {
      return VIDEO_DRV_ERROR;
    }
  }

  pVideo[channel]->Reg_FRAME_INDEX = 0U;

  return VIDEO_DRV_OK;
}


// Get Video Interface status
VideoDrv_Status_t VideoDrv_GetStatus (uint32_t channel) {
  VideoDrv_Status_t status = { 0U, 0U, 0U, 0U, 0U, 0U, 0U };
  uint32_t          status_reg;

  if ((((channel & 1U) == 0U) && ((channel >> 1) < VIDEO_INPUT_CHANNELS))  ||
      (((channel & 1U) != 0U) && ((channel >> 1) < VIDEO_OUTPUT_CHANNELS))) {
    status_reg       =  pVideo[channel]->Reg_STATUS;
    status.active    = (status_reg & Reg_STATUS_ACTIVE_Msk)    >> Reg_STATUS_ACTIVE_Pos;
    status.buf_empty = (status_reg & Reg_STATUS_BUF_EMPTY_Msk) >> Reg_STATUS_BUF_EMPTY_Pos;
    status.buf_full  = (status_reg & Reg_STATUS_BUF_FULL_Msk)  >> Reg_STATUS_BUF_FULL_Pos;
    status.overflow  = (status_reg & Reg_STATUS_OVERFLOW_Msk)  >> Reg_STATUS_OVERFLOW_Pos;
    status.underflow = (status_reg & Reg_STATUS_UNDERFLOW_Msk) >> Reg_STATUS_UNDERFLOW_Pos;
    status.eos       = (status_reg & Reg_STATUS_EOS_Msk)       >> Reg_STATUS_EOS_Pos;
  }

  return status;
}
