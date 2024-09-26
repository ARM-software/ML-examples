<!--
    MIT License

    Copyright (c) 2024 Arm Limited

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
-->

<h1><b>Running llama.cpp with KleidiAI Int4 matrix-multiplication (matmul) micro-kernels </b></h1>

## Prerequisities

- Experience with Arm® cross-compilation on Android™
- Proficiency with Android® shell commands
- An Android® device with an Arm® CPU with <strong>FEAT_DotProd</strong> (dotprod) and <strong>FEAT_I8MM</strong> (i8mm) features

## Dependencies
- A laptop/PC with a Linux®-based operating system (tested on Ubuntu® 20.04.4 LTS)
- The Android™ NDK (minimum version: r25), which can be downloaded from [here](https://developer.android.com/ndk/downloads).
- The Android™ SDK Platform command-line tools, which can be downloaded from [here](https://developer.android.com/tools/releases/platform-tools)

## Goal

In this guide, we will show you how to apply a patch on top of llama.cpp to enable the <strong>[KleidiAI](https://gitlab.arm.com/kleidi/kleidiai)</strong> int4 matmul micro-kernels with per-block quantization (c32).

> ℹ️ In the context of llama.cpp, this int4 format is called <strong>Q4_0</strong>.

These KleidiAI micro-kernels were fundamental to the Cookie and Ada chatbot, which Arm® showcased to demonstrate large language models (LLMs) running on existing flagship and premium mobile CPUs based on Arm® technology. You can learn more about the demo in <strong>[this](https://community.arm.com/arm-community-blogs/b/ai-and-ml-blog/posts/generative-ai-on-mobile-on-arm-cpu)</strong> blog post.

<p align="center">
<video autoplay src="https://community.arm.com/cfs-file/__key/telligent-evolution-videotranscoding-securefilestorage/communityserver-blogs-components-weblogfiles-00-00-00-38-23/phi_2D00_3-demo.mp4.mp4" width="640" height="480" controls></video>
</p>

> ⚠️ Please be aware that this guide is only intended as a demonstration of how to integrate the KleidiAI int4 matmul optimized routines in llama.cpp. It is provided without any commitment to support or keep it up to date with current versions of llama.cpp.

## Target Arm® CPUs

Arm® CPUs with <strong>FEAT_DotProd</strong> (dotprod) and <strong>FEAT_I8MM</strong> (i8mm) features.

## Running llama.cpp with KleidiAI

Connect your Android™ device to your computer and open Terminal. Then, follow the following steps to apply the patch with the KleidiAI backend on top of llama.cpp.

### Step 1:

Clone the [llama.cpp](https://github.com/ggerganov/llama.cpp) repository:

```bash
git clone https://github.com/ggerganov/llama.cpp.git
```
### Step 2:

Enter the `llama.cpp/` directory, and checkout the `6fcd1331efbfbb89c8c96eba2321bb7b4d0c40e4` commit:

```bash
cd llama.cpp
git checkout 6fcd1331efbfbb89c8c96eba2321bb7b4d0c40e4
```

The reason for checking out the `6fcd1331efbfbb89c8c96eba2321bb7b4d0c40e4` commit is that it provides a stable base for applying the patch with the KleidiAI backend for llama.cpp.

### Step 3:

In the `llama.cpp/` directory, copy [this](0001-Use-KleidiAI-Int4-Matmul-micro-kernels-in-llama.cpp.patch) patch, which includes the code changes for llama.cpp to enable the KleidiAI optimizations.


### Step 4:

Apply the patch with the KleidiAI backend:

```bash
git apply 0001-Use-KleidiAI-Int4-Matmul-micro-kernels-in-llama.cpp.patch
```

### Step 5:

Build the llama.cpp project for Android™:

```bash
mkdir build && cd build

export NDK_PATH="your-android-ndk-path"

cmake -DLLAMA_KLEIDIAI=ON -DLLAMA_KLEIDIAI_CACHE=ON -DCMAKE_TOOLCHAIN_FILE=${NDK_PATH}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-23 -DCMAKE_C_FLAGS=-march=armv8.2a+i8mm+dotprod -DCMAKE_CXX_FLAGS=-march=armv8.2a+i8mm+dotprod ..

make -j4
```
Build the llama.cpp project for Linux®:

```bash
mkdir build && cd build

cmake -DLLAMA_KLEIDIAI=ON -DLLAMA_KLEIDIAI_CACHE=ON -DCMAKE_C_FLAGS=-march=armv8.2-a+dotprod+i8mm -DCMAKE_CXX_FLAGS=-march=armv8.2-a+dotprod+i8mm ..

make -j4
```
The  -DLLAMA_KLEIDIAI_CACHE=ON  is used to enable the weights caching. Weights caching is a feature available in the KleidiAI backend to improve the model loading time. Since the layout of the original model weights is transformed by KleidiAI to improve the performance of the matrix-multiplication routines, this option ensures that the weights transformation only happens the first time you run the model.
To disable this option, you simply remove the flag from the cmake command.

### Step 6:

Download the Large Language Model (LLM) in `.gguf` format with `Q4_0` weights. For example, you can download the <strong>Phi-2</strong> model from [here](https://huggingface.co/TheBloke/phi-2-GGUF/blob/main/phi-2.Q4_0.gguf).


### Step 7:

Push the `llama-cli` binary and the `.gguf` file to `/data/local/tmp` on your Android™ device:

```bash
adb push bin/llama-cli /data/local/tmp
adb push phi-2.Q4_0.gguf /data/local/tmp
```

### Step 8:

Enter your Android™ device:

```bash
adb shell
```
Then, go to `/data/local/tmp`:

```bash
cd /data/local/tmp
```

### Step 9:

Run the model inference using the `llama-cli` binary using 4 CPU cores:

```bash
./llama-cli -m phi-2.Q4_0.gguf -p "Write a code in C for bubble sorting" -n 32 -t 4
```

That’s all for this guide!
