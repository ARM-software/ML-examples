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

#ifndef DSCNNMODEL_H
#define DSCNNMODEL_H

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/NN/Model.h"

#define SAMP_FREQ 16000
#define FRAME_SHIFT_MS 20
#define FRAME_SHIFT (static_cast<int16_t>(SAMP_FREQ * 0.001 * FRAME_SHIFT_MS))
#define NUM_FRAMES 49
#define NUM_MFCC_COEFFS 10
#define MFCC_BUFFER_SIZE (NUM_FRAMES*NUM_MFCC_COEFFS)
#define FRAME_LEN_MS 40
#define FRAME_LEN (static_cast<int16_t>(SAMP_FREQ * 0.001 * FRAME_LEN_MS))

class DsCnnModel : public Model {

public:
    DsCnnModel();
private:
    /* Maximum number of individual operations that can be enlisted */
    static constexpr int _maxOpCnt = 9;

    /* A mutable op resolver instance */
    tflite::MicroMutableOpResolver<_maxOpCnt> _opResolver;
protected:
    /** @brief   Gets the reference to op resolver interface class */
    const tflite::MicroOpResolver& GetOpResolver() override;

    /** @brief   Adds operations to the op resolver instance */
    bool EnlistOperations() override;

    /** @brief   Gets a pointer to the tensor arena */
    uint8_t* GetTensorArena() override;

    /** @brief   Gets the total size of tensor arena available for use */
    size_t GetActivationBufferSize() override;
};

#endif /* DSCNNMODEL_H */
