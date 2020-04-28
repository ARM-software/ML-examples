#! /usr/bin/env bash
#
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

export ARMNN_LIB_INSTALL_DIR=/opt/armnn/lib

# source the ros execution environment
. install/setup.sh

# print some usage guide
echo "Open the following url in any browser to see the detection visualisation"
echo
echo "http://<ip address of the hikey>:8080/stream?topic=/vision_detector/image_rects"
echo

sleep 2

# launch vision_detector node, this will also start roscore and a visualisation
# helper. This is launched in background and will be killed when ctrl-c is
# is received from user.
LD_LIBRARY_PATH="${ARMNN_LIB_INSTALL_DIR}:${LD_LIBRARY_PATH}" \
roslaunch vision_detector vision_yolo2tiny_detect.launch &
VISION_DETECTOR_PID=$!

# wait for roscore to start
sleep 2

# start web server so the detection result can be viewed in a browser. This is
# launched in background and will be killed when ctrl-c is is received from
# user.
rosrun web_video_server web_video_server & WEB_SERVER_PID=$!

# play rosbag in a loop. This is launched in background and will be killed when
# ctrl-c is is received from user.
rosbag play --loop --skip-empty 0.1 demo.rosbag &
ROSBAG_PLAY_PID=$!

# wait for ctrl-c and kill all processes on exit
interrupt_handler()
{
    kill $ROSBAG_PLAY_PID
    kill $WEB_SERVER_PID
    kill $VISION_DETECTOR_PID
    exit 1
}
trap 'interrupt_handler' INT

# Block and wait for ctrl-c signal from user which will shut down all launched
# processes and exit cleanly from script.
while true; do
    sleep 1
done
