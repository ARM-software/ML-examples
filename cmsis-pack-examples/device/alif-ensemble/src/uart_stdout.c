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
#include "uart_stdout.h"
#include "Driver_PINMUX_AND_PINPAD.h"
#include "Driver_USART.h"

#include <stdio.h>
#include <stdint.h>

#define CNTLQ       0x11
#define CNTLS       0x13
#define DEL         0x7F
#define BACKSPACE   0x08
#define CR          0x0D
#define LF          0x0A
#define ESC         0x1B

// <h>STDOUT USART Interface

//   <o>Connect to hardware via Driver_USART# <0-255>
//   <i>Select driver control block for USART interface
#if !defined(USART_DRV_NUM)
#define USART_DRV_NUM           2
#endif /* USART_DRV_NUM */

//   <o>Baudrate
#define USART_BAUDRATE          115200

// </h>


#define _USART_Driver_(n)  Driver_USART##n
#define  USART_Driver_(n) _USART_Driver_(n)

extern ARM_DRIVER_USART  USART_Driver_(USART_DRV_NUM);
#define ptrUSART       (&USART_Driver_(USART_DRV_NUM))

/**
  Initialize pinmux

  \return          0 on success, or -1 on error.
*/
static int usart_pinmux_init(void)
{
    int32_t ret;
    uint32_t port_config = PAD_FUNCTION_READ_ENABLE |
                           PAD_FUNCTION_DRIVER_DISABLE_STATE_WITH_PULL_UP;

#if USART_DRV_NUM == 1	/* Rev A0 - DevKit Alpha */
    /* PINMUX UART1_A */
    /* Configure GPIO Pin : P1_4 as UART1_RX_A */
    ret = PINMUX_Config (PORT_NUMBER_1, PIN_NUMBER_4, PINMUX_ALTERNATE_FUNCTION_1);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
    ret = PINPAD_Config(PORT_NUMBER_1, PIN_NUMBER_4, port_config);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }

    /* Configure GPIO Pin : P1_5 as UART1_TX_A */
    ret = PINMUX_Config (PORT_NUMBER_1, PIN_NUMBER_5, PINMUX_ALTERNATE_FUNCTION_1);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
    ret = PINPAD_Config(PORT_NUMBER_1, PIN_NUMBER_5, port_config);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
#elif USART_DRV_NUM == 2	/* Rev A1 - DevKit Beta */
    /* PINMUX UART2_B */
    /* Configure GPIO Pin : P3_16 as UART2_RX_B */
    ret = PINMUX_Config (PORT_NUMBER_3, PIN_NUMBER_16, PINMUX_ALTERNATE_FUNCTION_2);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
    ret = PINPAD_Config(PORT_NUMBER_3, PIN_NUMBER_16, port_config);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }

    /* Configure GPIO Pin : P3_17 as UART2_TX_B */
    ret = PINMUX_Config (PORT_NUMBER_3, PIN_NUMBER_17, PINMUX_ALTERNATE_FUNCTION_2);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
    ret = PINPAD_Config(PORT_NUMBER_3, PIN_NUMBER_17, port_config);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
#elif USART_DRV_NUM == 4	/* Rev A1 - DevKit Beta */
    /* PINMUX UART4_B */
    /* Configure GPIO Pin : P3_1 as UART4_RX_B */
    ret = PINMUX_Config (PORT_NUMBER_3, PIN_NUMBER_1, PINMUX_ALTERNATE_FUNCTION_1);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
    ret = PINPAD_Config(PORT_NUMBER_3, PIN_NUMBER_1, port_config);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }

    /* Configure GPIO Pin : P3_2 as UART4_TX_B */
    ret = PINMUX_Config (PORT_NUMBER_3, PIN_NUMBER_2, PINMUX_ALTERNATE_FUNCTION_1);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
    ret = PINPAD_Config(PORT_NUMBER_3, PIN_NUMBER_2, port_config);
    if(ret != ARM_DRIVER_OK) {
        return ret;
    }
#endif

    return ret;
}

void usart_callback(uint32_t event)
{
    if (event & ARM_USART_EVENT_SEND_COMPLETE) {
        /* Send Success */
    }

    if (event & ARM_USART_EVENT_RECEIVE_COMPLETE) {
        /* Receive Success */
    }

    if (event & ARM_USART_EVENT_RX_TIMEOUT) {
        /* Receive Success with rx timeout */
    }
}

int32_t UartStdOutInit(void)
{
  int32_t status;

  status = usart_pinmux_init();

  status += ptrUSART->Initialize(usart_callback);

  status += ptrUSART->PowerControl(ARM_POWER_FULL);

  status += ptrUSART->Control(ARM_USART_MODE_ASYNCHRONOUS |
                             ARM_USART_DATA_BITS_8       |
                             ARM_USART_PARITY_NONE       |
                             ARM_USART_STOP_BITS_1       |
                             ARM_USART_FLOW_CONTROL_NONE,
                             USART_BAUDRATE);

  status += ptrUSART->Control(ARM_USART_CONTROL_TX, 1);
  status += ptrUSART->Control(ARM_USART_CONTROL_RX, 1);

  return status;
}

unsigned char UartPutc(unsigned char ch)
{
  uint8_t buf[1];
  buf[0] = ch;

  if (ptrUSART->Send(buf, 1) != ARM_DRIVER_OK) {
    return (-1);
  }
  while (ptrUSART->GetTxCount() != 1);

  if (buf[0] == '\n') {
      if (ptrUSART->Send(buf, 1) != ARM_DRIVER_OK) {
        return (-1);
      }
      while (ptrUSART->GetTxCount() != 1);
  }

  return (ch);
}

unsigned char UartGetc(void)
{
    uint8_t buf[1];

    if (ptrUSART->Receive(buf, 1) != ARM_DRIVER_OK)
    {
        return (-1);
    }
    while (ptrUSART->GetRxCount() != 1)
        ;
    return (buf[0]);
}

unsigned char UartGetcNoBlock(void)
{
    static uint8_t buf[1];
    static bool buffer_queued = false;

    if (!buffer_queued)
    {
        if (ptrUSART->Receive(buf, 1) != ARM_DRIVER_OK) {
            return (-1);
        } else {
            buffer_queued = true;
        }
    }
    if (ptrUSART->GetRxCount() == 0) {
        return -1;
    } else {
        buffer_queued = false;
        return (buf[0]);
    }
}

bool GetLine (char *lp, unsigned int len)
{
   unsigned int cnt = 0;
   char c;

   do {
       c = UartGetc();
       switch (c) {
           case CNTLQ:                       /* ignore Control S/Q.            */
           case CNTLS:
               break;
           case BACKSPACE:
           case DEL:
               if (cnt == 0) {
                   break;
               }
               cnt--;                         /* decrement count.               */
               lp--;                          /* and line pointer.              */
               UartPutc (0x08);               /* echo backspace.                */
               UartPutc (' ');
               UartPutc (0x08);
               fflush (stdout);
               break;
           case ESC:
           case 0:
               *lp = 0;                       /* ESC - stop editing line.       */
               return false;
           case CR:                            /* CR - done, stop editing line. */
               UartPutc (*lp = c);             /* Echo and store character.     */
               lp++;                           /* Increment line pointer        */
               cnt++;                          /* and count.                    */
               c = LF;
               UartPutc (*lp = c);             /* Echo and store character.      */
               fflush (stdout);
               lp++;                           /* Increment line pointer         */
               break;
           default:
               UartPutc (*lp = c);            /* echo and store character.      */
               fflush (stdout);
               lp++;                          /* increment line pointer.        */
               cnt++;                         /* and count.                     */
               break;
       }
   } while (cnt < len - 2  &&  c != LF);      /* check limit and CR.            */
   *lp = 0;                                   /* mark end of string.            */
    return true;
}

__attribute__((noreturn)) void UartEndSimulation(int code)
{
    UartPutc((char) 0x4);  // End of simulation
    UartPutc((char) code); // Exit code
    while(1);
}
