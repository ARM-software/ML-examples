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
#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* A service handler function pointer */
typedef void (*service_handler)(void);

/**
 * @brief  Initialise the GPIO port/pin
 * @param[in]   gpio_port   GPIO port number
 * @param[in]   gpio_pin    pin number for the GPIO port
 * @param[in]   is_input    to be set to true to set the GPIO pin direction
 *                          as input. If false, the direction is set as
 *                          output.
 */
void gpio_init(uint8_t gpio_port, uint8_t gpio_pin, bool is_input);

/**
 * @brief  Set the signal for GPIO port/pin
 * @param[in]   gpio_port   GPIO port number
 * @param[in]   gpio_pin    pin number for the GPIO port
 * @param[in]   value       the signal is driven high if this is set as true,
 *                          driven low otherwise.
 * @return  true if successful, false otherwise.
 */
bool gpio_set_pin(uint8_t gpio_port, uint8_t gpio_pin, bool value);

/**
 * @brief  Get the current signal from GPIO port/pin
 * @param[in]   gpio_port   GPIO port number
 * @param[in]   gpio_pin    pin number for the GPIO port
 * @param[out]  value       pointer to bool; set to true if signal is high,
 *                          set to false otherwise.
 * @return  true if successful, false otherwise.
 */
bool gpio_get_pin(uint8_t gpio_port, uint8_t gpio_pin, bool* value);

/**
 * @brief  Waits for a GPIO signal to be driven high
 * @param[in]   gpio_port   GPIO port number
 * @param[in]   gpio_pin    pin number for the GPIO port
 * @param[in]   service     a function that needs to be run while
 *                          waiting for the signal. It can be set
 *                          to NULL if unrequired.
 * @return  true if successful, false otherwise.
 */
bool wait_for_gpio_signal(uint8_t gpio_port, uint8_t gpio_pin, service_handler service);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_GPIO_H */
