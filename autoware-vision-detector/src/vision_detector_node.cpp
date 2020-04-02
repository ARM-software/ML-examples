//
// Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// ROS node that runs object-detection algorithms

#include <autoware_msgs/DetectedObjectArray.h>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <ros/ros.h>
#include <sensor_msgs/image_encodings.h>
#include <vector>

#include "vision_detector.hpp"

template <typename T> class DetectorNode {
public:
  DetectorNode(const std::string &input_topic, const std::string &output_topic,
               const object_detection::VisionDetector<T> &detector)
      : m_detector(detector) {
    nh = ros::NodeHandle("~");
    sub = nh.subscribe(input_topic, 1, &DetectorNode::callback_image, this);
    pub = nh.advertise<autoware_msgs::DetectedObjectArray>(output_topic, 1);
    image_repub = nh.advertise<sensor_msgs::Image>("image_raw", 1);
  }

  void callback_image(const sensor_msgs::Image &image);

protected:
  const object_detection::VisionDetector<T> &m_detector;
  ros::NodeHandle nh;
  ros::Publisher pub;
  ros::Publisher image_repub;
  ros::Subscriber sub;
};

template <typename T>
void DetectorNode<T>::callback_image(const sensor_msgs::Image &image) {
  cv_bridge::CvImagePtr ptr_image =
      cv_bridge::toCvCopy(image, sensor_msgs::image_encodings::RGB8);

  // Run inference on received image
  ROS_INFO("Running inference on image");
  ros::Time start = ros::Time::now();
  std::vector<object_detection::RectClassScore> detections =
      m_detector.run_inference(ptr_image->image);
  int dur_msec = (int)(1000 * (ros::Time::now() - start).toSec());
  ROS_INFO("Inference complete: detected %d objects in %d msec",
           (int)detections.size(), dur_msec);

  // Populate message with detections
  autoware_msgs::DetectedObjectArray result;
  result.header = image.header;
  for (auto det = detections.begin(); det != detections.end(); ++det) {
    autoware_msgs::DetectedObject object;

    // Copy detection attributes
    object.x = det->x;
    object.y = det->y;
    object.width = det->w;
    object.height = det->h;
    object.label = det->class_name;
    object.valid = true;

    result.objects.push_back(object);
  }

  pub.publish(result);
  image_repub.publish(image);
}

int main(int argc, char *argv[]) {
  ros::init(argc, argv, "vision_detector");
  ros::NodeHandle nh("~");

  // Detection parameters
  float nms_threshold, score_threshold;
  nh.param("nms_threshold", nms_threshold, 0.6f);
  ROS_INFO("nms_threshold: %f", nms_threshold);
  nh.param("score_threshold", score_threshold, 0.4f);
  ROS_INFO("score_threshold: %f", score_threshold);

  // Model parameters
  using DataType = float;
  std::string pretrained_model_file;
  std::string pretrained_names_file;
  if (!nh.getParam("pretrained_model_file", pretrained_model_file)) {
    ROS_ERROR("Invalid pretrained model: %s", pretrained_model_file.c_str());
    return -1;
  }
  if (!nh.getParam("names_file", pretrained_names_file)) {
    ROS_ERROR("Invalid pretrained names: %s", pretrained_names_file.c_str());
    return -1;
  }

  // Load class names
  std::ifstream pretrained_names(pretrained_names_file);
  std::vector<std::string> names;
  std::string line;
  while (std::getline(pretrained_names, line)) {
    names.push_back(line);
  }
  pretrained_names.close();
  ROS_INFO("Loaded class names: %s", pretrained_names_file.c_str());

  // Topic parameters
  std::string topic_image_raw = "image_raw";
  nh.param("image_raw_node", topic_image_raw, topic_image_raw);
  ros::names::remap(topic_image_raw);
  ROS_INFO("image_raw_node: %s", topic_image_raw.c_str());
  std::string topic_detection = ros::names::remap("objects");

  // Enumerate Compute Device backends
  std::vector<armnn::BackendId> computeDevices;
  computeDevices.push_back(armnn::Compute::GpuAcc);
  computeDevices.push_back(armnn::Compute::CpuAcc);
  computeDevices.push_back(armnn::Compute::CpuRef);

  // Create ArmNN Runtime
  armnn::IRuntime::CreationOptions options;
  options.m_EnableGpuProfiling = false;
  armnn::IRuntimePtr runtime = armnn::IRuntime::Create(options);

  // Create detector instance and load network
  ROS_INFO("Loading network: %s", pretrained_model_file.c_str());
  detector_armnn::Yolo2TinyDetector<DataType> yolo(runtime);
  yolo.load_network(pretrained_model_file, computeDevices);
  object_detection::VisionDetector<DataType> vision_detector(yolo, names);
  ROS_INFO("Network loaded");

  // Subscribe to raw images with callback bound to vision_detector
  DetectorNode<DataType> detector_node(topic_image_raw, topic_detection,
                                       vision_detector);

  ros::spin();

  return 0;
}