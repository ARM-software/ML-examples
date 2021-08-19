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

#----------------------------About this file-------------------------------
#
# This file is called automatically in setup.sh and does not need to be
# run by the user.
#
# This file will create CMakeLists.txt files in each subdirectory
# (except leaf directories) of the specified directory.
# It will add .cc, .c, and .cpp files as sources:
#
# `target_sources(${PROJECT_NAME}
#   PUBLIC
#       <directory>/<file>
# )`
#
# .h files are included by adding the entire directory to target,
# which is formatted like this:
#
# `target_include_directories(${PROJECT_NAME}
#   PUBLIC
#       <directory>
# )`
#
# Additionally, if a subdirectory contains any files which are to be
# included, that subdirectory must be included as a subdirectory.
# For example `<dir>/<subdir>/<file.cc>` would require the following in
# `<dir>/CMakeLists.txt`(added automatically):
#
# `add_subdirectory("<subdir>")`
#--------------------------------------------------------------------------

root_dir=${PWD}
cd $1

touch CMakeLists
rm CMakeLists
minimum="cmake_minimum_required(VERSION 3.21.0)"

for file_path in $(find . -not -path '*/\.*' -type d); do
  cd $file_path

  current_dir=${PWD##*/}

  #----------------------------------------SOURCES----------------------------------------
  # Add sources
  sources="target_sources("'${PROJECT_NAME}'"\n\tPUBLIC"
  for cc in *.{c,cc,cpp}; do
    if ! [ "$cc" == "*.c" ] && ! [ "$cc" == "*.cc" ] && ! [ "$cc" == "*.cpp" ]; then
      sources+="\n\t\t $current_dir/$cc"
    fi
  done

  # Check if any sources were added
  if ! [ "$sources" == "target_sources("'${PROJECT_NAME}'"\n\tPUBLIC" ]; then
    cd ..
    if ! [[ ${PWD} == ${root_dir} ]]; then
      printf "\n\n${sources}\n)" >> CMakeLists.txt
    fi
    cd ${current_dir}
  fi

  #----------------------------------------HEADERS----------------------------------------
  # Add headers
  target="target_include_directories("'${PROJECT_NAME}'"\n\tPUBLIC"
  hfile=$(find . -maxdepth 0 -name "*.h")
  if [ ${#hfile[@]} -gt 0 ]; then
    if ! [ "$hfile" == "*.h" ]; then
      target+="\n\t\t $current_dir"
    fi
  fi
  if ! [ "$target" == "target_include_directories("'${PROJECT_NAME}'"\n\tPUBLIC" ]; then
    cd ..
    if ! [[ ${PWD} == ${root_dir} ]]; then
      printf "\n\n${target}\n)" >> CMakeLists.txt
    fi
    cd ${current_dir}
  fi

  #-------------------------------SUBDIRECTORIES & GENERAL--------------------------------
  # Only create CMakeLists.txt if NOT leaf node in directory tree
  if [[ $(find ./* -maxdepth 0 -not -path '*/\.*' -type d) ]]; then
    touch CMakeLists.txt
    printf "$minimum\n\n" >> CMakeLists.txt

    # Add all subdirectories and exclude hidden ones
    for dir in $(find ./* -maxdepth 0 -not -path '*/\.*' -type d); do
      if [[ $(find ${dir}/* -maxdepth 0 -not -path '*/\.*' -type d) ]]; then
        printf "add_subdirectory($dir)\n" >> CMakeLists.txt
      fi
    done
  fi
  cd
  cd ${root_dir}/$1
done

