//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Tests for ArmNN YOLO v2 Tiny implementation

#include <armnn/ArmNN.hpp>
#include <armnn/TypesUtils.hpp>
#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <vector>

#include "vision_detector.hpp"

int main(int argc, char *argv[]) {
  // Enumerate Compute Device backends
  std::vector<armnn::BackendId> computeDevices;
  computeDevices.push_back(armnn::Compute::GpuAcc);
  computeDevices.push_back(armnn::Compute::CpuAcc);
  computeDevices.push_back(armnn::Compute::CpuRef);

  // Create ArmNN Runtime
  armnn::IRuntime::CreationOptions options;
  options.m_EnableGpuProfiling = false;
  armnn::IRuntimePtr runtime = armnn::IRuntime::Create(options);

  // Model parameters
  using DataType = float;
  std::string pretrained_model_file = "models/yolo_v2_tiny.pb";
  std::string pretrained_names_file = "models/coco.names";

  // Load class names
  std::ifstream pretrained_names(pretrained_names_file);
  std::vector<std::string> names;
  std::string line;
  while (std::getline(pretrained_names, line)) {
    names.push_back(line);
  }
  pretrained_names.close();

  // Create detector instance and load network
  detector_armnn::Yolo2TinyDetector<DataType> yolo(runtime);
  yolo.load_network(pretrained_model_file, computeDevices);
  object_detection::VisionDetector<DataType> vision_detector(yolo, names);

  cv::Mat image = cv::imread("test/test_input.png", CV_LOAD_IMAGE_COLOR);

  if (image.size().width == 0 || image.size().height == 0) {
    std::cerr << "Failed to load test/test_input.png" << std::endl;
    return 1;
  }

  // Convert color from BGR to RGB
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

  // Run inference on inputData
  std::vector<object_detection::RectClassScore> results =
      vision_detector.run_inference(image);

  return results.size() != 5;
}