/*
 * SPDX-FileCopyrightText: Copyright 2022-2024 Arm Limited and/or its
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

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio_wrapper.h"

#include "RTE_Components.h"
#include "RTE_Device.h"
#include CMSIS_device_header

#include "Driver_GPIO.h"
#include "log_macros.h"

extern ARM_DRIVER_GPIO Driver_GPIO6;
extern ARM_DRIVER_GPIO Driver_GPIO7;
extern ARM_DRIVER_GPIO Driver_GPIO12;

static bool is_gpio_port_valid(uint8_t gpio_port)
{
    switch (gpio_port) {
        case 6: return true;
        case 7: return true;
        case 12: return true;
        default: printf_err("Invalid GPIO port # %d\n", gpio_port);
    }
    return false;
}

static ARM_DRIVER_GPIO* get_driver(uint8_t port)
{
    switch (port) {
    case 6:
        return &Driver_GPIO6;
    case 7:
        return &Driver_GPIO7;
    case 12:
        return &Driver_GPIO12;
    default:
        break;
    }

    return NULL;
}

bool gpio_set_pin(uint8_t gpio_port, uint8_t gpio_pin, bool value)
{
    ARM_DRIVER_GPIO* ptrDrv      = NULL;
    GPIO_PIN_DIRECTION direction = GPIO_PIN_DIRECTION_INPUT;
    GPIO_PIN_OUTPUT_STATE state  = value ? GPIO_PIN_OUTPUT_STATE_HIGH : GPIO_PIN_OUTPUT_STATE_LOW;
    int32_t ret                  = 0;

    if (!is_gpio_port_valid(gpio_port)) {
        return false;
    }

    ptrDrv = get_driver(gpio_port);

    ret = ptrDrv->GetDirection(gpio_pin, &direction);
    if (ARM_DRIVER_OK != ret || direction != GPIO_PIN_DIRECTION_OUTPUT) {
        return false;
    }

    debug("Setting pin %d to %d\n", gpio_pin, state);
    ret = ptrDrv->SetValue(gpio_pin, state);
    return (ARM_DRIVER_OK == ret);
}

bool gpio_get_pin(uint8_t gpio_port, uint8_t gpio_pin, bool* value)
{
    ARM_DRIVER_GPIO* ptrDrv      = NULL;
    GPIO_PIN_DIRECTION direction = GPIO_PIN_DIRECTION_OUTPUT;
    GPIO_PIN_STATE state         = GPIO_PIN_STATE_LOW;
    int32_t ret                  = 0;

    if (!is_gpio_port_valid(gpio_port)) {
        return false;
    }

    ptrDrv = get_driver(gpio_port);

    ret = ptrDrv->GetDirection(gpio_pin, &direction);
    if (ARM_DRIVER_OK != ret || direction != GPIO_PIN_DIRECTION_INPUT) {
        return false;
    }

    ret = ptrDrv->GetValue(gpio_pin, &state);
    if (ARM_DRIVER_OK != ret) {
        return false;
    }

    *value = (state == GPIO_PIN_STATE_HIGH);
    return true;
}

bool wait_for_gpio_signal(uint8_t gpio_port, uint8_t gpio_pin, service_handler service)
{
    bool signal = false;

    while (gpio_get_pin(gpio_port, gpio_pin, &signal)) {

        /* If we have the signal being asserted */
        if (signal) {
            return true;
        }

        __WFI(); /* Wait for interrupt */

        if (service) {
            service();
        }

        signal = false;
    }

    return false;
}

void gpio_init(uint8_t gpio_port, uint8_t gpio_pin, bool is_input)
{
    ARM_DRIVER_GPIO* ptrDrv = NULL;
    int32_t ret             = 0;

    if (!is_gpio_port_valid(gpio_port)) {
        return;
    }

    ptrDrv = get_driver(gpio_port);

    ret = ptrDrv->Initialize(gpio_pin, NULL);

    if (ret != ARM_DRIVER_OK) {
        printf_err("ERROR: Failed to initialize\n");
        return;
    }

    ret = ptrDrv->PowerControl(gpio_pin, ARM_POWER_FULL);

    if (ret != ARM_DRIVER_OK) {
        printf_err("ERROR: Failed to powered full\n");
        goto error_uninitialize;
    }

    ret = ptrDrv->SetDirection(gpio_pin,
                               is_input ? GPIO_PIN_DIRECTION_INPUT : GPIO_PIN_DIRECTION_OUTPUT);

    if (ret != ARM_DRIVER_OK) {
        printf_err("ERROR: Failed to configure\n");
        goto error_power_off;
    }

    return;

error_power_off:

    ret = ptrDrv->PowerControl(gpio_pin, ARM_POWER_OFF);

    if (ret != ARM_DRIVER_OK) {
        printf_err("ERROR: Failed to power off \n");
    } else {
        info("LEDs power off \n");
    }

error_uninitialize:

    ret = ptrDrv->Uninitialize(gpio_pin);

    if (ret != ARM_DRIVER_OK) {
        printf_err("Failed to Un-initialize \n");
    } else {
        info("Un-initialized \n");
    }
}

#ifdef __cplusplus
}
#endif
