/*
 * SPDX-FileCopyrightText: Copyright 2022-2023 Arm Limited and/or its
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
#ifndef UART_STDOUT_H
#define UART_STDOUT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       Initialised the UART block.
 **/
extern int32_t UartStdOutInit(void);

/**
 * @brief       Transmits a character over UART (blocking call).
 * @param[in]   my_ch Character to be transmitted.
 * @return      Character transmitted.
 **/
extern unsigned char UartPutc(unsigned char my_ch);

/**
 * @brief       Receives a character from the UART block (blocking call).
 * @return      Character received.
 **/
extern unsigned char UartGetc(void);

/**
 * @brief       Receives a character from the UART block (NON blocking call).
 * @return      Character received, or 0xFF if no character
 **/
extern unsigned char UartGetcNoBlock(void);

/**
 * @brief       Reads characters from the UART block until a line feed or
 *              carriage return terminates the function. NULL character
 *              also terminates the function, error is returned.
 * @param[out]  lp      Characters read from the UART block.
 * @param[in]   len     Character to be transmitted.
 * @return      true if successful, false otherwise.
 **/
extern bool GetLine(char *lp, unsigned int len);

/**
 * @brief       Terminates UART simulation. This is useful when a Fixed
 *              Virtual Platform's session needs to be gracefully terminated.
 * @param[in]   code Terminating code displayed on the UART before the end of the simulation.
 **/
extern void UartEndSimulation(int code);

#ifdef __cplusplus
}
#endif

#endif /* UART_STDOUT_H */
