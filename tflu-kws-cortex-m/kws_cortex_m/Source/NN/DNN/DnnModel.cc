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

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/NN/DNN/DnnModel.h"
#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/NN/BufAttributes.h"

#ifndef ACTIVATION_BUF_SZ
#define ACTIVATION_BUF_SZ       (0x20000) /* default value of 128kiB */
#endif /* ACTIVATION_BUF_SZ */

static uint8_t  _tensor_arena[ACTIVATION_BUF_SZ] ALIGNMENT_ATTRIBUTE;

DnnModel::DnnModel()
{
    numMfccFeatures = NUM_MFCC_COEFFS;
    frameLen = FRAME_LEN;
    frameShift = FRAME_SHIFT;
    numFrames = NUM_FRAMES;
}

const tflite::MicroOpResolver& DnnModel::GetOpResolver()
{
    return this->_opResolver;
}

bool DnnModel::EnlistOperations()
{
    this->_opResolver.AddQuantize();
    this->_opResolver.AddFullyConnected();
    this->_opResolver.AddRelu();
    this->_opResolver.AddSoftmax();
    this->_opResolver.AddDequantize();

    return true;
}

uint8_t* DnnModel::GetTensorArena()
{
    return _tensor_arena;
}

size_t DnnModel::GetActivationBufferSize()
{
    return ACTIVATION_BUF_SZ;
}
