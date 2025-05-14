<!--
    SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

    SPDX-License-Identifier: Apache-2.0
-->

# Building and Running the Audio Generation Application on Arm® CPUs with the Stable Open Audio Small Model

## Dependencies
- An host laptop/PC with a Linux®-based operating system (tested on Ubuntu® 20.04.4 LTS with x86_64) or with macOS®.
- The Android™ NDK r25, which can be downloaded from [here](https://developer.android.com/ndk/downloads).
- CMake version: 3.16.0 or above, which can be downloaded from [here](https://cmake.org/download/)
- The three models (T5, DiT, and AutoEncoder) that make up the Stable Open Audio model converted into LiteRT-compatible formats. Ensure you have followed the instructions provided in the `scripts/` folder’s README.md file.

## Goal

This guide will show you how to build the <strong>LiteRT</strong> runtime along with the <strong>audio generation (audiogen)</strong> app contained in the single <strong>audiogen.cpp</strong> file.

## Building the Audio Generation App

In the `app/` folder, we recommend you set up a new virtual environment with Python 3.12 or later, for example, with <strong>virtualenv</strong>:

```bash
virtualenv -p python3.12 env3_12

# Activate virtual environment
source env3_12/bin/activate
```

Next, set the `LITERT_MODELS_PATH` environment variable to the path where your Stable Audeio Open Small models exported to LiteRT are located:

```bash
export LITERT_MODELS_PATH=<path_to_your_litert_models>
```

Then, follow one the following sections, depending on your <strong>HOST</strong> and <strong>TARGET</strong> platforms, to build the audiogen application.


### Build the audiogen app on Linux® (HOST) or macOS® (HOST) for Android™ (TARGET)

#### Step 1
Download and extract the Android™ NDK r25b.

```bash
# On Linux®
wget https://dl.google.com/android/repository/android-ndk-r25b-linux.zip
unzip android-ndk-r25b-linux

# On macOS®
wget https://dl.google.com/android/repository/android-ndk-r25b-darwin.zip
unzip android-ndk-r25b-darwin
mv android-ndk-r25b-darwin ~/Library/Android/android-ndk-r25b
```

#### Step 2
Set the `NDK_PATH` environment variable to the path where you extracted the Android™ NDK r25b:

```bash
export NDK_PATH=<path_to_android_ndk_r25>
```

Make sure the path points to the root directory of the extracted NDK package (e.g., /home/user/Android/Sdk/ndk/25.2.9519653)

#### Step 3
Clone the TensorFlow project:

```bash
git clone https://github.com/tensorflow/tensorflow.git tensorflow_src
```

#### Step 4
Enter the `tensorflow_src` directory, and checkout the `84dd28bbc29d75e6a6d917eb2998e4e8ea90ec56` commit:

```bash
cd tensorflow_src
git checkout 84dd28bbc29d75e6a6d917eb2998e4e8ea90ec56
```

In the `tensorflow_src` directory, set the `TF_SRC_PATH` environment variable to its current path:

```bash
export TF_SRC_PATH=$(pwd)
```

#### Step 5
Configure the Bazel build:

```bash
./configure
```

After running the `configure` script, you'll be prompted with a series of questions to set up the Bazel build. When asked for the Android™ NDK location, ensure it matches the path specified in the `NDK_PATH` environment variable.

To ensure reproducibility, we provide the following:

```bash
Please specify the location of python. [Default is ../app/.venv/bin/python3]:
Please input the desired Python library path to use. Default is [/../app/.venv/lib/python3.10/site-packages]
Do you wish to build TensorFlow with ROCm support? [y/N]: n
Do you wish to build TensorFlow with CUDA support? [y/N]: n
Do you want to use Clang to build TensorFlow? [Y/n]: n
Please specify optimization flags to use during compilation when bazel option "--config=opt" is specified [Default is -Wno-sign-compare]:
Would you like to interactively configure ./WORKSPACE for Android builds? [y/N]: y
Please specify the home path of the Android NDK to use. [Default is /Users/../library/Android/Sdk/ndk-bundle]: ../Library/Android/android-ndk-r25b
Please specify the (min) Android NDK API level to use. [Available levels: [16, 17, 18, 19, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31, 32, 33]] [Default is 21]: 30
Please specify the home path of the Android SDK to use. [Default is ../Library/Android/sdk]:
Please specify the Android SDK API level to use. [Available levels: ['35']] [Default is 35]:
Please specify an Android build tools version to use. [Available versions: ['35.0.0', '35.0.1', '36.0.0']] [Default is 36.0.0]:
Do you wish to build TensorFlow with iOS support? [y/N]: n

Configuration finished
```
#### Step 6
Build the LiteRT dynamic library with the following command:

```bash
bazel build -c opt --config android_arm64 //tensorflow/lite:libtensorflowlite.so \
    --define tflite_with_xnnpack=true \
    --define=xnn_enable_arm_i8mm=true \
    --define tflite_with_xnnpack_qs8=true \
    --define tflite_with_xnnpack_qu8=true
```

#### Step 7
Build flatbuffer:

```bash
mkdir flatc-native-build && cd flatc-native-build
cmake ../tensorflow/lite/tools/cmake/native_tools/flatbuffers
cmake --build .
```

#### Step 8
Build the audiogen application. To do so, from the `flatc-native-build`, go back to the `app` directory:

```bash
cd ../..
```

Inside the `app` directory, create the `build` folder and navigate into it:

```bash
mkdir build && cd build
```

Next, run CMake using the following command:

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=$NDK_PATH/build/cmake/android.toolchain.cmake \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -DANDROID_ABI=arm64-v8a \
      -DTF_INCLUDE_PATH=$TF_SRC_PATH \
      -DTF_LIB_PATH=$TF_SRC_PATH/bazel-bin/tensorflow/lite \
      -DFLATBUFFER_INCLUDE_PATH=$TF_SRC_PATH/flatc-native-build/flatbuffers/include \ \
    ..
```

Then, build the application:
```bash
make -j
```

At this point, you are ready to push the binaries to your Android™ device and run the audiogen application. To do so, use the `adb` tool to push all necessary files into `/data/local/tmp/app`

```bash
adb shell mkdir -p /data/local/tmp/app
adb push audiogen /data/local/tmp/app
adb push $LITERT_MODELS_PATH/dit_model.tflite /data/local/tmp/app
adb push $LITERT_MODELS_PATH/autoencoder_model.tflite /data/local/tmp/app
adb push $LITERT_MODELS_PATH/conditioners_float32.tflite /data/local/tmp/app
adb push ${TF_SRC_PATH}/bazel-bin/tensorflow/lite/libtensorflowlite.so /data/local/tmp/app
```

Since the tokenizer used in the audiogen application is based on <strong>SentencePiece</strong>, you’ll need to download the `spiece.model` file from:
https://huggingface.co/google-t5/t5-base/tree/main
and transfer it to your device.

```bash
wget https://huggingface.co/google-t5/t5-base/resolve/main/spiece.model
adb push spiece.model /data/local/tmp/app
```

At this point, you are ready to test the audiogen application.

Use the `adb` tool to enter the device:

```bash
adb shell
```

Then, go to `/data/local/tmp/app`
```bash
cd /data/local/tmp/app
```

From there, you can then run the `audiogen` application, which requires just three input arguments:

- **Model Path**: The directory containing your LiteRT models and `spiece.model` files
- **Prompt**: A text description of the desired audio (e.g., *warm arpeggios on house beats 120BPM with drums effect*)
- **CPU Threads**: The number of CPU threads to use (e.g., `4`)

```bash
LD_LIBRARY_PATH=. ./audiogen . "warm arpeggios on house beats 120BPM with drums effect" 4
```

If everything runs successfully, the generated audio will be saved in `.wav` format (`output.wav`) in the same directory as the `audiogen` binary. At this point, you can then retrieve it using the `adb` tool from a different Terminal and play it on your laptop or PC.

```bash
adb pull data/local/tmp/output.wav
```
