//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Common objection-detection interface definition

#pragma once

#include <fstream>
#include <iostream>
#include <vector>

float sigmoid(float x);

namespace object_detection {

std::vector<float> from_file(std::string filename, size_t n);
void to_file(std::string filename, const std::vector<float> &data);

struct detection {
  float x;   // x coord center of bounding box (between 0, 1)
  float y;   // y coord center of bounding box (between 0, 1)
  float h;   // height
  float w;   // width
  float p_0; // objectness score

  size_t n_c; // number of classes (length of p_i)
  float *p_i; // probability of class i

  detection();
  detection(const detection &src);
  detection(detection &&src);
  detection &operator=(const detection &src);
  detection &operator=(detection &&src);
  ~detection();
};

template <typename T> class IDetector {
public:
  IDetector() {}
  virtual std::vector<detection>
  run_inference(const std::vector<T> &input) const = 0;

  size_t input_height = 0;
  size_t input_width = 0;
  size_t input_depth = 0;

  size_t n_coords = 4;   // number of coordinates in the model
  size_t n_classes = 20; // number of classes in the model

protected:
};

float intersect_over_union(const detection &bbox1, const detection &bbox2);
bool nms_class_comparator(const detection &d1, const detection &d2);
void non_maximum_suppression(std::vector<detection> &detections,
                             float nms_threshold, float iou_threshold);

} // namespace object_detection