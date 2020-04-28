//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Vision-based object-detection implementation

#pragma once

#include <chrono>
#include <iostream>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>

#include "armnn_yolo2tiny.hpp"
#include "detector.hpp"
#include "detector_armnn.hpp"
#include "profile.hpp"

namespace object_detection {

struct RectClassScore {
  float x;    // x-coordinate of bounding box
  float y;    // y-coordinate of bounding box
  float w;    // bounding box width
  float h;    // bounding box height
  float obj;  // objectness score
  float prob; // detection probability of class i
  size_t i;   // class index
  std::string class_name;
};

std::vector<RectClassScore>
process_detections(std::vector<detection> &detections_raw,
                   float min_confidence);

template <typename T> class VisionDetector : public IDetector<T> {
public:
  VisionDetector(std::vector<std::string> &classes)
      : delegate(*this), classes(classes) {}
  VisionDetector(IDetector<T> &delegate, std::vector<std::string> &classes)
      : delegate(delegate), classes(classes) {}

  std::vector<T> process_image(const cv::Mat &input) const;
  std::vector<RectClassScore> run_inference(const cv::Mat &input_image) const;
  std::vector<detection> run_inference(const std::vector<T> &input) const {
    return this->delegate.run_inference(input);
  }

  float iou_threshold = 0.4;
  float nms_threshold = 0.6;

protected:
  IDetector<T> &delegate;
  std::vector<std::string> &classes;
};

template <typename T>
std::vector<T> VisionDetector<T>::process_image(const cv::Mat &input) const {
  cv::Mat temp = input;
  cv::Mat flat;

  // Compute the aspect ratio and padding
  double scale_x = (double)input.size().width / delegate.input_width;
  double scale_y = (double)input.size().height / delegate.input_height;
  double scale = std::max(scale_x, scale_y);

  if (scale != 1) {
    // Resizing required
    cv::resize(temp, temp, cv::Size(), 1.0f / scale, 1.0f / scale);
  }

  size_t w_pad = delegate.input_width - temp.size().width;
  size_t h_pad = delegate.input_height - temp.size().height;

  if (w_pad || h_pad) {
    // Padding required
    cv::copyMakeBorder(temp, temp, h_pad / 2, (h_pad - h_pad / 2), w_pad / 2,
                       (w_pad - w_pad / 2), cv::BORDER_CONSTANT,
                       cv::Scalar(0, 0, 0));
  }

  // Transform RGB space into DataType
  if (std::is_same<T, float>::value) {
    temp.convertTo(temp, CV_32FC3, 1 / 255.0f);
  } else {
    std::cout << "ERROR: no matching result type";
  }

  // Flatten input image into inference input format
  flat = temp.reshape(1, delegate.input_width * delegate.input_height *
                             delegate.input_depth);
  if (!flat.isContinuous()) {
    std::cerr << "NOT FLAT." << std::endl;
  }

  return flat.isContinuous() ? flat : flat.clone();
}

template <typename T>
std::vector<RectClassScore>
VisionDetector<T>::run_inference(const cv::Mat &input_image) const {
  if (delegate.input_height == 0 || delegate.input_width == 0) {
    return std::vector<RectClassScore>();
  }

  Profiler profiler{};
  profiler.start();

  // Convert input image into tensor
  std::vector<T> input_tensor = process_image(input_image);

  profiler.end("Image pre-processing");
  profiler.start();

  // Run inference using the underlying model
  std::vector<detection> detections_raw = delegate.run_inference(input_tensor);

  profiler.end("Inference + Tensor post-processing");
  profiler.start();

  // Suppress non-maximum detection duplicates
  non_maximum_suppression(detections_raw, nms_threshold, iou_threshold);

  // Parse raw detections_raw into {bbox, label, confidence}
  std::vector<RectClassScore> result =
      object_detection::process_detections(detections_raw, iou_threshold);

  // Compute the aspect ratio and padding
  double scale_x = (double)input_image.size().width / delegate.input_width;
  double scale_y = (double)input_image.size().height / delegate.input_height;
  double scale = std::max(scale_x, scale_y);

  size_t w_scaled = input_image.size().width / scale;
  size_t h_scaled = input_image.size().height / scale;
  size_t w_pad = delegate.input_width - w_scaled;
  size_t h_pad = delegate.input_height - h_scaled;

  // Process RectClassScore results
  for (auto it = result.begin(); it != result.end(); it++) {
    // Compute absolute coordinates corrected for padding offset
    it->x = it->x * delegate.input_width - w_pad / 2;
    it->y = it->y * delegate.input_height - h_pad / 2;
    it->h = it->h * delegate.input_height;
    it->w = it->w * delegate.input_width;

    // Scale coordinates to input size
    it->x = it->x * scale;
    it->y = it->y * scale;
    it->h = it->h * scale;
    it->w = it->w * scale;

    // Add class labels
    if (it->i <= classes.size()) {
      it->class_name = classes[it->i];
    }
  }

  profiler.end("Detection post-processing");
  profiler.start();

  return result;
}

} // namespace object_detection