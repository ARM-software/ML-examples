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
# Example usages:
# ./test_performance.sh 42 DISCO_F746NG ARM
# ./test_performance.sh 10 NUCLEO_F746ZG GCC_ARM
if ! [ $# -eq 3 ] ; then
    echo "Wrong number of arguments (expected 3, SEED, TARGET and TOOLCHAIN)"
    exit 1
fi
if ! [[ $1 =~ ^[0-9]+$ ]] ;  then
    echo "SEED should be a non-negative integer!"
    exit 1
fi
if ! [ "$3" = "GCC_ARM" ] && ! [ "$3" = "ARM" ] ; then
    echo $3
    echo "Invalid toolchain. Should be GCC_ARM or ARM"
    exit 1
fi
TARGET=$2
TOOLCHAIN=$3
IMAGE_SIZE=3073
RANDOM=$1
OFFSET=$(( RANDOM % 200 ))
BYTES_OFFSET=$(( OFFSET * 153650 ))
xxd -s $BYTES_OFFSET -l 153650 -i test_batch.bin image_recognition/50_cifar_images.h
sed -i -E "s/unsigned char/const unsigned char/g" image_recognition/50_cifar_images.h
mbedtools configure -m $TARGET -t $TOOLCHAIN -o cmake_build/$TARGET/release/$TOOLCHAIN/
cmake -S . -B cmake_build/$TARGET/release/$TOOLCHAIN -DCMAKE_BUILD_TYPE=RELEASE -DMAINFILE=test -GNinja
ninja -C cmake_build/$TARGET/release/$TOOLCHAIN


