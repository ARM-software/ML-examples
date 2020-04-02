//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Accelerated object detection algorithms on ArmNN

#pragma once

#include <armnn/ArmNN.hpp>
#include <armnnTfParser/ITfParser.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <vector>

#include "detector.hpp"

namespace detector_armnn {

class ArmnnDetector {
public:
  ArmnnDetector(armnn::IRuntimePtr &runtime) : runtime(runtime) {}
  virtual void
  load_network(armnn::INetworkPtr network,
               const std::vector<armnn::BackendId> &compute_devices);

protected:
  armnn::IRuntimePtr &runtime;
  armnn::NetworkId networkID = 0;
  armnn::TensorShape input_tensor_shape;
};

template <typename T>
class YoloDetector : public ArmnnDetector,
                     public object_detection::IDetector<T> {
public:
  YoloDetector(armnn::IRuntimePtr &runtime)
      : ArmnnDetector(runtime), object_detection::IDetector<T>() {}

  // Compute index to access the specified property of the flattened output
  // array
  size_t get_result_index(const armnn::TensorShape &tensor_shape,
                          size_t anchor_index, size_t cell_x, size_t cell_y,
                          size_t cell_index) const;

protected:
};

// Index of cell attribute (at cell index) in a flattened array
template <typename T>
size_t YoloDetector<T>::get_result_index(const armnn::TensorShape &tensor_shape,
                                         size_t anchor_index, size_t cell_x,
                                         size_t cell_y,
                                         size_t cell_index) const {
  size_t cell_depth = tensor_shape[3];
  size_t cell_stride = tensor_shape[2];
  return anchor_index * (this->n_coords + 1 + this->n_classes) +
         cell_depth * cell_stride * cell_y + cell_depth * cell_x + cell_index;
}

} // namespace detector_armnn
