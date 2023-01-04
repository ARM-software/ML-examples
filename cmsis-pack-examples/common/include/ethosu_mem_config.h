/*
 * Copyright (c) 2022 Arm Limited. All rights reserved.
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

#ifndef ETHOS_U_NPU_MEM_CONFIG_H
#define ETHOS_U_NPU_MEM_CONFIG_H

#define ETHOS_U_NPU_MEMORY_MODE_SRAM_ONLY      0
#define ETHOS_U_NPU_MEMORY_MODE_SHARED_SRAM    1
#define ETHOS_U_NPU_MEMORY_MODE_DEDICATED_SRAM 2

#define ETHOS_U_MEM_BYTE_ALIGNMENT 16

#ifndef ETHOS_U_NPU_MEMORY_MODE
#if defined(ETHOSU65)
#define ETHOS_U_NPU_MEMORY_MODE ETHOS_U_NPU_MEMORY_MODE_DEDICATED_SRAM
#else
#define ETHOS_U_NPU_MEMORY_MODE ETHOS_U_MEMORY_MODE_SHARED_SRAM
#endif /* defined (ETHOSU65) */
#endif /* ETHOS_U_NPU_MEMORY_MODE */

#if (ETHOS_U_NPU_MEMORY_MODE == ETHOS_U_NPU_MEMORY_MODE_DEDICATED_SRAM)
#ifndef ETHOS_U_NPU_CACHE_SIZE
#define ETHOS_U_CACHE_BUF_SZ (393216U) /* See vela doc for reference */
#else
#define ETHOS_U_CACHE_BUF_SZ ETHOS_U_NPU_CACHE_SIZE
#endif /* ETHOS_U_NPU_CACHE_SIZE */
#else
#define ETHOS_U_CACHE_BUF_SZ (0U)
#endif /* CACHE_BUF_SZ */

/**
 * Activation buffer aka tensor arena section name
 * We have to place the tensor arena in different region based on the memory config.
 **/
#if (ETHOS_U_NPU_MEMORY_MODE == ETHOS_U_NPU_MEMORY_MODE_SHARED_SRAM)
#define ACTIVATION_BUF_SECTION      section(".bss.NoInit.activation_buf_sram")
#define ACTIVATION_BUF_SECTION_NAME ("SRAM")
#elif (ETHOS_U_NPU_MEMORY_MODE == ETHOS_U_NPU_MEMORY_MODE_SRAM_ONLY)
#define ACTIVATION_BUF_SECTION      section(".bss.NoInit.activation_buf_sram")
#define ACTIVATION_BUF_SECTION_NAME ("SRAM")
#elif (ETHOS_U_NPU_MEMORY_MODE == ETHOS_U_NPU_MEMORY_MODE_DEDICATED_SRAM)
#define ACTIVATION_BUF_SECTION      section("activation_buf_dram")
#define CACHE_BUF_SECTION           section(".bss.NoInit.ethos_u_cache")
#define ACTIVATION_BUF_SECTION_NAME ("DDR/DRAM")
#define CACHE_BUF_ATTRIBUTE         __attribute__((aligned(ETHOS_U_MEM_BYTE_ALIGNMENT), CACHE_BUF_SECTION))
#endif

#endif /* ETHOS_U_NPU_MEM_CONFIG_H */
