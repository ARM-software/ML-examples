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

set(MBED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mbed-os CACHE INTERNAL "")
set(MBED_CONFIG_PATH ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "")
set(APP_TARGET tools_project)

include(${MBED_PATH}/tools/cmake/app.cmake)

project("mbed-tflm-image-recognition")

add_executable("${PROJECT_NAME}" "")

add_subdirectory("tensorflow")
add_subdirectory(${MBED_PATH})
add_subdirectory("BSP")
add_subdirectory("image_recognition")

mbed_configure_app_target(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        image_recognition
        ${CMAKE_CURRENT_SOURCE_DIR}/tensorflow/lite/micro/tools/make/downloads/flatbuffers/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
  mbed-baremetal
)

if("${MAINFILE}" STREQUAL "test")
  set(mainfile test.cpp)
else()
  set(mainfile main.cpp)
endif()

target_sources(${PROJECT_NAME}
    PUBLIC
        image_recognition/${mainfile}
)

mbed_set_post_build(${PROJECT_NAME})

option(VERBOSE_BUILD "Have a verbose build process")
if(VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()
