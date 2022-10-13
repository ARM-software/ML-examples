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

#ifndef _UART_STDOUT_H_
#define _UART_STDOUT_H_

#if __cplusplus
extern "C" {
#endif

void UartStdOutInit(void);
unsigned char UartPutc(unsigned char my_ch);
unsigned char UartGetc(void);
unsigned int GetLine(char *lp, unsigned int len);

#if __cplusplus
}
#endif

#endif
