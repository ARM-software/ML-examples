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

#ifndef BUF_ATTRIBUTES_HPP
#define BUF_ATTRIBUTES_HPP

#if defined(ETHOSU55) || defined(ETHOSU65) || defined(ETHOSU85)
#define ARM_NPU
#endif /* defined(ETHOSU55) || defined(ETHOSU65) || defined(ETHOSU85) */

#if defined(ARM_NPU)
/* When Arm NPU is defined, we use the config set by NPU mem parameters */
#include "ethosu_mem_config.h"
#define BYTE_ALIGNMENT ETHOS_U_MEM_BYTE_ALIGNMENT
#else /* defined(ARM_NPU) */
/* otherwise, we use the default ones here. */
#define ACTIVATION_BUF_SECTION      section(".bss.NoInit.activation_buf_sram")
#define ACTIVATION_BUF_SECTION_NAME ("SRAM")
#define BYTE_ALIGNMENT              16
#endif /* defined(ARM_NPU) */

#ifdef __has_attribute
#define HAVE_ATTRIBUTE(x) __has_attribute(x)
#else /* __has_attribute */
#define HAVE_ATTRIBUTE(x) 0
#endif /* __has_attribute */

#if HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__))

/* We want all buffers/sections to be aligned to 16 byte.  */
#define ALIGNMENT_REQ aligned(BYTE_ALIGNMENT)

#define MODEL_SECTION section("nn_model")

/* Label section name */
#define LABEL_SECTION section("labels")

#ifndef ACTIVATION_BUF_SZ
#warning "ACTIVATION_BUF_SZ needs to be defined. Using default value"
#define ACTIVATION_BUF_SZ 0x00200000
#endif /* ACTIVATION_BUF_SZ */

/* IFM section name. */
#define IFM_BUF_SECTION section("ifm")

/* Form the attributes, alignment is mandatory. */
#define MAKE_ATTRIBUTE(x)        __attribute__((ALIGNMENT_REQ, x))
#define MODEL_TFLITE_ATTRIBUTE   MAKE_ATTRIBUTE(MODEL_SECTION)
#define ACTIVATION_BUF_ATTRIBUTE MAKE_ATTRIBUTE(ACTIVATION_BUF_SECTION)
#define IFM_BUF_ATTRIBUTE        MAKE_ATTRIBUTE(IFM_BUF_SECTION)
#define LABELS_ATTRIBUTE         MAKE_ATTRIBUTE(LABEL_SECTION)

#else /* HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__)) */

#define MODEL_TFLITE_ATTRIBUTE
#define ACTIVATION_BUF_ATTRIBUTE
#define IFM_BUF_ATTRIBUTE
#define LABELS_ATTRIBUTE

#endif /* HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__)) */

#endif /* BUF_ATTRIBUTES_HPP */
