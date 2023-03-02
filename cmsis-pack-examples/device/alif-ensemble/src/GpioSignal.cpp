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

#include "GpioSignal.hpp"

#include "log_macros.h"

#include <inttypes.h>

namespace arm {
namespace app {

    GpioSignal::GpioSignal(SignalPort port, SignalPin pin, SignalDirection dir) :
        m_port(static_cast<uint8_t>(port)), m_pin(static_cast<uint8_t>(pin)), m_direction(dir)
    {
        bool isInput = (this->m_direction == SignalDirection::DirectionInput);
        gpio_init(this->m_port, this->m_pin, isInput);
    }

    void GpioSignal::Send(bool signalValue)
    {
        if (this->m_direction == SignalDirection::DirectionOutput) {
            if (true == gpio_set_pin(this->m_port, this->m_pin, signalValue)) {
                debug(
                    "Sent signal to port %d pin %d: %d\n", this->m_port, this->m_pin, signalValue);
                return;
            }
        }
        printf_err("Failed to set port %d pin %d: %d\n", this->m_port, this->m_pin, signalValue);
    }

    bool GpioSignal::Recv()
    {
        if (this->m_direction == SignalDirection::DirectionInput) {
            bool signalValue = false;
            if (true == gpio_get_pin(this->m_port, this->m_pin, &signalValue)) {
                debug(
                    "Got signal from port %d pin %d: %d\n", this->m_port, this->m_pin, signalValue);
                return signalValue;
            }
        }
        printf_err("Failed to get signal from port %d pin %d.\n", this->m_port, this->m_pin);
        return false;
    }

    bool GpioSignal::WaitForSignal(service_handler handler)
    {
        if (this->m_direction == SignalDirection::DirectionInput) {
            return wait_for_gpio_signal(this->m_port, this->m_pin, handler);
        }
        return false;
    }

} /* namespace app */
} /* namespace arm */