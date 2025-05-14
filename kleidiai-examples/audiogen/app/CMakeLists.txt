#
# SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.16)
project(audiogen_runner)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(NOT ROOT_PATH)
  set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()

set(srcs audiogen.cpp)

# TFLite
if(NOT TF_LIB_PATH)
  message( FATAL_ERROR "TenserFlow library path is required, Flag TF_LIB_PATH must be set" )
endif()

if(NOT TF_INCLUDE_PATH)
  message( FATAL_ERROR "TenserFlow include path is required, Flag TF_INCLUDE_PATH must be set" )
endif()

# Flatbuffer
if(NOT FLATBUFFER_INCLUDE_PATH)
  message( FATAL_ERROR "Flatbuffer include path is required, Flag FLATBUFFER_INCLUDE_PATH must be set" )
endif()

include(FetchContent)

FetchContent_Declare(
  sentencepiece_src
  GIT_REPOSITORY https://github.com/google/sentencepiece.git
  GIT_TAG        v0.2.0
)

FetchContent_Populate(sentencepiece_src)
add_subdirectory(${sentencepiece_src_SOURCE_DIR} ${sentencepiece_src_BINARY_DIR})

list(APPEND common_link_directories
  ${TF_LIB_PATH}/)

list(APPEND common_include_directories
  ${ROOT_PATH}/runner
  ${TF_INCLUDE_PATH}
  ${FLATBUFFER_INCLUDE_PATH} )

list(APPEND audiogen_runner_deps tensorflowlite)

add_executable(audiogen ${srcs})

target_include_directories(audiogen PUBLIC
  ${common_include_directories}
  ${sentencepiece_SOURCE_DIR}/src)

target_link_directories(audiogen PUBLIC ${common_link_directories})

add_dependencies(audiogen sentencepiece)

# It is required to avoid the linking with libsentencepiece.so
list(APPEND audiogen_runner_deps ${sentencepiece_src_BINARY_DIR}/src/libsentencepiece.a)

if(ANDROID)
  list(APPEND audiogen_runner_deps log)
endif()

target_link_libraries(audiogen
  ${audiogen_runner_deps}
)

target_compile_options(audiogen PUBLIC)
