//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Tests for image-to-(flattened-)tensor preprocessing functions

#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <vector>

#include "vision_detector.hpp"

int main(int argc, char *argv[]) {
  // testcase parameters
  const size_t network_width = 416;
  const size_t network_height = 416;
  const size_t network_depth = 3;
  const size_t input_image_width = 128;
  const size_t input_image_height = 127;

  // Model parameters
  using DataType = float;

  std::vector<std::string> names;

  // Create detector instance
  object_detection::VisionDetector<DataType> vision_detector(names);
  vision_detector.input_width = network_width;
  vision_detector.input_height = network_height;
  vision_detector.input_depth = network_depth;

  cv::Mat3b input_image(input_image_width, input_image_height);
  cv::randu(input_image, cv::Scalar::all(0), cv::Scalar::all(255));

  // Run preprocessing stage
  std::vector<DataType> input_tensor =
      vision_detector.process_image(input_image);
  std::cerr << "Size of input_tensor: " << input_tensor.size() << std::endl;

  return input_tensor.size() != network_width * network_height * network_depth;
}
