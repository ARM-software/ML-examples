/*
 * SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its
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

#include "BoardAudioUtils.hpp"
#include <assert.h>
#include <cstring>

#if defined(__cplusplus)
extern "C" {
#endif /* C */

#include "RTE_Components.h"
#include "RTE_Device.h"
#include <Driver_PINMUX_AND_PINPAD.h>
#include <Driver_SAI.h>
#include CMSIS_device_header

#include <stdio.h>

#define I2S_ADC       2 /* Audio I2S Controller 2 */
extern ARM_DRIVER_SAI ARM_Driver_SAI_(I2S_ADC);
ARM_DRIVER_SAI*       s_i2s_drv;

typedef struct _audio_capture_state {
    volatile bool capStarted;
    volatile bool capCompleted;
} audio_capture_state;

static volatile audio_capture_state s_cap_state;

static void set_capture_completed(bool val)
{
    NVIC_DisableIRQ((IRQn_Type)I2S2_IRQ);
    s_cap_state.capCompleted = val;
    NVIC_EnableIRQ((IRQn_Type)I2S2_IRQ);
}

static void set_capture_started(bool val)
{
    s_cap_state.capStarted = val;
}

/**
 * @brief Callback routine from the i2s driver.
 *
 * @param[in]  event  Event for which the callback has been called.
 */
static void I2SCallback(uint32_t event)
{
    if (event & ARM_SAI_EVENT_RECEIVE_COMPLETE) {
        s_cap_state.capCompleted = true;
    }
}

static audio_buf* s_stereoBufferDMA     = NULL;

#if defined(__cplusplus)
}
#endif /* C */

static int32_t ConfigureI2SPinMuxPinPad()
{
    int32_t status = 0;

    // Configure P2_1.I2S2_SDI_A
    status |= PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_1, PINMUX_ALTERNATE_FUNCTION_3);
    status |=
        PINPAD_Config(PORT_NUMBER_2,
                      PIN_NUMBER_1,
                      PAD_FUNCTION_DRIVER_DISABLE_STATE_WITH_PULL_DOWN | PAD_FUNCTION_READ_ENABLE);

    /* Configure P2_3.I2S2_SCLK_A */
    status |= PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_3, PINMUX_ALTERNATE_FUNCTION_3);
    status |= PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_3, PAD_FUNCTION_READ_ENABLE);

    /* Configure P2_3.I2S2_WS_A */
    status |= PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_4, PINMUX_ALTERNATE_FUNCTION_2);
    status |= PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_4, PAD_FUNCTION_READ_ENABLE);

    return status;
}

static bool InitializeI2SDriver(void)
{
    int32_t status = 0;
    constexpr uint32_t audioSamplingRate = 16000;
    constexpr uint32_t wlen = 16;

    set_capture_completed(false);
    set_capture_started(false);

    /* Configure pins to their I2S related functions */
    status = ConfigureI2SPinMuxPinPad();
    if (status) {
        printf("I2S pinmux configuration failed\n");
        return false;
    }

    /* Use the I2S as Receiver */
    s_i2s_drv = &ARM_Driver_SAI_(I2S_ADC);

    /* Verify the I2S API version for compatibility */
    ARM_DRIVER_VERSION version = s_i2s_drv->GetVersion();
    printf("I2S API version = %d\n", version.api);

    /* Verify if I2S protocol is supported */
    ARM_SAI_CAPABILITIES cap = s_i2s_drv->GetCapabilities();
    if (!cap.protocol_i2s) {
        printf("I2S is not supported\n");
        return false;
    }

    /* Initializes I2S interface */
    status = s_i2s_drv->Initialize(I2SCallback);
    if (status) {
        printf("I2S Initialize failed status = %d\n", status);
        goto i2sInitializeError;
    }

    /* Enable the power for I2S */
    status = s_i2s_drv->PowerControl(ARM_POWER_FULL);
    if (status) {
        printf("I2S Power failed status = %d\n", status);
        goto i2sPowerError;
    }

    /* Configure I2S Receiver to Asynchronous Master */
    status = s_i2s_drv->Control(ARM_SAI_CONFIGURE_RX | ARM_SAI_MODE_MASTER | ARM_SAI_ASYNCHRONOUS |
                                ARM_SAI_PROTOCOL_I2S | ARM_SAI_DATA_SIZE(wlen),
                                wlen * 2,
                                audioSamplingRate);

    if (status) {
        printf("I2S Control status = %d\n", status);
        goto i2sControlError;
    }
    status = s_i2s_drv->Control(ARM_SAI_CONTROL_RX, 1, 0); // Added here to keep recording going
    return true;

i2sControlError:
    s_i2s_drv->PowerControl(ARM_POWER_OFF);
i2sPowerError:
    s_i2s_drv->Uninitialize();
i2sInitializeError:
    return false;
}

static void UninitializeI2SDriver(void)
{
    /* Uninitialize turns the power off beforehand */
    s_i2s_drv->Uninitialize();
}

void AudioUtils::StartAudioRecording()
{
    if (!s_stereoBufferDMA) {
        return;
    }

    int32_t status = 0;

    /* Enable Receiver */

    set_capture_started(true);
    // status = s_i2s_drv->Control(ARM_SAI_CONTROL_RX, 1, 0);  // Uncomment to start/stop the mic.
    if (status) {
        printf("I2S Control RX start status = %d\n", status);
        return;
    }

    /* Receive data */
    status = s_i2s_drv->Receive(s_stereoBufferDMA->data, s_stereoBufferDMA->n_elements);
    if (status) {
        printf("I2S Receive status = %d\n", status);
        return;
    }

    return;
}

void AudioUtils::StopAudioRecording()
{
    /* Stop the RX */
    int status = 0;

    // status = s_i2s_drv->Control(ARM_SAI_CONTROL_RX, 0, 0);  // Uncomment to start/stop the mic.
    if (status) {
        printf("I2S Control RX stop status = %d\n", status);
        return;
    }

    this->SetAudioEmpty();
}

AudioUtils::AudioUtils()
{}

AudioUtils::~AudioUtils()
{
    UninitializeI2SDriver();
}

bool AudioUtils::AudioInit(audio_buf* audioBufferInStereo)
{
    if (!InitializeI2SDriver()) {
        printf("Failed to initialise audio\n");
        return false;
    }

    s_stereoBufferDMA = audioBufferInStereo;

    /* Start and stop recording as a test */
    this->StartAudioRecording();
    this->StopAudioRecording();

    printf("Audio recording configured.\n");
    return true;
}

bool AudioUtils::IsStereo() const
{
    return true;
}

void AudioUtils::SetVolumeIn(uint8_t vol)
{}

void AudioUtils::SetVolumeOut(uint8_t vol)
{}

bool AudioUtils::IsAudioAvailable()
{
    if (s_cap_state.capStarted) {
        return s_cap_state.capCompleted;
    }

    printf("No audio available\n");
    return false;
}

void AudioUtils::SetAudioEmpty()
{
    set_capture_completed(false);
    set_capture_started(false);
}
