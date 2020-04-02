#! /usr/bin/env python2
#
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

from __future__ import print_function
from ros import rosbag
from sensor_msgs.msg import Image
from glob import glob
from cv_bridge import CvBridge
from os import path
import numpy as np
import rospy.rostime
import cv2
import time

def main(image_dir, rosbag_name):
    bridge = CvBridge()

    fn_l = glob(path.join(image_dir, "*.png"))
    fn_l = sorted(fn_l)

    bag =rosbag.Bag(rosbag_name, 'w')
    timestamp = rospy.rostime.Time.from_sec(time.time())
    # the data is captured at 20 Hz
    delta_t = rospy.rostime.Duration.from_sec(1.0/20.0)

    for i, fn in enumerate(fn_l):
        print(i + 1, "/", len(fn_l))
        img = cv2.imread(fn)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        img_msg = bridge.cv2_to_imgmsg(img, "rgb8")
        timestamp += delta_t
        bag.write('/image_raw', img_msg, timestamp)

    bag.close()

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description='Convert images into rosbag')
    parser.add_argument('image_dir',
        help='folder containing png images to be turned into rosbag')
    parser.add_argument('rosbag_name',
        help='full path to output rosbag')

    args = parser.parse_args()

    main(args.image_dir, args.rosbag_name)
