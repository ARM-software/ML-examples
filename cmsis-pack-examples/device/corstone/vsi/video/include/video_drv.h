/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
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

#ifndef VIDEO_DRV_H
#define VIDEO_DRV_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/* Video Channel */
#define VIDEO_DRV_IN0                   (0UL)       ///< Video Input channel 0
#define VIDEO_DRV_OUT0                  (1UL)       ///< Video Output channel 0
#define VIDEO_DRV_IN1                   (2UL)       ///< Video Input channel 1
#define VIDEO_DRV_OUT1                  (3UL)       ///< Video Output channel 1

/* Video Mode */
#define VIDEO_DRV_MODE_SINGLE           (0UL)       ///< Single frame
#define VIDEO_DRV_MODE_CONTINUOS        (1UL)       ///< Continuos stream

/* Video Color Format */
#define VIDEO_DRV_COLOR_FORMAT_BEGIN    (0UL)       ///< Color format begin
#define VIDEO_DRV_COLOR_GRAYSCALE8      (1UL)       ///< 8 bit grayscale color format
#define VIDEO_DRV_COLOR_RGB888          (2UL)       ///< 24 bit RGB color format
#define VIDEO_DRV_COLOR_BGR565          (3UL)       ///< 16 bit BGR color format
#define VIDEO_DRV_COLOR_YUV420          (4UL)       ///< 12 bit YUV420 color format
#define VIDEO_DRV_COLOR_NV12            (5UL)       ///< 24 bit NV12 color format
#define VIDEO_DRV_COLOR_NV21            (6UL)       ///< 24 bit NV12 color format
#define VIDEO_DRV_COLOR_FORMAT_END      (7UL)       ///< Color format end

/* Video Event */
#define VIDEO_DRV_EVENT_FRAME           (1UL << 0)  ///< Video frame received
#define VIDEO_DRV_EVENT_OVERFLOW        (1UL << 1)  ///< Video buffer overflow
#define VIDEO_DRV_EVENT_UNDERFLOW       (1UL << 2)  ///< Video buffer underflow
#define VIDEO_DRV_EVENT_EOS             (1UL << 3)  ///< Video end of stream

/* Return code */
#define VIDEO_DRV_OK                    (0)         ///< Operation succeeded
#define VIDEO_DRV_ERROR                 (-1)        ///< Unspecified error
#define VIDEO_DRV_ERROR_PARAMETER       (-2)        ///< Parameter error

/// Video Status
typedef struct {
  uint32_t active       :  1;           ///< Video stream active
  uint32_t buf_empty    :  1;           ///< Video buffer empty
  uint32_t buf_full     :  1;           ///< Video buffer full
  uint32_t overflow     :  1;           ///< Video buffer overflow (cleared on GetStatus)
  uint32_t underflow    :  1;           ///< Video buffer underflow (cleared on GetStatus)
  uint32_t eos          :  1;           ///< Video end of stream (cleared on GetStatus)
  uint32_t reserved     : 26;
} VideoDrv_Status_t;

/// \brief       Video Events callback function type.
/// \param[in]   channel        channel number
/// \param[in]   event          events notification mask
/// \return      none
typedef void (*VideoDrv_Event_t) (uint32_t channel, uint32_t event);

/// \brief       Initialize Video Interface.
/// \param[in]   cb_event pointer to \ref VideoDrv_Event_t
/// \return      return code
int32_t VideoDrv_Initialize (VideoDrv_Event_t cb_event);

/// \brief       De-initialize Video Interface.
/// \return      return code
int32_t VideoDrv_Uninitialize (void);

/// \brief       Set Video channel file.
/// \param[in]   channel        channel number
/// \param[in]   name           video filename (pointer to NULL terminated string)
/// \return      return code
int32_t VideoDrv_SetFile (uint32_t channel, const char *name);

/// \brief       Configure Video channel.
/// \param[in]   channel        channel number
/// \param[in]   frame_width    frame width in pixels
/// \param[in]   frame_height   frame height in pixels
/// \param[in]   color_format   pixel color format
/// \param[in]   frame_rate     frame rate (frames per second)
/// \return      return code
int32_t VideoDrv_Configure (uint32_t channel, uint32_t frame_width, uint32_t frame_height, uint32_t color_format, uint32_t frame_rate);

/// \brief       Set Video channel buffer.
/// \param[in]   channel        channel number
/// \param[in]   buf            pointer to buffer for video stream
/// \param[in]   buf_size       video stream buffer size in bytes
/// \return      return code
int32_t VideoDrv_SetBuf (uint32_t channel, void *buf, uint32_t buf_size);

/// \brief       Flush Video channel buffer.
/// \param[in]   channel        channel number
/// \return      return code
int32_t VideoDrv_FlushBuf (uint32_t channel);

/// \brief       Start Video channel stream.
/// \param[in]   channel        channel number
/// \param[in]   mode           stream mode
/// \return      return code
int32_t VideoDrv_StreamStart (uint32_t channel, uint32_t mode);

/// \brief       Stop Video channel stream.
/// \param[in]   channel        channel number
/// \return      return code
int32_t VideoDrv_StreamStop (uint32_t channel);

/// \brief       Get Video channel Frame buffer.
/// \param[in]   channel        channel number
/// \return      pointer to frame buffer
void *VideoDrv_GetFrameBuf (uint32_t channel);

/// \brief       Release Video channel Frame.
/// \param[in]   channel        channel number
/// \return      return code
int32_t VideoDrv_ReleaseFrame (uint32_t channel);

/// \brief       Get Video channel status.
/// \param[in]   channel        channel number
/// \return      \ref VideoDrv_Status_t
VideoDrv_Status_t VideoDrv_GetStatus (uint32_t channel);

#ifdef  __cplusplus
}
#endif

#endif  /* VIDEO_DRV_H */
