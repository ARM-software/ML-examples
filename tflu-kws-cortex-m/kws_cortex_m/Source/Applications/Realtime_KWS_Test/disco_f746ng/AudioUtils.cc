/*
 * Copyright (C) 2021 Arm Limited or its affiliates. All rights reserved.
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

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/Applications/Realtime_KWS_Test/AudioUtils.h"

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/Applications/Realtime_KWS_Test/KWSWrapper.h"
#include "BufAttributes.h"

#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_sdram.h"
#include "mbed_assert.h"

#include <cstring>

extern KWSWrapper* kwsWrapperPtr;

typedef enum
{
    BUFFER_OFFSET_NONE = 0,
    BUFFER_OFFSET_FULL = 1,
} BufferStateTypeDef;


/*
* The audio recording works with two ping-pong buffers.
* The data for each window will be tranfered by the DMA, which sends
* sends an interrupt after the transfer is completed.
*/
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
    const size_t halfBuffInSize = kwsWrapperPtr->audioBufferDMATransferIn.size() / 2;
    const size_t nbrBytesToCopy = kwsWrapperPtr->audioBufferDMATransferIn.size() * sizeof(int16_t) /2;
    if(kwsWrapperPtr->audioUtils->audioRecBufferState == BUFFER_OFFSET_NONE)
    {

        /* copy the new recording data */
        const size_t startIndex = kwsWrapperPtr->currentAudioBufferSize;
        const size_t endIndex = (kwsWrapperPtr->currentAudioBufferSize + halfBuffInSize) < kwsWrapperPtr->audioBufferAcc.size() ?
                                kwsWrapperPtr->currentAudioBufferSize + halfBuffInSize :
                                kwsWrapperPtr->audioBufferAcc.size();

        /*copy latest audio block accumulate buffer */
        memcpy((int16_t *)kwsWrapperPtr->audioBufferAcc.data()+startIndex,
               (int16_t *)kwsWrapperPtr->audioBufferDMATransferIn.data()+halfBuffInSize,
               nbrBytesToCopy);

        kwsWrapperPtr->currentAudioBufferSize = endIndex;

        /* Check if we have all the audio buffer needed to run KWS*/
        if (endIndex == kwsWrapperPtr->audioBufferAcc.size())
        {

            kwsWrapperPtr->audioUtils->audioRecBufferState = BUFFER_OFFSET_FULL;
            kwsWrapperPtr->currentAudioBufferSize = 0;
        }
        else
        {
            kwsWrapperPtr->currentAudioBufferSize = endIndex;
        }
    }

    return;
}

void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
    const size_t halfBuffInSize = kwsWrapperPtr->audioBufferDMATransferIn.size() / 2;
    const size_t nbrBytesToCopy = kwsWrapperPtr->audioBufferDMATransferIn.size() * sizeof(int16_t) /2;
    if(kwsWrapperPtr->audioUtils->audioRecBufferState == BUFFER_OFFSET_NONE)
    {
        /* copy the new recording data */
        const size_t startIndex = kwsWrapperPtr->currentAudioBufferSize;
        const size_t  endIndex = (kwsWrapperPtr->currentAudioBufferSize + halfBuffInSize) < kwsWrapperPtr->audioBufferAcc.size() ?
                                 kwsWrapperPtr->currentAudioBufferSize + halfBuffInSize :
                                 kwsWrapperPtr->audioBufferAcc.size();

        memcpy((int16_t *)kwsWrapperPtr->audioBufferAcc.data()+startIndex,
               (int16_t *)kwsWrapperPtr->audioBufferDMATransferIn.data(),
               nbrBytesToCopy);

        /* Check if we have all the audio buffer needed to run KWS*/
        if (endIndex == kwsWrapperPtr->audioBufferAcc.size())
        {
            kwsWrapperPtr->audioUtils->audioRecBufferState = BUFFER_OFFSET_FULL;
            kwsWrapperPtr->currentAudioBufferSize = 0;
        }
        else
        {
            kwsWrapperPtr->currentAudioBufferSize = endIndex;
        }
    }
    return;
}

/**
 * @brief  Audio IN Error callback function
 */
void BSP_AUDIO_IN_Error_CallBack(void)
{
    MBED_ASSERT(0);
}

void AudioUtils::StartAudioInRecord(uint16_t *audioBufferIn,uint32_t nbrOfBytesIn)
{
    if (BSP_AUDIO_IN_Record(audioBufferIn, nbrOfBytesIn) != AUDIO_OK)
    {
        printf("BSP_AUDIO_IN_Record error\r\n");
    }
}

void AudioUtils::StopAudioInRecord()
{
    if (BSP_AUDIO_IN_Stop(CODEC_PDWN_SW) != AUDIO_OK)
    {
        printf("BSP_AUDIO_Stop error\r\n");
    }
}

AudioUtils::AudioUtils()
{
    this->SetSysClock_PLL_HSE_200MHz();
    BSP_SDRAM_Init();
}

void AudioUtils::AudioInit(uint16_t *audioBufferIn, uint32_t nbrOfBytesIn)
{
    if (BSP_AUDIO_IN_InitEx(INPUT_DEVICE_DIGITAL_MICROPHONE_2,DEFAULT_AUDIO_IN_FREQ, DEFAULT_AUDIO_IN_BIT_RESOLUTION, 1)
        != AUDIO_OK)
    {
        printf("BSP_AUDIO_IN_Init error\r\n");
    }

    /* Start Recording */
    if (BSP_AUDIO_IN_Record((uint16_t *)audioBufferIn, nbrOfBytesIn) != AUDIO_OK)
    {
        printf("BSP_AUDIO_IN_Record error\r\n");
    }

    /* Stop Recording until memory allocation etc for model is complete*/
    if (BSP_AUDIO_IN_Stop(CODEC_PDWN_SW) != AUDIO_OK)
    {
        printf("BSP_AUDIO_IN_Stop error\r\n");
    }

    this->audioRecBufferState = BUFFER_OFFSET_NONE;

    printf("AUDIO recording configured from digital microphones (U20 & U21 components on board) \r\n");
}

void AudioUtils::SetVolumeIn(uint8_t vol)
{
    /* Volume level to be set in percentage from 0% to 100%
     * (0 for Mute and 100 for Max volume level). */
    if (BSP_AUDIO_IN_SetVolume(vol) != AUDIO_OK)
    {
        printf("BSP_AUDIO_IN_SetVolume error\r\n");
    }
}

void AudioUtils::SetVolumeOut(uint8_t vol)
{
    /* Volume level to be set in percentage from 0% to 100%
     * (0 for Mute and 100 for Max volume level). */
    if (BSP_AUDIO_OUT_SetVolume(vol) != AUDIO_OK)
    {
        printf("BSP_AUDIO_IN_SetVolume error\r\n");
    }
}

bool AudioUtils::IsAudioAvailable()
{
    return this->audioRecBufferState == BUFFER_OFFSET_FULL;
}

void AudioUtils::SetAudioEmpty()
{
    kwsWrapperPtr->audioUtils->audioRecBufferState = BUFFER_OFFSET_NONE;
}

void AudioUtils::ConvertToMono(int16_t* audioS, int16_t* audioM, uint32_t nbrBytesStereo)
{
    for (int i = 0, j = 0; j < nbrBytesStereo-2; ++i, j +=2)
    {
        audioM[i] = ((audioS[j] >> 1) + (audioS[j + 1] >> 1));
    }
}

uint8_t AudioUtils::SetSysClock_PLL_HSE_200MHz()
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    // Enable power clock
    __PWR_CLK_ENABLE();

    // Enable HSE oscillator and activate PLL with HSE as source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON; /* External xtal on OSC_IN/OSC_OUT */

    // Warning: this configuration is for a 25 MHz xtal clock only
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;            // VCO input clock = 1 MHz (25 MHz / 25)
    RCC_OscInitStruct.PLL.PLLN = 400;           // VCO output clock = 400 MHz (1 MHz * 400)
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // PLLCLK = 200 MHz (400 MHz / 2)
    RCC_OscInitStruct.PLL.PLLQ = 8;             // USB clock = 50 MHz (400 MHz / 8)

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        return 0; // FAIL
    }

    // Activate the OverDrive to reach the 216 MHz Frequency
    if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
        return 0; // FAIL
    }

    // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
    // clocks dividers
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // 200 MHz
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;        // 200 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;         //  50 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;         // 100 MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
    {
        return 0; // FAIL
    }
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_4);
    return 1; // OK
}