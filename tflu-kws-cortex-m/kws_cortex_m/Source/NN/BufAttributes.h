/*
 * Copyright (C) 2020 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifdef __has_attribute
#define HAVE_ATTRIBUTE(x) __has_attribute(x)
#else   /* __has_attribute */
#define HAVE_ATTRIBUTE(x) 0
#endif  /* __has_attribute */

#if HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__))

/* We want all buffers/sections to be aligned to 16 byte  */
#define ALIGNMENT_REQ               aligned(16)
/* Form the attributes, alignment is mandatory */
#define ALIGNMENT_ATTRIBUTE   __attribute__((ALIGNMENT_REQ))

#else /* HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__)) */

#define ALIGNMENT_ATTRIBUTE

#endif /* HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__)) */
