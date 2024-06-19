/*
 * SPDX-FileCopyrightText: Copyright 2023-2024 Arm Limited and/or its
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

#include "BoardInit.hpp"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

#include "RTE_Components.h"
#include "RTE_Device.h"
#include CMSIS_device_header
#include "pinconf.h"
#include "Driver_Common.h"
#include "ethosu_driver.h"
#include "uart_stdout.h"
#include "log_macros.h"
#include <stdio.h>

#if defined(M55_HP)
#include "se_services_port.h"
#endif /* defined(M55_HP) */

static struct ethosu_driver npuDriver;
static void npu_irq_handler(void)
{
    ethosu_irq_handler(&npuDriver);
}

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

bool NpuInit()
{
    void * const npuBaseAddr = reinterpret_cast<void*>(LOCAL_NPU_BASE);

    /*  Initialize Ethos-U NPU driver. */
    if (ethosu_init(&npuDriver, /* Arm Ethos-U device driver pointer  */
                    npuBaseAddr, /* Base address for the Arm Ethos-U device */
                    0, /* Cache memory pointer (not applicable for U55) */
                    0, /* Cache memory size */
                    1, /* Secure */
                    1) /* Privileged */ ) {
        printf("Failed to initialize Arm Ethos-U driver");
        return false;
    }

    NVIC_SetVector(LOCAL_NPU_IRQ_IRQn, (uint32_t) &npu_irq_handler);
    NVIC_EnableIRQ(LOCAL_NPU_IRQ_IRQn);

    return true;
}

/**
 * @brief  CPU L1-Cache enable.
 * @param  None
 * @retval None
 */
static void CpuCacheEnable(void)
{
    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();
}

#if defined (M55_HP)

static int I3CPinsInit(void)
{
    int ret = pinconf_set(PORT_7, PIN_2,
                          PINMUX_ALTERNATE_FUNCTION_5,
                          PADCTRL_READ_ENABLE | PADCTRL_DRIVER_DISABLED_PULL_UP);

    if (ret != ARM_DRIVER_OK) {
        return ret;
    }

    ret = pinconf_set(PORT_7, PIN_3,
                      PINMUX_ALTERNATE_FUNCTION_5,
                      PADCTRL_READ_ENABLE | PADCTRL_DRIVER_DISABLED_PULL_UP);

    return ret;
}

static int CameraPinsInit(void)
{
    return pinconf_set(PORT_0, PIN_3, PINMUX_ALTERNATE_FUNCTION_6, 0);
}

static uint32_t CameraClocksEnable(void)
{
    uint32_t error_code = SERVICES_REQ_SUCCESS;
    uint32_t service_error_code;
    run_profile_t runp = {0};

    /* Initialize the SE services */
    se_services_port_init();

    /* Enable MIPI Clocks */
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_CLK_100M, true, &service_error_code);
    if(error_code != SERVICES_REQ_SUCCESS) {
        printf_err("SE: MIPI 100MHz clock enable = %d\n", error_code);
        return error_code;
    }

    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_HFOSC, true, &service_error_code);
    if(error_code != SERVICES_REQ_SUCCESS) {
        printf_err("SE: MIPI 38.4Mhz(HFOSC) clock enable = %d\n", error_code);
        goto error_disable_100mhz_clk;
    }

    /* Get the current run configuration from SE */
    error_code = SERVICES_get_run_cfg(se_services_s_handle,
                                      &runp,
                                      &service_error_code);
    if (error_code != SERVICES_REQ_SUCCESS) {
        printf_err("\r\nSE: get_run_cfg error = %d\n", error_code);
        goto error_disable_hfosc_clk;
    }

    runp.memory_blocks = MRAM_MASK | SRAM0_MASK | SRAM1_MASK;

    runp.phy_pwr_gating = MIPI_PLL_DPHY_MASK | MIPI_TX_DPHY_MASK | MIPI_RX_DPHY_MASK | LDO_PHY_MASK;

    /* Set the new run configuration */
    error_code = SERVICES_set_run_cfg(se_services_s_handle,
                                      &runp,
                                      &service_error_code);
    if(error_code) {
        printf_err("\r\nSE: set_run_cfg error = %d\n", error_code);
        goto error_disable_hfosc_clk;
    }

    info("Camera clocks enabled.\n");
    return 0;

error_disable_hfosc_clk:
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_HFOSC, false, &service_error_code);
    if (error_code != SERVICES_REQ_SUCCESS) {
        printf_err("SE: MIPI 38.4Mhz(HFOSC)  clock disable = %d\n", error_code);
    }

error_disable_100mhz_clk:
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_CLK_100M, false, &service_error_code);
    if (error_code != SERVICES_REQ_SUCCESS) {
        printf_err("SE: MIPI 100MHz clock disable = %d\n", error_code);
    }

    return 1;
}

#endif /* defined (M55_HP) */

void BoardInit(void)
{
#if !defined(SEMIHOSTING)
    UartStdOutInit();
#endif /* defined(SEMIHOSTING) */

#if defined (M55_HP)
    /* Initialise the I3C and camera pins for the High performance core. */
    if (ARM_DRIVER_OK != I3CPinsInit()) {
        printf("I3CPinsInit failed\n");
        return;
    }
    if (ARM_DRIVER_OK != CameraPinsInit()) {
        printf("CameraPinsInit failed\n");
        return;
    }

    if (0 != CameraClocksEnable()) {
        printf("CameraClocksEnable failed\n");
        return;
    }
#endif /* defined (M55_HP) */

#if defined(ETHOSU_ARCH) && (ETHOSU_ARCH==u55)
    if (!NpuInit()) {
        return;
    }
#endif

    /* Enable the CPU Cache */
    CpuCacheEnable();

    printf("Board init: completed\n");
    return;
}
