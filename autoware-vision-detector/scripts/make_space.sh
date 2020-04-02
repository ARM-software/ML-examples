#! /usr/bin/env bash
#
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

set -u
set -e

sudo cp -r /var/cache /home/arm01/armnn-devenv/var-cache
sudo rm -rf /var/cache
sudo ln -s /home/arm01/armnn-devenv/var-cache /var/cache

sudo cp -r /usr/lib/aarch64-linux-gnu /home/arm01/armnn-devenv/aarch64-linux-gnu
sudo rm -rf /usr/lib/aarch64-linux-gnu
sudo ln -s /home/arm01/armnn-devenv/aarch64-linux-gnu /usr/lib/aarch64-linux-gnu
