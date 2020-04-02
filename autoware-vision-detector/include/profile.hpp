//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Simple timing and profiling macros

#pragma once
#include <chrono>

#if defined(NO_ROS) && (NO_ROS == 1)
#include <iostream>
#define ROS_DEBUG_STREAM(fmt) std::cout << fmt
#else
#include <ros/console.h>
#endif

#define PROFILE_MODULE "[ vision_detector ] "
#if !defined(VISION_DETECTOR_DISABLE_PROFILE) ||                               \
    (VISION_DETECTOR_DISABLE_PROFILE == 0)
#define PROFILER_ON 1
#else
#define PROFILER_ON 0
#endif

class Profiler {
public:
  void start() {
    if (PROFILER_ON) {
      profile_ref = std::chrono::high_resolution_clock::now();
    }
  }

  void end(std::string phase) {
    if (PROFILER_ON) {
      auto profile_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::high_resolution_clock::now() - profile_ref)
              .count();
      ROS_DEBUG_STREAM(PROFILE_MODULE << phase << " : " << profile_ms << " ms"
                                      << std::endl);
    }
  }

private:
  std::chrono::high_resolution_clock::time_point profile_ref{};
};
