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
#ifndef TRIGGER_HPP
#define TRIGGER_HPP

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

#include "gpio.h"

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)

namespace arm {
namespace app {

    /* GPIO signal direction enum */
    enum class SignalDirection { DirectionOutput = 0, DirectionInput = 1, DirectionInvalid = 2 };

    /* Valid GPIO port numbers */
    enum class SignalPort { Port1 = 1, Port2 = 2, Port3 = 3 };

    /* Valid and used GPIO pin number definitions */
    enum class SignalPin {
        Port1_UserButton1 = 12, /* Wired to SW2 on the board. */
        Port3_UserButton2 = 4,  /* Wired to SW3 on the board. */
        Port1_StatusLED1 = 14,  /* LED1 - green. */
        Port1_StatusLED2 = 15   /* LED2 - orange. */
    };

    /**
     * @brief   Class to wrap inbound/outbound GPIO "trigger" signals.
     */
    class GpioSignal {
    private:
        uint8_t m_port              = 0xFF; /* GPIO port number to be used */
        uint8_t m_pin               = 0xFF; /* GPIO pin number (from the GPIO port) to be used */
        SignalDirection m_direction = SignalDirection::DirectionInvalid; /* signal direction */

    public:
        /* Remove default contructor */
        GpioSignal() = delete;

        /**
         * @brief       Constructor
         * @param[in]   port    GPIO port number
         * @param[in]   pin     GPIO pin number from the specified GPIO port
         * @param[in]   dir     Signal direction (input or an output)
         **/
        explicit GpioSignal(SignalPort port, SignalPin pin, SignalDirection dir);

        /* Destructor */
        ~GpioSignal() = default;

        /**
         * @brief       Set the signal value for this port/pin combination
         *              if the direction is set as "output".
         * @param[in]   signalValue If true, the function will assert the signal,
         *                          or else will release it.
         **/
        void Send(bool signalValue);

        /**
         * @brief   Receives a trigger signal
         * @return  True if the direction is set as input and the signal is being
         *          asserted; false otherwise.
         **/
        bool Recv();

        /**
         *  @brief  Waits for the signal to be asserted (blocking function call).
         *  @return True if the wait is successful, false otherwise.
         **/
        bool WaitForSignal(service_handler handler = nullptr);
    };

} /* namespace app */
} /* namespace arm */

#endif /* TRIGGER_HPP */