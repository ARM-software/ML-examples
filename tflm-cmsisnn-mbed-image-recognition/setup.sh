#!/bin/bash

#
# Copyright (c) 2019-2021 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Clean the project before setting up again.
./clean.sh

#---------------------Configuration variables------------------------
TFLM_SHA="22eb58f5c4a8856f6aa5d96c6afa45e6c4ba0e45"
STM_CUBE_SHA="c7c5ec99c7482ea8bcdbf0a869c930af4547088f"
CMSIS=true

#---------------------------TFLM------------------------------
git clone https://github.com/tensorflow/tflite-micro
cd tflite-micro
git checkout ${TFLM_SHA}
git am ../patches/0001-profiler.patch

# Third party files need to be downloaded to build tflite-micro.
# For instance, Flatbuffers is not natively included in tflite-micro.
make -f tensorflow/lite/micro/tools/make/Makefile third_party_downloads

# Returns all necessary header and source files
OPTIONS="OPTIMIZED_KERNEL_DIR=cmsis_nn"
if [ "$CMSIS" = false ] ; then
  OPTIONS=""
fi
HEADERS=$(make -f tensorflow/lite/micro/tools/make/Makefile $OPTIONS list_library_headers)
SOURCES=$(make -f tensorflow/lite/micro/tools/make/Makefile $OPTIONS list_library_sources)
TP_HEADERS=$(make -f tensorflow/lite/micro/tools/make/Makefile $OPTIONS list_third_party_headers)
TP_SOURCES=$(make -f tensorflow/lite/micro/tools/make/Makefile $OPTIONS list_third_party_sources)
HEADERS_SEP=$(echo $HEADERS | tr " " "\n")
SOURCES_SEP=$(echo $SOURCES | tr " " "\n")
TP_HEADERS_SEP=$(echo $TP_HEADERS | tr " " "\n")
TP_SOURCES_SEP=$(echo $TP_SOURCES | tr " " "\n")
cd ..

# Copy files into project folder
for file in $HEADERS_SEP
do
  DIR=$(dirname $file)
  BASENAME=$(basename $file)
  mkdir -p $DIR
  cp tflite-micro/$file $DIR
done

for file in $SOURCES_SEP
do
  DIR=$(dirname $file)
  BASENAME=$(basename $file)
  mkdir -p $DIR
  cp tflite-micro/$file $DIR
done

for file in $TP_HEADERS_SEP
do
  DIR=$(dirname $file)
  BASENAME=$(basename $file)
  mkdir -p $DIR
  cp tflite-micro/$file $DIR
done

for file in $TP_SOURCES_SEP
do
  DIR=$(dirname $file)
  BASENAME=$(basename $file)
  mkdir -p $DIR
  cp tflite-micro/$file $DIR
done

# Remove the tflite-micro clone as it is no longer needed
rm -rf tflite-micro/
#---------------------------TEST IMAGES---------------------------
wget mirror.tensorflow.org/www.cs.toronto.edu/~kriz/cifar-10-binary.tar.gz
tar -xzf cifar-10-binary.tar.gz
rm cifar-10-binary.tar.gz
cp cifar-10-batches-bin/test_batch.bin .
rm -rf cifar-10-batches-bin/

#---------------------------DRIVERS---------------------------
git clone https://github.com/STMicroelectronics/STM32CubeF7
cd STM32CubeF7
git checkout ${STM_CUBE_SHA}
git am ../patches/0001-Drivers-patch.patch
cd ..


# Copy drivers from STM32CubeF7 to separate folder
mkdir -p BSP
mkdir -p BSP/Drivers
mkdir -p BSP/Drivers/BSP
mkdir -p BSP/Utilities
mkdir -p BSP/Drivers/BSP/Components
cp -r STM32CubeF7/Drivers/BSP/STM32746G-Discovery BSP/Drivers/BSP
cp -r STM32CubeF7/Drivers/BSP/Components/Common BSP/Drivers/BSP/Components
cp -r STM32CubeF7/Drivers/BSP/Components/ft5336 BSP/Drivers/BSP/Components
cp -r STM32CubeF7/Drivers/BSP/Components/n25q128a BSP/Drivers/BSP/Components
cp -r STM32CubeF7/Drivers/BSP/Components/ov9655 BSP/Drivers/BSP/Components
cp -r STM32CubeF7/Drivers/BSP/Components/rk043fn48h BSP/Drivers/BSP/Components
cp -r STM32CubeF7/Drivers/BSP/Components/wm8994 BSP/Drivers/BSP/Components
cp -r STM32CubeF7/Utilities/Fonts BSP/Utilities
rm -rf STM32CubeF7/


#---------------------------CMAKE-----------------------------
./generate_cmake_files.sh tensorflow
./generate_cmake_files.sh BSP

#---------------------------MBED------------------------------
mbed-tools deploy


