//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Load and run a pretrained YOLOv2-tiny model on ArmNN

#pragma once

#include <cmath>
#include <map>

#include "detector_armnn.hpp"
#include "profile.hpp"

namespace detector_armnn {

template <typename T> class Yolo2TinyDetector : public YoloDetector<T> {
public:
  Yolo2TinyDetector(armnn::IRuntimePtr &runtime) : YoloDetector<T>(runtime) {
    this->input_height = 416;
    this->input_width = 416;
    this->input_depth = 3;

    this->n_classes = 80;
    this->output_depth =
        (this->n_coords + 1 + this->n_classes) * this->n_anchors;
    this->input_tensor_shape = armnn::TensorShape(
        {1, (unsigned int)this->input_height, (unsigned int)this->input_width,
         (unsigned int)this->input_depth});
    this->output_tensor_shape =
        armnn::TensorShape({1, 13, 13, (unsigned int)this->output_depth});

    // Anchor values for COCO dataset (width, height)
    this->anchors.push_back(std::pair<float, float>(0.57273f, 0.677385f));
    this->anchors.push_back(std::pair<float, float>(1.87446f, 2.06253f));
    this->anchors.push_back(std::pair<float, float>(3.33843f, 5.47434f));
    this->anchors.push_back(std::pair<float, float>(7.88282f, 3.52778f));
    this->anchors.push_back(std::pair<float, float>(9.77052f, 9.16828f));
  }

  std::vector<object_detection::detection>
  run_inference(const std::vector<T> &input_tensor) const;
  std::vector<object_detection::detection>
  process_output(std::vector<T> &output_tensor) const;
  void load_network(const std::string &model_path,
                    const std::vector<armnn::BackendId> &compute_devices);
  ~Yolo2TinyDetector() {}

protected:
  armnn::TensorShape output_tensor_shape;
  size_t output_depth = 0; // values per detection 'cell' in output layer
  size_t n_anchors = 5;    // number of anchoring bounding-boxes
  std::vector<std::pair<float, float>> anchors;
};

template <typename T>
std::vector<object_detection::detection>
Yolo2TinyDetector<T>::run_inference(const std::vector<T> &input_tensor) const {
  // Validate input dimensions
  if (this->input_tensor_shape.GetNumElements() != input_tensor.size()) {
    throw armnn::Exception("Input size mismatch");
  }

  // Allocate output container
  size_t output_size = this->output_tensor_shape.GetNumElements();
  std::vector<T> output(output_size);

  // Create input and output tensors and their bindings
  armnn::InputTensors inputTensors{
      {0,
       armnn::ConstTensor(this->runtime->GetInputTensorInfo(this->networkID, 0),
                          input_tensor.data())}};
  armnn::OutputTensors outputTensors{
      {0, armnn::Tensor(this->runtime->GetOutputTensorInfo(this->networkID, 0),
                        output.data())}};

  Profiler profiler{};
  profiler.start();

  // Run inference
  this->runtime->EnqueueWorkload(this->networkID, inputTensors, outputTensors);

  profiler.end("  Raw inference in ArmNN");
  profiler.start();

  std::vector<object_detection::detection> result = process_output(output);

  profiler.end("  Tensor post-processing");

  return result;
}

template <typename T>
std::vector<object_detection::detection>
Yolo2TinyDetector<T>::process_output(std::vector<T> &output_tensor) const {

  const size_t l_h = output_tensor_shape[1]; // layer height
  const size_t l_w = output_tensor_shape[2]; // layer width

  // Create a result container for (n_anchors * n_classes * width * height)
  // detections
  size_t num_detections = this->n_classes * this->n_anchors * l_w * l_h;
  std::vector<object_detection::detection> result(num_detections);
  size_t i_result = 0; // index of current detection result

  // Parse results into detection vector
  // For each detection cell in the model output
  for (size_t c_y = 0; c_y < l_h; c_y++) {
    for (size_t c_x = 0; c_x < l_w; c_x++) {
      for (size_t anchor_index = 0; anchor_index < this->n_anchors;
           anchor_index++) {
        float anchor_w = this->anchors[anchor_index].first;
        float anchor_h = this->anchors[anchor_index].second;

        // Compute property indices
        size_t i_box_x = this->get_result_index(this->output_tensor_shape,
                                                anchor_index, c_x, c_y, 0);
        size_t i_box_y = this->get_result_index(this->output_tensor_shape,
                                                anchor_index, c_x, c_y, 1);
        size_t i_box_w = this->get_result_index(this->output_tensor_shape,
                                                anchor_index, c_x, c_y, 2);
        size_t i_box_h = this->get_result_index(this->output_tensor_shape,
                                                anchor_index, c_x, c_y, 3);
        size_t i_box_p = this->get_result_index(this->output_tensor_shape,
                                                anchor_index, c_x, c_y, 4);
        size_t n_classes = this->n_classes;

        // Transform log-space predicted coordinates to absolute space + offset
        // Transform bounding box position from offset to absolute (ratio)
        output_tensor[i_box_x] = (sigmoid(output_tensor[i_box_x]) + c_x) / l_w;
        output_tensor[i_box_y] = (sigmoid(output_tensor[i_box_y]) + c_y) / l_h;

        // Transform bounding box height and width from log to absolute space
        output_tensor[i_box_w] = anchor_w * exp(output_tensor[i_box_w]) / l_w;
        output_tensor[i_box_h] = anchor_h * exp(output_tensor[i_box_h]) / l_h;

        // Assemble a detection object
        object_detection::detection r;
        r.x = output_tensor[i_box_x];
        r.y = output_tensor[i_box_y];
        r.w = output_tensor[i_box_w];
        r.h = output_tensor[i_box_h];
        r.p_0 = sigmoid(output_tensor[i_box_p]);
        r.n_c = n_classes;
        r.p_i = new float[n_classes];

        // Copy class probabilities
        float p_total = 0;
        for (size_t i_class = 0; i_class < n_classes; i_class++) {
          size_t i_prob_class = this->get_result_index(
              this->output_tensor_shape, anchor_index, c_x, c_y, 5 + i_class);
          r.p_i[i_class] = std::exp(output_tensor[i_prob_class]);
          p_total += std::exp(output_tensor[i_prob_class]);
        }

        // Softmax
        for (size_t i_class = 0; i_class < n_classes; i_class++) {
          r.p_i[i_class] /= p_total;
        }

        // Next item
        result[i_result] = std::move(r);
        i_result++;
      }
    }
  }

  return result;
}

template <typename T>
void Yolo2TinyDetector<T>::load_network(
    const std::string &model_path,
    const std::vector<armnn::BackendId> &compute_devices) {
  // Create input and output shape structures
  std::map<std::string, armnn::TensorShape> inputShapes;
  std::vector<std::string> requestedOutputs;
  inputShapes.insert(std::pair<std::string, armnn::TensorShape>(
      "input", this->input_tensor_shape));
  requestedOutputs.push_back("output");

  // Setup ArmNN Network
  // Parse pre-trained model from TensorFlow protobuf format
  using ParserType = armnnTfParser::ITfParser;
  auto parser(ParserType::Create());
  armnn::INetworkPtr network{nullptr, [](armnn::INetwork *) {}};
  network = parser->CreateNetworkFromBinaryFile(model_path.c_str(), inputShapes,
                                                requestedOutputs);

  // Network loaded successfully
  this->ArmnnDetector::load_network(std::move(network), compute_devices);
}

} // namespace detector_armnn