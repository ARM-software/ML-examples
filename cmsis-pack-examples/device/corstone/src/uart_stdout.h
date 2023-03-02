/*
 * SPDX-FileCopyrightText: Copyright 2022 Arm Limited and/or its
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

#if __cplusplus
extern "C" {
#endif

/**
 * @brief UART initialisation - to enable printf output redirection.
 */
void UartStdOutInit(void);

/**
 * @brief Sends a character over UART
 *
 * @param[in] my_ch Character to be sent.
 * @return          Character sent.
 * @note            This is a blocking function.
 */
unsigned char UartPutc(unsigned char my_ch);

/**
 * @brief   Reads a character over UART.
 * @return  Character read.
 * @note    This is a blocking function.
 */
unsigned char UartGetc(void);

#if __cplusplus
}
#endif

#endif /* UART_STDOUT_H */
