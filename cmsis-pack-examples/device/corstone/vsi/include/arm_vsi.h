/*
 * Copyright (c) 2021-2022 Arm Limited. All rights reserved.
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

/*
 * Virtual Streaming Interface (VSI)
 */

#ifndef __ARM_VSI_H
#define __ARM_VSI_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __IM
#define __IM  volatile const    /*! Defines 'read only' structure member permissions */
#endif
#ifndef __OM
#define __OM  volatile          /*! Defines 'write only' structure member permissions */
#endif
#ifndef __IOM
#define __IOM volatile          /*! Defines 'read/write' structure member permissions */
#endif

#include <stdint.h>

/// Structure type to access the virtual streaming interface
typedef struct
{
  /// Interrupt Request (IRQ)
  struct {
    __IOM uint32_t Enable;      /*!< (R/W) IRQ Enable */
    __OM  uint32_t Set;         /*!< (-/W) IRQ Set */
    __OM  uint32_t Clear;       /*!< (-/W) IRQ Clear */
    __IM  uint32_t Status;      /*!< (R/-) IRQ Status */
  } IRQ;
  uint32_t reserved1[60];
  /// Time counter with 1MHz input frequency
  struct {
    __IOM uint32_t Control;     /*!< (R/W) Timer Control */
    __IOM uint32_t Interval;    /*!< (R/W) Timer Interval Value (in microseconds) */
    __IM  uint32_t Count;       /*!< (R/-) Timer Overflow Count */
  } Timer;
  uint32_t reserved2[61];
  /// Direct Memory Access (DMA) Controller
  struct {
    __IOM uint32_t Control;     /*!< (R/W) DMA Control */
    __IOM uint32_t Address;     /*!< (R/W) DMA Memory Start Address */
    __IOM uint32_t BlockSize;   /*!< (R/W) DMA Block Size (in bytes, multiple of 4) */
    __IOM uint32_t BlockNum;    /*!< (R/W) DMA Number of Blocks (must be 2^n) */
    __IM  uint32_t BlockIndex;  /*!< (R/-) DMA Block Index */
  } DMA;
  uint32_t reserved3[59];
  __IOM uint32_t Regs[64];      /*!< (R/W) User Registers */
} ARM_VSI_Type;

/* VSI Timer Control Definitions for Timer.Control register */
#define ARM_VSI_Timer_Run_Pos           0U                                      /*!< Timer Control: Run Position */
#define ARM_VSI_Timer_Run_Msk           (1UL << ARM_VSI_Timer_Run_Pos)          /*!< Timer Control: Run Mask */
#define ARM_VSI_Timer_Periodic_Pos      1U                                      /*!< Timer Control: Periodic Position */
#define ARM_VSI_Timer_Periodic_Msk      (1UL << ARM_VSI_Timer_Periodic_Pos)     /*!< Timer Control: Periodic Mask */
#define ARM_VSI_Timer_Trig_IRQ_Pos      2U                                      /*!< Timer Control: Trig_IRQ Position */
#define ARM_VSI_Timer_Trig_IRQ_Msk      (1UL << ARM_VSI_Timer_Trig_IRQ_Pos)     /*!< Timer Control: Trig_IRQ Mask */
#define ARM_VSI_Timer_Trig_DMA_Pos      3U                                      /*!< Timer Control: Trig_DAM Position */
#define ARM_VSI_Timer_Trig_DMA_Msk      (1UL << ARM_VSI_Timer_Trig_DMA_Pos)     /*!< Timer Control: Trig_DMA Mask */

/* VSI DMA Control Definitions for DMA.Control register */
#define ARM_VSI_DMA_Enable_Pos          0U                                      /*!< DMA Control: Enable Position */
#define ARM_VSI_DMA_Enable_Msk          (1UL << ARM_VSI_DMA_Enable_Pos)         /*!< DMA Control: Enable Mask */
#define ARM_VSI_DMA_Direction_Pos       1U                                      /*!< DMA Control: Direction Position */
#define ARM_VSI_DMA_Direction_Msk       (1UL << ARM_VSI_DMA_Direction_Pos)      /*!< DMA Control: Direction Mask */
#define ARM_VSI_DMA_Direction_P2M       (0UL*ARM_VSI_DMA_Direction_Msk)         /*!< DMA Control: Direction P2M */
#define ARM_VSI_DMA_Direction_M2P       (1UL*ARM_VSI_DMA_Direction_Msk)         /*!< DMA Control: Direction M2P */

/* Memory mapping of 8 VSI peripherals */
#define ARM_VSI0_BASE           (0x4FF00000UL)                          /*!< VSI 0 Base Address */
#define ARM_VSI1_BASE           (0x4FF10000UL)                          /*!< VSI 1 Base Address */
#define ARM_VSI2_BASE           (0x4FF20000UL)                          /*!< VSI 2 Base Address */
#define ARM_VSI3_BASE           (0x4FF30000UL)                          /*!< VSI 3 Base Address */
#define ARM_VSI4_BASE           (0x4FF40000UL)                          /*!< VSI 4 Base Address */
#define ARM_VSI5_BASE           (0x4FF50000UL)                          /*!< VSI 5 Base Address */
#define ARM_VSI6_BASE           (0x4FF60000UL)                          /*!< VSI 6 Base Address */
#define ARM_VSI7_BASE           (0x4FF70000UL)                          /*!< VSI 7 Base Address */
#define ARM_VSI0                ((ARM_VSI_Type *)ARM_VSI0_BASE)         /*!< VSI 0 struct */
#define ARM_VSI1                ((ARM_VSI_Type *)ARM_VSI1_BASE)         /*!< VSI 1 struct */
#define ARM_VSI2                ((ARM_VSI_Type *)ARM_VSI2_BASE)         /*!< VSI 2 struct */
#define ARM_VSI3                ((ARM_VSI_Type *)ARM_VSI3_BASE)         /*!< VSI 3 struct */
#define ARM_VSI4                ((ARM_VSI_Type *)ARM_VSI4_BASE)         /*!< VSI 4 struct */
#define ARM_VSI5                ((ARM_VSI_Type *)ARM_VSI5_BASE)         /*!< VSI 5 struct */
#define ARM_VSI6                ((ARM_VSI_Type *)ARM_VSI6_BASE)         /*!< VSI 6 struct */
#define ARM_VSI7                ((ARM_VSI_Type *)ARM_VSI7_BASE)         /*!< VSI 7 struct */

#ifdef __cplusplus
}
#endif

#endif /* __ARM_VSI_H */
