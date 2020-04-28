//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Vision detection implementation

#include <vector>

#include "vision_detector.hpp"

using namespace object_detection;

std::vector<RectClassScore>
object_detection::process_detections(std::vector<detection> &detections_raw,
                                     float min_confidence) {
  std::vector<RectClassScore> result;

  for (auto it = detections_raw.begin(); it != detections_raw.end(); it++) {
    size_t detection_class = 0; // detected class
    float detection_prob = -1;  // detected class probability

    if (it->p_0 <= min_confidence) {
      continue;
    }

    // Determine most probable class
    for (size_t i = 0; i < it->n_c; i++) {
      if (it->p_i[i] >= min_confidence * it->p_0 &&
          it->p_i[i] >= detection_prob) {
        detection_class = i;
        detection_prob = it->p_i[i];
      }
    }

    // Class detection probable: append to results
    if (detection_prob > 0) {
      RectClassScore detection;

      // Translate bbox positions from center-relative to corner-relative
      detection.x = it->x - (it->w / 2);
      detection.y = it->y - (it->h / 2);

      // Copy rectangle, class, and score attributes
      detection.h = it->h;
      detection.w = it->w;
      detection.obj = it->p_0;
      detection.prob = it->p_0 * detection_prob;
      detection.i = detection_class;

      result.push_back(detection);
    }
  }

  return result;
}