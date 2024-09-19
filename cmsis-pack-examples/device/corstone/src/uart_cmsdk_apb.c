/*
 * SPDX-FileCopyrightText: Copyright 2022, 2024 Arm Limited and/or its
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

/* Basic CMSDK APB UART driver */

#include "uart_config.h"
#include "uart_stdout.h"

/* Platform dependent files */
#include "RTE_Components.h"  /* Provides definition for CMSIS_device_header */
#include CMSIS_device_header /* Gives us IRQ num, base addresses. */

#include <stdint.h>
#include <stdio.h>

#define CNTLQ     0x11
#define CNTLS     0x13
#define DEL       0x7F
#define BACKSPACE 0x08
#define CR        0x0D
#define LF        0x0A
#define ESC       0x1B

#define __IO volatile
#define __I  volatile const
#define __O  volatile

typedef struct {
    __IO uint32_t DATA;  /* Offset: 0x000 (R/W) Data Register    */
    __IO uint32_t STATE; /* Offset: 0x004 (R/W) Status Register  */
    __IO uint32_t CTRL;  /* Offset: 0x008 (R/W) Control Register */
    union {
        __I uint32_t INTSTATUS; /* Offset: 0x00C (R/ ) Interrupt Status Register */
        __O uint32_t INTCLEAR;  /* Offset: 0x00C ( /W) Interrupt Clear Register  */
    };
    __IO uint32_t BAUDDIV; /* Offset: 0x010 (R/W) Baudrate Divider Register */
} CMSDK_UART_TypeDef;

#define CMSDK_UART0_BASE     UART0_BASE_NS
#define CMSDK_UART0          ((CMSDK_UART_TypeDef*)CMSDK_UART0_BASE)
#define CMSDK_UART0_BAUDRATE UART0_BAUDRATE

void UartStdOutInit(void)
{
    CMSDK_UART0->BAUDDIV = SYSTEM_CORE_CLOCK / CMSDK_UART0_BAUDRATE;

    CMSDK_UART0->CTRL = ((1ul << 0) | /* TX enable */
                         (1ul << 1)); /* RX enable */
}

// Output a character
unsigned char UartPutc(unsigned char my_ch)
{
    while ((CMSDK_UART0->STATE & 1))
        ; // Wait if Transmit Holding register is full

    if (my_ch == '\n') {
        CMSDK_UART0->DATA = '\r';
        while ((CMSDK_UART0->STATE & 1))
            ; // Wait if Transmit Holding register is full
    }

    CMSDK_UART0->DATA = my_ch; // write to transmit holding register

    return (my_ch);
}

// Get a character
unsigned char UartGetc(void)
{
    unsigned char my_ch;
    // unsigned int  cnt;

    while ((CMSDK_UART0->STATE & 2) == 0) // Wait if Receive Holding register is empty
        ;

    my_ch = CMSDK_UART0->DATA;

    // Convert CR to LF
    if (my_ch == '\r')
        my_ch = '\n';

    return (my_ch);
}

// Get line from terminal
unsigned int GetLine(char* lp, unsigned int len)
{
    unsigned int cnt = 0;
    char c;

    do {
        c = UartGetc();
        switch (c) {
        case CNTLQ: /* ignore Control S/Q             */
        case CNTLS:
            break;
        case BACKSPACE:
        case DEL:
            if (cnt == 0) {
                break;
            }
            cnt--;          /* decrement count                */
            lp--;           /* and line pointer               */
            UartPutc(0x08); /* echo backspace                 */
            UartPutc(' ');
            UartPutc(0x08);
            fflush(stdout);
            break;
        case ESC:
        case 0:
            *lp = 0; /* ESC - stop editing line        */
            return 0;
        case CR: /* CR - done, stop editing line   */
            *lp = c;
            lp++;  /* increment line pointer         */
            cnt++; /* and count                      */
            c = LF;
            /* fall through */
        default:
            UartPutc(*lp = c); /* echo and store character       */
            fflush(stdout);
            lp++;  /* increment line pointer         */
            cnt++; /* and count                      */
            break;
        }
    } while (cnt < len - 2 && c != LF); /* check limit and CR             */
    *lp = 0;                            /* mark end of string             */
    return 1;
}
