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

#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_sdram.h"

#include "BoardAudioUtils.hpp"
#include <assert.h>
#include <cstring>

#if defined(__cplusplus)
extern "C" {
typedef enum { BUFFER_EMPTY = 0, BUFFER_HALF_FULL = 1, BUFFER_FULL = 2 } BufferStateTypeDef;

static BufferStateTypeDef s_bufferState = BUFFER_EMPTY;
static audio_buf* s_stereoBufferDMA     = NULL;
}
#endif /* C */

/*
 * The audio recording works with two ping-pong buffers.
 * The data for each window will be tranfered by the DMA, which sends
 * sends an interrupt after the transfer is completed.
 */
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
    s_bufferState = BUFFER_FULL;
    return;
}

void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
    s_bufferState = BUFFER_HALF_FULL;
    return;
}

/**
 * @brief  Audio IN Error callback function
 */
void BSP_AUDIO_IN_Error_CallBack(void)
{
    assert(0);
}

void AudioUtils::StartAudioRecording()
{
    if (!s_stereoBufferDMA) {
        return;
    }

    if (BSP_AUDIO_IN_Record((uint16_t*)s_stereoBufferDMA->data, s_stereoBufferDMA->n_elements) !=
        AUDIO_OK) {
        printf("BSP_AUDIO_IN_Record error\r\n");
        return;
    }
}

void AudioUtils::StopAudioRecording()
{
    if (BSP_AUDIO_IN_Stop(CODEC_PDWN_SW) != AUDIO_OK) {
        printf("BSP_AUDIO_Stop error\r\n");
    }
}

AudioUtils::AudioUtils()
{
    this->SetSysClock_PLL_HSE_200MHz();
    BSP_SDRAM_Init();
}

bool AudioUtils::AudioInit(audio_buf* audioBufferInStereo)
{
    if (BSP_AUDIO_IN_InitEx(INPUT_DEVICE_DIGITAL_MICROPHONE_2,
                            AUDIO_FREQUENCY_16K,
                            DEFAULT_AUDIO_IN_BIT_RESOLUTION,
                            1) != AUDIO_OK) {
        printf("BSP_AUDIO_IN_Init error\r\n");
        return false;
    }

    s_stereoBufferDMA = audioBufferInStereo;

    /* Start and stop recording as a test */
    this->StartAudioRecording();
    this->StopAudioRecording();

    printf("AUDIO recording configured from digital microphones (U20 & U21)\r\n");
    return true;
}

void AudioUtils::SetVolumeIn(uint8_t vol)
{
    /* Volume level to be set in percentage from 0% to 100%
     * (0 for Mute and 100 for Max volume level). */
    if (BSP_AUDIO_IN_SetVolume(vol) != AUDIO_OK) {
        printf("BSP_AUDIO_IN_SetVolume error\r\n");
    }
}

void AudioUtils::SetVolumeOut(uint8_t vol)
{
    /* Volume level to be set in percentage from 0% to 100%
     * (0 for Mute and 100 for Max volume level). */
    if (BSP_AUDIO_OUT_SetVolume(vol) != AUDIO_OK) {
        printf("BSP_AUDIO_IN_SetVolume error\r\n");
    }
}

bool AudioUtils::IsAudioAvailable()
{
    HAL_NVIC_DisableIRQ(AUDIO_IN_SAIx_DMAx_IRQ);
    auto state = s_bufferState;
    HAL_NVIC_EnableIRQ(AUDIO_IN_SAIx_DMAx_IRQ);
    return (state == BufferStateTypeDef::BUFFER_FULL);
}

void AudioUtils::SetAudioEmpty()
{
    HAL_NVIC_DisableIRQ(AUDIO_IN_SAIx_DMAx_IRQ);
    s_bufferState = BUFFER_EMPTY;
    HAL_NVIC_EnableIRQ(AUDIO_IN_SAIx_DMAx_IRQ);
}

bool AudioUtils::IsStereo()
{
    return true;
}

uint8_t AudioUtils::SetSysClock_PLL_HSE_200MHz()
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    // Enable power clock
    __PWR_CLK_ENABLE();

    // Enable HSE oscillator and activate PLL with HSE as source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON; /* External xtal on OSC_IN/OSC_OUT */

    // Warning: this configuration is for a 25 MHz xtal clock only
    RCC_OscInitStruct.PLL.PLLState  = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM      = 25;            // VCO input clock = 1 MHz (25 MHz / 25)
    RCC_OscInitStruct.PLL.PLLN      = 400;           // VCO output clock = 400 MHz (1 MHz * 400)
    RCC_OscInitStruct.PLL.PLLP      = RCC_PLLP_DIV2; // PLLCLK = 200 MHz (400 MHz / 2)
    RCC_OscInitStruct.PLL.PLLQ      = 8;             // USB clock = 50 MHz (400 MHz / 8)

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return 0; // FAIL
    }

    // Activate the OverDrive to reach the 216 MHz Frequency
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        return 0; // FAIL
    }

    // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
    // clocks dividers
    RCC_ClkInitStruct.ClockType =
        (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK; // 200 MHz
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;         // 200 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;           //  50 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;           // 100 MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) {
        return 0; // FAIL
    }
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_4);
    return 1; // OK
}
