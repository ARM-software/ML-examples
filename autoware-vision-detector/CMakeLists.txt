#
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 2.8.12)
project(vision_detector)

find_package(autoware_build_flags REQUIRED)
find_package(autoware_msgs REQUIRED)
find_package(autoware_config_msgs REQUIRED)

find_package(catkin REQUIRED COMPONENTS
        std_msgs
        sensor_msgs
        autoware_msgs
        autoware_config_msgs
        cv_bridge
        image_transport
        roscpp
        )

find_package(OpenCV REQUIRED)

catkin_package(CATKIN_DEPENDS
        std_msgs
        sensor_msgs
        autoware_msgs
        autoware_config_msgs
        cv_bridge
        image_transport
        roscpp
        )

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-O3 -g -Wall ${CMAKE_CXX_FLAGS}")

set(ARMNN_INSTALL_DIR /opt/armnn)
set(ARMNN_LIB_DIR ${ARMNN_INSTALL_DIR}/lib)
set(ARMNN_INCLUDE_DIRS ${ARMNN_INSTALL_DIR}/include)
add_library(armnn STATIC IMPORTED)
set_target_properties(armnn PROPERTIES IMPORTED_LOCATION ${ARMNN_INSTALL_DIR}/lib/libarmnn.so)
add_library(armnnTfParser STATIC IMPORTED)
set_target_properties(armnnTfParser PROPERTIES IMPORTED_LOCATION ${ARMNN_INSTALL_DIR}/lib/libarmnnTfParser.so)
add_library(protobuf STATIC IMPORTED)
set_target_properties(protobuf PROPERTIES IMPORTED_LOCATION ${ARMNN_INSTALL_DIR}/lib/libprotobuf.so)


add_library(yolo_armnn SHARED
        src/detector.cpp
        src/detector_armnn.cpp
        src/vision_detector.cpp
        )

target_include_directories(yolo_armnn PRIVATE
        ${catkin_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${ARMNN_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries(yolo_armnn
        ${catkin_LIBRARIES}
        ${OpenCV_LIBRARIES}
        armnn
        armnnTfParser
        protobuf
        )

add_dependencies(yolo_armnn
        ${catkin_EXPORTED_TARGETS}
        )

add_executable(vision_detector_tool
        src/vision_detector_tool.cpp
        )

target_include_directories(vision_detector_tool PRIVATE
        ${catkin_INCLUDE_DIRS}
        ${ARMNN_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries(vision_detector_tool
        ${catkin_LIBRARIES}
        ${OpenCV_LIBS}
        armnn
        yolo_armnn
        )

add_dependencies(vision_detector_tool
        ${catkin_EXPORTED_TARGETS}
        yolo_armnn
        )

add_executable(vision_detector
        src/vision_detector_node.cpp
        )

target_include_directories(vision_detector PRIVATE
        ${catkin_INCLUDE_DIRS}
        ${ARMNN_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries(vision_detector
        ${catkin_LIBRARIES}
        ${OpenCV_LIBS}
        armnn
        yolo_armnn
        )

add_dependencies(vision_detector
        ${catkin_EXPORTED_TARGETS}
        )

add_executable(test_armnn_yolo2tiny
        test/test_armnn_yolo2tiny.cpp
        )

target_include_directories(test_armnn_yolo2tiny PRIVATE
        ${catkin_INCLUDE_DIRS}
        ${ARMNN_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries(test_armnn_yolo2tiny
        ${OpenCV_LIBS}
        armnn
        yolo_armnn
        )

target_compile_definitions(test_armnn_yolo2tiny PUBLIC
        NO_ROS=1
        )

add_dependencies(test_armnn_yolo2tiny
        ${catkin_EXPORTED_TARGETS}
        yolo_armnn
        )

add_test(NAME test_armnn_yolo2tiny
        COMMAND test_armnn_yolo2tiny
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

set_tests_properties(test_armnn_yolo2tiny PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${ARMNN_LIB_DIR}:$LD_LIBRARY_PATH")

add_executable(test_preprocess_image
        test/test_preprocess_image.cpp
        )

target_include_directories(test_preprocess_image PRIVATE
        ${catkin_INCLUDE_DIRS}
        ${ARMNN_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries(test_preprocess_image
        ${OpenCV_LIBS}
        armnn
        yolo_armnn
        )

target_compile_definitions(test_preprocess_image PUBLIC
        NO_ROS=1
        )

add_dependencies(test_preprocess_image
        ${catkin_EXPORTED_TARGETS}
        yolo_armnn
        )

add_test(NAME test_preprocess_image
        COMMAND test_preprocess_image
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

set_tests_properties(test_preprocess_image PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${ARMNN_LIB_DIR}:$LD_LIBRARY_PATH")

install(DIRECTORY launch/
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
        )

install(DIRECTORY models/
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/models
        )

install(TARGETS vision_detector vision_detector_tool yolo_armnn test_armnn_yolo2tiny test_preprocess_image
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )