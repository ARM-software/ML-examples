//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Common objection-detection interface implementation

#include <algorithm>
#include <cmath>
#include <cstring>

#include "detector.hpp"

#define FP_EPSILON 0.0005

using namespace object_detection;

float sigmoid(float x) { return (float)(1.0 / (1.0 + std::exp(-x))); }

std::vector<float> object_detection::from_file(std::string filename, size_t n) {
  std::ifstream inputFile(filename);
  std::vector<float> inputData;
  size_t i = 0;
  for (i = 0; i < n && !inputFile.eof(); i++) {
    float f = 0;
    inputFile >> f;
    if (i < 10)
      std::cout << f << std::endl;
    inputData.push_back(f);
  }
  std::cout << "Read: " << i << " / " << n << std::endl;
  inputFile.close();
  return inputData;
}

void object_detection::to_file(std::string filename,
                               const std::vector<float> &data) {
  std::ofstream outputFile(filename);
  size_t i = 0;
  for (i = 0; i < data.size(); i++) {
    outputFile << data[i] << std::endl;
  }
  std::cout << "Wrote: " << i << std::endl;
  outputFile.close();
}

detection::detection() {
  this->x = 0;
  this->y = 0;
  this->h = 0;
  this->w = 0;
  this->p_0 = 0;

  this->n_c = 0;
  this->p_i = nullptr;
}

detection::detection(const detection &src) {
  this->x = src.x;
  this->y = src.y;
  this->h = src.h;
  this->w = src.w;
  this->p_0 = src.p_0;
  this->n_c = src.n_c;

  this->p_i = new float[n_c];
  std::memcpy(this->p_i, src.p_i, n_c);
}

detection::detection(detection &&src) {
  this->x = src.x;
  this->y = src.y;
  this->h = src.h;
  this->w = src.w;
  this->p_0 = src.p_0;
  this->n_c = src.n_c;

  this->p_i = src.p_i;
  src.n_c = 0;
  src.p_i = nullptr;
}

detection &detection::operator=(const detection &src) {
  if (&src == this) {
    return *this;
  }
  this->x = src.x;
  this->y = src.y;
  this->h = src.h;
  this->w = src.w;
  this->p_0 = src.p_0;
  this->n_c = src.n_c;

  this->p_i = new float[n_c];
  std::memcpy(this->p_i, src.p_i, n_c);
  return *this;
}

detection &detection::operator=(detection &&src) {
  this->x = src.x;
  this->y = src.y;
  this->h = src.h;
  this->w = src.w;
  this->p_0 = src.p_0;
  this->n_c = src.n_c;

  this->p_i = src.p_i;
  src.n_c = 0;
  src.p_i = nullptr;
  return *this;
}

detection::~detection() {
  if (p_i != nullptr)
    delete[] p_i;
  p_i = nullptr;
}

float object_detection::intersect_over_union(const detection &bbox1,
                                             const detection &bbox2) {
  float x1 = std::max(bbox1.x, bbox2.x);
  float y1 = std::max(bbox1.y, bbox2.y);
  float x2 = std::min(bbox1.x + bbox1.w, bbox2.x + bbox2.w);
  float y2 = std::min(bbox1.y + bbox1.h, bbox2.y + bbox2.h);
  float intersection = (y2 > y1 && x2 > x1) ? (y2 - y1) * (x2 - x1) : 0.0f;
  float _union = bbox1.h * bbox1.w + bbox2.h * bbox2.w - intersection;
  if (_union < FP_EPSILON) {
    return 0;
  }
  return intersection / _union;
}

// Sort detections by class index (global within this file)
static size_t global_nms_class_comparator_current_class;

bool object_detection::nms_class_comparator(const detection &d1,
                                            const detection &d2) {
  if (d2.p_i == nullptr)
    return true;
  if (d1.p_i == nullptr)
    return false;

  size_t class_id = global_nms_class_comparator_current_class;
  return d1.p_i[class_id] < d2.p_i[class_id];
}

void object_detection::non_maximum_suppression(
    std::vector<detection> &detections, float nms_threshold,
    float iou_threshold) {
  // Filter detections with objectness score above nms_threshold
  std::vector<detection>::iterator current =
      detections.begin(); // currently evaluated detection
  std::vector<detection>::iterator end =
      detections.end() - 1; // last non-zero objectness score
  for (; current <= end;) {
    if (current->p_0 > nms_threshold) {
      ++current;
      continue;
    } else {
      // Annul detection if nms_threshold is not met
      current->p_0 = 0;
      // Move annulled detection to back of vector
      std::swap(*current, *end);
      --end;
    }
  }

  // Any detections after `end` have zero objectiveness score
  ++end;

  // Greedily unify same-class detections with overlapping bounding boxes
  size_t num_classes = detections[0].n_c;
  for (size_t class_id = 0; class_id < num_classes; class_id++) {
    // Sort detections by objectiveness score
    global_nms_class_comparator_current_class = class_id;
    std::sort(detections.begin(), end, nms_class_comparator);

    for (auto primary = detections.begin(); primary != end; primary++) {
      // Continue if no primary detections left for this class
      if (primary->p_i[class_id] < FP_EPSILON)
        continue;

      // Suppress overlapping secondary detections of the same class
      for (auto secondary = primary + 1; secondary != end; secondary++) {
        if (intersect_over_union(*primary, *secondary) > iou_threshold) {
          secondary->p_i[class_id] = 0;
        }
      }
    }
  }
}