#! /usr/bin/env bash
#
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

set -u
set -e

NETWORK_NAME="yolov2-tiny"
NETWORK_CFG="${NETWORK_NAME}.cfg"
NETWORK_WEIGHTS="${NETWORK_NAME}.weights"
NETWORK_LABELS="labels.txt"

# download network and label in darknet format
curl https://raw.githubusercontent.com/pjreddie/darknet/master/cfg/${NETWORK_NAME}.cfg > ${NETWORK_CFG}
curl https://pjreddie.com/media/files/${NETWORK_NAME}.weights > ${NETWORK_WEIGHTS}
curl https://raw.githubusercontent.com/amikelive/coco-labels/master/coco-labels-2014_2017.txt > ${NETWORK_LABELS}

# workaround for https://github.com/thtrieu/darkflow/issues/802
tail -c +5 ${NETWORK_WEIGHTS} > ${NETWORK_WEIGHTS}.truncated
mv ${NETWORK_WEIGHTS}.truncated ${NETWORK_WEIGHTS}

# start a virtual python environment
VENV_NAME="darkflow-venv"
virtualenv -p python3 ${VENV_NAME}
source ${VENV_NAME}/bin/activate
# install dependencies for darkflow
pip install --upgrade tensorflow==1.15 numpy opencv-python cython

# install darkflow
rm -rf darkflow
git clone https://github.com/thtrieu/darkflow.git
cd darkflow
pip install .
cd ..

# call darkflow to convert model from darknet format to tensorflow format
flow --model ${NETWORK_CFG} --load ${NETWORK_WEIGHTS} --savepb --labels ${NETWORK_LABELS}

# shutdown virtual python environment
deactivate

# copy the output pb file to the current directory with a name expected by the
# vision detector node
cp built_graph/${NETWORK_NAME}.pb yolo_v2_tiny.pb
