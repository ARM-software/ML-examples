//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//
#include "model_output_labels_loader.hpp"

#include "armnn/BackendId.hpp"
#include "armnn/IRuntime.hpp"
#include "armnnTfLiteParser/ITfLiteParser.hpp"
#include "backendsCommon/BackendRegistry.hpp"
#include "ImageTensorGenerator/ImageTensorGenerator.hpp"
#include "InferenceTest.hpp"
#include "TensorIOUtils.hpp"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/variant.hpp>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

using namespace armnn::test;

struct ProgramOptions
{
    std::string modelPath;
    std::string imagePath;
    std::string modelOutputLabelsPath;
    std::vector<armnn::BackendId> computeDevice;
};

/** Parse and get program options from command line arguments
 *
 * @param[in] argc    Number of command line arguments
 * @param[in] argv    Array of command line arguments
 * @param[in] options Program options
 * @return Status
 */
int GetProgramOptions(int argc, char* argv[], ProgramOptions& options)
{
    namespace po = boost::program_options;
    std::vector<armnn::BackendId> defaultBackends = {armnn::Compute::CpuAcc, armnn::Compute::CpuRef};

    const std::string backendsMessage = "Which device to run layers on by default. Possible choices: "
        + armnn::BackendRegistryInstance().GetBackendIdsAsString();

    po::options_description desc("Options");

    desc.add_options()
        ("help,h", "Display help messages")
        ("model-path,m", po::value<std::string>(&options.modelPath)->required(), "Path to armnn format model file")
        ("data-dir,d", po::value<std::string>(&options.imagePath)->required(),
         "Path to directory containing the ImageNet test data")
        ("model-output-labels,p", po::value<std::string>(&options.modelOutputLabelsPath)->required(),
         "Path to model output labels file.")
        ("compute,c", po::value<std::vector<armnn::BackendId>>(&options.computeDevice)->default_value(defaultBackends),
         backendsMessage.c_str());

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return EXIT_FAILURE;
        }
        po::notify(vm);
    }
    catch (po::error& e)
    {
        std::cerr << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    armnn::LogSeverity level = armnn::LogSeverity::Info;
    armnn::ConfigureLogging(true, true, level);

    const std::string inputName = "input";
    const std::string outputName = "MobilenetV1/Predictions/Reshape_1";
    const unsigned int inputTensorWidth = 224;
    const unsigned int inputTensorHeight = 224;
    const unsigned int inputTensorBatchSize = 1;
    const armnn::DataLayout inputTensorDataLayout = armnn::DataLayout::NHWC;

    // ------------------------------------------------------------------------
    // Get program options
    // ------------------------------------------------------------------------

    ProgramOptions programOptions;
    if (GetProgramOptions(argc, argv, programOptions) != 0)
    {
        return EXIT_FAILURE;
    }

    // ------------------------------------------------------------------------
    // Load model output labels
    // ------------------------------------------------------------------------

    const std::vector<CategoryNames> modelOutputLabels =
        LoadModelOutputLabels(programOptions.modelOutputLabelsPath);

    // ------------------------------------------------------------------------
    // Load and preprocess input image
    // ------------------------------------------------------------------------

    using TContainer = boost::variant<std::vector<uint8_t>>;

    // Prepare image normalization parameters
    NormalizationParameters normParams;
    normParams.scale = 1.0;
    normParams.mean = { 0.0, 0.0, 0.0 };
    normParams.stddev = { 1.0, 1.0, 1.0 };

    // Load and preprocess input image
    const std::vector<TContainer> inputDataContainers =
    { PrepareImageTensor<uint8_t>(programOptions.imagePath,
            inputTensorWidth, inputTensorHeight,
            normParams,
            inputTensorBatchSize,
            inputTensorDataLayout) };

    // ------------------------------------------------------------------------
    // Prepare output tensor
    // ------------------------------------------------------------------------

    // Output tensor size is equal to the number of model output labels
    const unsigned int outputNumElements = modelOutputLabels.size();
    std::vector<TContainer> outputDataContainers = { std::vector<uint8_t>(outputNumElements) };

    // ------------------------------------------------------------------------
    // Import graph
    // ------------------------------------------------------------------------

    // Import the TensorFlowLite model.
    using IParser = armnnTfLiteParser::ITfLiteParser;
    auto armnnparser(IParser::Create());
    armnn::INetworkPtr network = armnnparser->CreateNetworkFromBinaryFile(programOptions.modelPath.c_str());

    // Find the binding points for the input and output nodes
    using BindingPointInfo = armnnTfLiteParser::BindingPointInfo;
    const std::vector<BindingPointInfo> inputBindings  = { armnnparser->GetNetworkInputBindingInfo(0, inputName) };
    const std::vector<BindingPointInfo> outputBindings = { armnnparser->GetNetworkOutputBindingInfo(0, outputName) };

    // ------------------------------------------------------------------------
    // Optimize graph and load the optimized graph onto a compute device
    // ------------------------------------------------------------------------

    // Optimize the network for a specific runtime compute device, e.g. CpuAcc, GpuAcc
    armnn::IRuntime::CreationOptions options;
    armnn::IRuntimePtr runtime(armnn::IRuntime::Create(options));
    armnn::IOptimizedNetworkPtr optimizedNet =
        armnn::Optimize(*network, programOptions.computeDevice, runtime->GetDeviceSpec());

    // Load the optimized network onto the runtime device
    armnn::NetworkId networkId;
    runtime->LoadNetwork(networkId, std::move(optimizedNet));

    // ------------------------------------------------------------------------
    // Run graph on device
    // ------------------------------------------------------------------------

    std::cout << "Running network..." << std::endl;
    runtime->EnqueueWorkload(networkId,
            armnnUtils::MakeInputTensors(inputBindings, inputDataContainers),
            armnnUtils::MakeOutputTensors(outputBindings, outputDataContainers));

    // ------------------------------------------------------------------------
    // Process and report output
    // ------------------------------------------------------------------------

    std::vector<uint8_t> output = boost::get<std::vector<uint8_t>>(outputDataContainers[0]);

    size_t labelInd = std::distance(output.begin(), std::max_element(output.begin(), output.end()));
    std::cout << "Prediction: ";
    for (const auto& label : modelOutputLabels[labelInd])
    {
        std::cout << label << ", ";
    }
    std::cout << std::endl;

    std::cerr<< "Ran successfully!" << std::endl;
    return 0;
}
