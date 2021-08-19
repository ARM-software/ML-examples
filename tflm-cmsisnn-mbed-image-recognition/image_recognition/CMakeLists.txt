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

cmake_minimum_required(VERSION 3.21.0)

 target_sources(${PROJECT_NAME}
   PUBLIC
     stm32f746_discovery/display_util.cc
     stm32f746_discovery/image_provider.cc
     stm32f746_discovery/image_util.cc
)

 target_include_directories(${PROJECT_NAME}
   PUBLIC
     stm32f746_discovery
)

 target_sources(${PROJECT_NAME}
   PUBLIC
     image_recognition_model/image_recognition_model.cc
)

 target_include_directories(${PROJECT_NAME}
   PUBLIC
     image_recognition_model
)
