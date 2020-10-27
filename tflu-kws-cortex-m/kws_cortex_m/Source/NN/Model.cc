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

#include <cstdint>

#include "tensorflow/lite/micro/examples/kws_cortex_m/Source/NN/Model.h"

/* Initialise the model */
Model::~Model()
{
    if (this->_interpreter) {
        delete this->_interpreter;
    }
}

Model::Model() :
        _inited (false),
        _type(kTfLiteNoType)
{
    this->_errorReporterPtr = &this->_uErrorReporter;
}

bool Model::Init()
{
    /* Following tf lite micro example:
     * Map the model into a usable data structure. This doesn't involve any
     * copying or parsing, it's a very lightweight operation. */
    const uint8_t* model_addr = ModelPointer();
    this->_model = ::tflite::GetModel(model_addr);

    if (this->_model->version() != TFLITE_SCHEMA_VERSION) {
        this->_errorReporterPtr->Report(
                "model's schema version %d is not equal "
                "to supported version %d.",
                this->_model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    /* Pull in only the operation implementations we need.
     * This relies on a complete list of all the ops needed by this graph.
     * An easier approach is to just use the AllOpsResolver, but this will
     * incur some penalty in code space for op implementations that are not
     * needed by this graph.
     * static ::tflite::ops::micro::AllOpsResolver resolver; */
    /* NOLINTNEXTLINE(runtime-global-variables) */
    this->EnlistOperations();

    /* Build an interpreter to run the model with. */
    this->_interpreter = new ::tflite::MicroInterpreter(
            this->_model, this->GetOpResolver(),
            this->GetTensorArena(), this->GetActivationBufferSize(),
            this->_errorReporterPtr);

    if (!this->_interpreter) {
        printf("Failed to allocate interpreter\n");
        return false;
    }

    /* Allocate memory from the tensor_arena for the model's tensors. */
    TfLiteStatus allocate_status = this->_interpreter->AllocateTensors();

    if (allocate_status != kTfLiteOk) {
        this->_errorReporterPtr->Report("AllocateTensors() failed");
        printf("Tensor allocation failed!\n");
        delete this->_interpreter;
        return false;
    }

    /* Get information about the memory area to use for the model's input. */
    this->_input = this->_interpreter->input(0);
    this->_output = this->_interpreter->output(0);

    if (!this->_input || !this->_output) {
        printf("Failed to get tensors\n");
        return false;
    } else {
        this->_type = this->_input->type;

        /* Clear the tensor */
        std::memset(this->_input->data.data, 0, this->_input->bytes);
        std::memset(this->_output->data.data, 0, this->_output->bytes);
    }

    this->_inited = true;
    return true;
}

bool Model::IsInited() const
{
    return this->_inited;
}

bool Model::RunInference()
{
    bool inference_state = false;
    if (this->_model && this->_interpreter) {
        if (kTfLiteOk != this->_interpreter->Invoke()) {
            printf("Invoke failed.\n");
        } else {
            inference_state = true;
        }
    } else {
        printf("Error: No interpreter!\n");
    }
    return inference_state;
}

TfLiteTensor* Model::GetInputTensor() const
{
    if (this->_model && this->_interpreter) {
        return this->_input;
    }
    return nullptr;
}

TfLiteTensor* Model::GetOutputTensor() const
{
    if (this->_model && this->_interpreter) {
        return this->_output;
    }
    return nullptr;
}

TfLiteType Model::GetType() const
{
    return this->_type;
}

TfLiteIntArray* Model::GetInputShape() const
{
    if (this->_model && this->_interpreter) {
        return this->_input->dims;
    }
    return nullptr;
}

TfLiteIntArray* Model::GetOutputShape() const
{
    if (this->_model && this->_interpreter) {
        return this->_output->dims;
    }
    return nullptr;
}

const uint8_t* Model::ModelPointer()
{
    return GetModelPointer();
}

size_t Model::ModelSize()
{
    return GetModelLen();
}