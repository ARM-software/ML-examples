//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Generic ArmNN-based detector implementation

#include "detector_armnn.hpp"

using namespace detector_armnn;

void ArmnnDetector::load_network(
    armnn::INetworkPtr network,
    const std::vector<armnn::BackendId> &compute_devices) {
  // Set optimisation options
  armnn::OptimizerOptions options;
  options.m_ReduceFp32ToFp16 = false;

  // Optimise network
  armnn::IOptimizedNetworkPtr optNet{nullptr,
                                     [](armnn::IOptimizedNetwork *) {}};
  optNet = armnn::Optimize(*network, compute_devices, runtime->GetDeviceSpec(),
                           options);
  if (!optNet) {
    throw armnn::Exception("armnn::Optimize failed");
  }

  // Load network into runtime
  armnn::Status ret =
      this->runtime->LoadNetwork(this->networkID, std::move(optNet));
  if (ret == armnn::Status::Failure) {
    throw armnn::Exception("IRuntime::LoadNetwork failed");
  }
}