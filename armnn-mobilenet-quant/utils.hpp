//
// Copyright © 2019 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include "inference_test_image.hpp"

#include <armnn/Tensor.hpp>
#include <armnn/TypesUtils.hpp>

#include <boost/format.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/program_options.hpp>

#include <string>
#include <vector>
#include <istream>

namespace armnn
{

inline std::istream& operator>>(std::istream& in, armnn::Compute& compute)
{
    std::string token;
    in >> token;
    compute = armnn::ParseComputeDevice(token.c_str());
    if (compute == armnn::Compute::Undefined)
    {
        in.setstate(std::ios_base::failbit);
        throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
    }
    return in;
}

inline std::istream& operator>>(std::istream& in, armnn::BackendId& backend)
{
    std::string token;
    in >> token;
    armnn::Compute compute = armnn::ParseComputeDevice(token.c_str());
    if (compute == armnn::Compute::Undefined)
    {
        in.setstate(std::ios_base::failbit);
        throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
    }
    backend = compute;
    return in;
}

} // namespace armnn

// Prepare a qasymm8 image tensor
std::vector<uint8_t> PrepareImageTensor(const std::string& imagePath,
                                        unsigned int newWidth,
                                        unsigned int newHeight,
                                        const NormalizationParameters& normParams);

template<typename TContainer>
armnn::InputTensors MakeInputTensors(const std::vector<armnn::BindingPointInfo>& inputBindings,
                                     const std::vector<TContainer>& inputDataContainers)
{
    armnn::InputTensors inputTensors;

    const size_t numInputs = inputBindings.size();
    if (numInputs != inputDataContainers.size())
    {
        throw armnn::Exception(boost::str(boost::format("The number of inputs does not match the number of "
                                                        "tensor data containers: %1% != %2%")
                                          % numInputs
                                          % inputDataContainers.size()));
    }

    for (size_t i = 0; i < numInputs; i++)
    {
        const armnn::BindingPointInfo& inputBinding = inputBindings[i];
        const TContainer& inputData = inputDataContainers[i];

        boost::apply_visitor([&](auto&& value)
        {
            if (value.size() != inputBinding.second.GetNumElements())
            {
               throw armnn::Exception(boost::str(boost::format("The input tensor has incorrect size "
                                                               "(expected %1% got %2%)")
                                                 % inputBinding.second.GetNumElements()
                                                 % value.size()));
            }

            armnn::ConstTensor inputTensor(inputBinding.second, value.data());
            inputTensors.push_back(std::make_pair(inputBinding.first, inputTensor));
        },
        inputData);
    }

    return inputTensors;
}

template<typename TContainer>
armnn::OutputTensors MakeOutputTensors(const std::vector<armnn::BindingPointInfo>& outputBindings,
                                       std::vector<TContainer>& outputDataContainers)
{
    armnn::OutputTensors outputTensors;

    const size_t numOutputs = outputBindings.size();
    if (numOutputs != outputDataContainers.size())
    {
        throw armnn::Exception(boost::str(boost::format("Number of outputs does not match number of "
                                                        "tensor data containers: %1% != %2%")
                                          % numOutputs
                                          % outputDataContainers.size()));
    }

    for (size_t i = 0; i < numOutputs; i++)
    {
        const armnn::BindingPointInfo& outputBinding = outputBindings[i];
        TContainer& outputData = outputDataContainers[i];

        boost::apply_visitor([&](auto&& value)
        {
            if (value.size() != outputBinding.second.GetNumElements())
            {
                throw armnn::Exception("Output tensor has incorrect size");
            }

            armnn::Tensor outputTensor(outputBinding.second, value.data());
            outputTensors.push_back(std::make_pair(outputBinding.first, outputTensor));
        },
        outputData);
    }

    return outputTensors;
}
