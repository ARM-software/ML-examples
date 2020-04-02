//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Standalone tool for testing vision-based object detection algorithms

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

void draw_box(cv::Mat &image, const std::string &label, const cv::Point &p1,
              const cv::Point &p2, const cv::Scalar &color, float scale = 1) {
  // Label configuration
  int text_font = cv::FONT_HERSHEY_DUPLEX;
  int text_thickness = 1;
  int text_baseline = 0;
  int text_margin = 5;

  // Draw bounding box
  cv::rectangle(image, p1, p2, color);

  // Compute label size and background rectangle
  cv::Size labelSize =
      cv::getTextSize(label, text_font, scale, text_thickness, &text_baseline);
  cv::Point l_p1(p1.x, p1.y - labelSize.height - text_margin * 2);
  cv::Point l_p2(p1.x + labelSize.width + text_margin * 2, p1.y);
  cv::Point l_t(p1.x + text_margin, p1.y - text_margin);

  // Draw label
  cv::rectangle(image, l_p1, l_p2, color, CV_FILLED);
  cv::putText(image, label, cv::Point(p1.x + 5, p1.y - 5), text_font, scale,
              cv::Scalar(0, 0, 0), text_thickness);
}

int main(int argc, const char *argv[]) {
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
  if (argc != 3) {
    std::cout << "Usage: vision_detector_tool <yolo_v2_tiny.pb> <coco.names>"
              << std::endl;
    return 0;
  }
  std::string pretrained_model_file(argv[1]);
  std::string pretrained_names_file(argv[2]);

  // Load class names
  std::ifstream pretrained_names(pretrained_names_file);
  std::vector<std::string> names;
  std::string line;
  while (std::getline(pretrained_names, line)) {
    names.push_back(line);
  }
  pretrained_names.close();
  std::cout << "Loaded class names: " << pretrained_names_file << std::endl;

  // Create detector instance and load network
  detector_armnn::Yolo2TinyDetector<DataType> yolo(runtime);
  yolo.load_network(pretrained_model_file, computeDevices);
  object_detection::VisionDetector<DataType> vision_detector(yolo, names);
  std::cout << "Loaded network: " << pretrained_model_file << std::endl;

  std::string image_file;
  std::cout << std::endl << "Image file: ";
  std::cin >> image_file;

  while (image_file != "") {
    // Load image
    cv::Mat image = cv::imread(image_file, CV_LOAD_IMAGE_COLOR);

    // Convert color from BGR to RGB
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

    // Run inference on inputData
    std::vector<object_detection::RectClassScore> results =
        vision_detector.run_inference(image);

    // Print detections
    std::cout << "Detections: " << results.size() << std::endl;
    for (size_t i = 0; i < results.size(); i++) {
      object_detection::RectClassScore &r = results[i];
      std::cout << "[" << i << "] : ";
      std::cout << r.obj << "%";
      std::cout << "\t(" << r.i << ")\t" << r.class_name << " : " << r.prob
                << " %";
      std::cout << " (" << r.w << "x" << r.h << ") @ (" << r.x << ", " << r.y
                << ")" << std::endl;

      // Compute bounding box
      int x1 = r.x;
      int y1 = r.y;
      int x2 = r.x + r.w;
      int y2 = r.y + r.h;

      double fontScale = 0.5;
      std::string label =
          r.class_name + " : " + std::to_string((int)(r.prob * 100)) + "%";

      // Draw bounding box
      draw_box(image, label, cv::Point(x1, y1), cv::Point(x2, y2),
               cv::Scalar(0, 200, 200), fontScale);
    }

    // OpenCV uses BGR colour encoding
    cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
    cv::imwrite("predictions.png", image);

    std::cout << std::endl << "Image file: ";
    std::cin >> image_file;
  }

  return 0;
}
