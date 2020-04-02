#! /usr/bin/env bash
#
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

set -u
set -e

ROS_DISTRO=kinetic

# install tools needed
sudo apt-get install -y python-catkin-pkg python-rosdep ros-$ROS_DISTRO-catkin
sudo apt-get install -y python3-pip python3-colcon-common-extensions
sudo apt-get install -y python3-setuptools python3-vcstool
sudo apt-get install -y gksu wget git ros-kinetic-web-video-server rosbash
pip3 install -U setuptools

# create folder for the source code
mkdir -p autoware/src
pushd autoware

# download manifest for source code
wget -O autoware.ai.repos "https://gitlab.com/autowarefoundation/autoware.ai/autoware/raw/1.12.0/autoware.ai.repos?inline=false"

# checkout all repos specified in the manifest
vcs import src < autoware.ai.repos

# remove modules that are not related to this demo
rm -rf src/car_demo src/citysim

# install ros dependencies for autoware
rosdep update
rosdep install -y --from-paths src --ignore-src --rosdistro $ROS_DISTRO

# setup armnn dependencies
ARMNN_INSTALL_DIR=/opt/armnn
ARMNN_LIB_INSTALL_DIR=$ARMNN_INSTALL_DIR/lib
ARMNN_INCLUDE_INSTALL_DIR=$ARMNN_INSTALL_DIR/include
ARMNN_SRC_DIR=${HOME}/armnn-devenv/armnn
ARMNN_BUILD_DIR=${ARMNN_SRC_DIR}/build
BOOST_LIB_DIR=${ARMNN_SRC_DIR}/../pkg/install/lib/

# copy armnn related dependencies into /opt/armnn
sudo mkdir -p ${ARMNN_LIB_INSTALL_DIR}
sudo mkdir -p ${ARMNN_INCLUDE_INSTALL_DIR}
sudo cp -r ${ARMNN_SRC_DIR}/include/* ${ARMNN_INCLUDE_INSTALL_DIR}
sudo cp -r ${ARMNN_BUILD_DIR}/*.so* ${ARMNN_LIB_INSTALL_DIR}
sudo cp -r ${BOOST_LIB_DIR}/* ${ARMNN_LIB_INSTALL_DIR}
