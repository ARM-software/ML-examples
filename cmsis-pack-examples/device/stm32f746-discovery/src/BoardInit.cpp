/*
 * SPDX-FileCopyrightText: Copyright 2021-2023 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BoardInit.hpp"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)
#include "RTE_Components.h"
#include CMSIS_device_header
#include "log_macros.h"
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_cortex.h"
#include "uart_stdout.h"
#include <stdbool.h>

/** @brief SysTick ISR */
__attribute__((used)) void SysTick_Handler(void)
{
    HAL_IncTick();
}

extern SAI_HandleTypeDef haudio_out_sai;
extern SAI_HandleTypeDef haudio_in_sai;
/**
 * @brief This function handles DMA2 Stream 7 interrupt request.
 */
void AUDIO_IN_SAIx_DMAx_IRQHandler(void)
{
    HAL_DMA_IRQHandler(haudio_in_sai.hdmarx);
}

/**
 * @brief  This function handles DMA2 Stream 6 interrupt request.
 */
void AUDIO_OUT_SAIx_DMAx_IRQHandler(void)
{
    HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

/**
 * @brief System clock configuration. Refer to:
 * https://github.com/ARMmbed/mbed-os/blob/a3be10c976c36da222517abc0cb4f81e88ff8552/targets/TARGET_STM/TARGET_STM32F7/TARGET_STM32F746xG/TARGET_DISCO_F746NG/system_clock.c
 */
bool SetSysClock_PLL_HSE(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    // Select HSI as system clock source to allow modification of the PLL configuration
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);

    // Enable HSE oscillator and activate PLL with HSE as source
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON; /* External xtal on OSC_IN/OSC_OUT */

    // Warning: this configuration is for a 25 MHz xtal clock only
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM            = 25;            // VCO input clock = 1 MHz (25 MHz / 25)
    RCC_OscInitStruct.PLL.PLLN            = 432;           // VCO output clock = 432 MHz (1 MHz * 432)
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2; // PLLCLK = 216 MHz (432 MHz / 2)
    RCC_OscInitStruct.PLL.PLLQ            = 9;             // USB clock = 48 MHz (432 MHz / 9) --> OK for USB

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return false;
    }

    // Activate the OverDrive to reach the 216 MHz Frequency
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        return false;
    }

    // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers
    RCC_ClkInitStruct.ClockType      = (RCC_CLOCKTYPE_SYSCLK |
                                        RCC_CLOCKTYPE_HCLK |
                                        RCC_CLOCKTYPE_PCLK1 |
                                        RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK; // 216 MHz
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;         // 216 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;           //  54 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;           // 108 MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) {
        return false;
    }

    return true;
}

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)



/**
 * @brief  Infinite error loop.
 */
static void ErrorLoop()
{
    printf_err("Infinite error loop\n");
    while (1) {
        __NOP();
    }
}

void BoardInit(void)
{
    /* STM32F7xx HAL initialization */
    if (HAL_OK != HAL_Init()) {
        ErrorLoop();
    }

    /* Configure the System clock to have a frequency of 216 MHz */
    if (!SetSysClock_PLL_HSE()) {
        ErrorLoop();
    }

    SystemCoreClockUpdate();

    HAL_SetTickFreq(HAL_TICK_FREQ_100HZ);
    UartStdOutInit();

    /* Enable I and D-Cache */
    SCB_EnableICache();
    SCB_EnableDCache();

    return;
}
