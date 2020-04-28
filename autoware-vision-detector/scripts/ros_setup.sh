#! /usr/bin/env bash
#
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

set -u
set -e

ROS_DISTRO=kinetic

# setup source lists for ros
sudo apt-get install lsb-release
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654

# install ros base packages
sudo apt-get update
sudo apt-get install -y ros-$ROS_DISTRO-ros-base

# initialise rosdep
sudo rosdep init
rosdep update

# add ros setup script to bash init script
echo "source /opt/ros/kinetic/setup.bash" >> ~/.bashrc
source ~/.bashrc
