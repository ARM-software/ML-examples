/*
 * Copyright (c) 2022, Arm Limited and affiliates.
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

#include "uart_stdout.h"

static UART_HandleTypeDef s_uart;

void UartStdOutInit(void)
{
    UART_InitTypeDef* init = &s_uart.Init;
    init->BaudRate         = 115200;
    init->WordLength       = UART_WORDLENGTH_8B;
    init->Parity           = 0;
    init->StopBits         = 1;
    init->HwFlowCtl        = UART_HWCONTROL_NONE;
    init->Mode             = UART_MODE_TX;
    init->OverSampling     = UART_OVERSAMPLING_16;
    init->OneBitSampling   = UART_ONEBIT_SAMPLING_DISABLED;

    BSP_COM_Init(COM1, &s_uart);
}

UART_HandleTypeDef* GetUartHandle(void)
{
    return &s_uart;
}
