//
// Copyright © 2017 Arm Ltd. All rights reserved.
// See LICENSE file in the project root for full license information.
//

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include <array>
#include <algorithm>
#include "armnn/ArmNN.hpp"
#include "armnn/Exceptions.hpp"
#include "armnn/Tensor.hpp"
#include "armnn/INetwork.hpp"
#include "armnnCaffeParser/ICaffeParser.hpp"

#include "mnist_loader.hpp"


// Helper function to make input tensors
armnn::InputTensors MakeInputTensors(const std::pair<armnn::LayerBindingId,
    armnn::TensorInfo>& input,
    const void* inputTensorData)
{
    return { { input.first, armnn::ConstTensor(input.second, inputTensorData) } };
}

// Helper function to make output tensors
armnn::OutputTensors MakeOutputTensors(const std::pair<armnn::LayerBindingId,
    armnn::TensorInfo>& output,
    void* outputTensorData)
{
    return { { output.first, armnn::Tensor(output.second, outputTensorData) } };
}

int main(int argc, char** argv)
{
    // Load a test image and its correct label
    std::string dataDir = "data/";
    int testImageIndex = 0;
    std::unique_ptr<MnistImage> input = loadMnistImage(dataDir, testImageIndex);
    if (input == nullptr)
        return 1;

    // Import the Caffe model. Note: use CreateNetworkFromTextFile for text files.
    armnnCaffeParser::ICaffeParserPtr parser = armnnCaffeParser::ICaffeParser::Create();
    armnn::INetworkPtr network = parser->CreateNetworkFromBinaryFile("model/lenet_iter_9000.caffemodel",
                                                                   { }, // input taken from file if empty
                                                                   { "prob" }); // output node

    // Find the binding points for the input and output nodes
    armnnCaffeParser::BindingPointInfo inputBindingInfo = parser->GetNetworkInputBindingInfo("data");
    armnnCaffeParser::BindingPointInfo outputBindingInfo = parser->GetNetworkOutputBindingInfo("prob");

    // Optimize the network for a specific runtime compute device, e.g. CpuAcc, GpuAcc
    armnn::IRuntimePtr runtime = armnn::IRuntime::Create(armnn::Compute::CpuAcc);
    armnn::IOptimizedNetworkPtr optNet = armnn::Optimize(*network, runtime->GetDeviceSpec());

    // Load the optimized network onto the runtime device
    armnn::NetworkId networkIdentifier;
    runtime->LoadNetwork(networkIdentifier, std::move(optNet));

    // Run a single inference on the test image
    std::array<float, 10> output;
    armnn::Status ret = runtime->EnqueueWorkload(networkIdentifier,
                                                 MakeInputTensors(inputBindingInfo, &input->image[0]),
                                                 MakeOutputTensors(outputBindingInfo, &output[0]));

    // Convert 1-hot output to an integer label and print
    int label = std::distance(output.begin(), std::max_element(output.begin(), output.end()));
    std::cout << "Predicted: " << label << std::endl;
    std::cout << "   Actual: " << input->label << std::endl;
    return 0;
}
