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

<h1><b>Running llama.cpp with KleidiAI </b></h1>

## Prerequisities

- Experience with Arm® cross-compilation on Android™ or Linux®
- Proficiency with Android™ shell commands
- An Android™ or Linux® device with an Arm® CPU with at least <strong>FEAT_DotProd</strong> (dotprod) and optionally <strong>FEAT_I8MM</strong> (i8mm) feature or the <strong>SME2</strong> technology.
- Minimum RAM requirement: 4GB.
- Minimum storage requirement: 4GB


## Dependencies
- A laptop/PC with a Linux®-based operating system (tested on Ubuntu® 20.04.4 LTS)
- The Android™ NDK (minimum version: r27), which can be downloaded from [here](https://developer.android.com/ndk/downloads).
- The Android™ SDK Platform command-line tools, which can be downloaded from [here](https://developer.android.com/tools/releases/platform-tools)
- CMake version: 3.27.0 or above, which can be downloaded from [here](https://cmake.org/download/)

## Goal

In this guide, we will show you how to apply a patch on top of llama.cpp to enable the <strong>[KleidiAI](https://gitlab.arm.com/kleidi/kleidiai)</strong> int4 matmul micro-kernels with per-block quantization (c32), and build llama.cpp for different Arm® targets.

> ℹ️ In the context of llama.cpp, this int4 format is called <strong>Q4_0</strong>.

These KleidiAI micro-kernels were fundamental to the Cookie and Ada chatbot, which Arm® showcased to demonstrate large language models (LLMs) running on existing flagship and premium mobile CPUs based on Arm® technology. You can learn more about the demo in <strong>[this](https://community.arm.com/arm-community-blogs/b/ai-and-ml-blog/posts/generative-ai-on-mobile-on-arm-cpu)</strong> blog post.

<p align="center">
<video autoplay src="https://community.arm.com/cfs-file/__key/telligent-evolution-videotranscoding-securefilestorage/communityserver-blogs-components-weblogfiles-00-00-00-38-23/phi_2D00_3-demo.mp4.mp4" width="640" height="480" controls></video>
</p>

> ⚠️ Please be aware that this guide is only intended as a demonstration of how to integrate the KleidiAI int4 matmul optimized routines in llama.cpp. It is provided without any commitment to support or keep it up to date with current versions of llama.cpp.

## Target Arm® CPUs

Arm® CPUs with <strong>FEAT_DotProd</strong> (dotprod), <strong>FEAT_I8MM</strong> (i8mm) features, or <strong>SME2</strong> technology.
<br>
<br>

# Applying the patch on top of llama.cpp to enable KleidiAI

### Step 1:

Clone the [llama.cpp](https://github.com/ggerganov/llama.cpp) repository:

```bash
git clone https://github.com/ggerganov/llama.cpp.git
```
### Step 2:

Enter the `llama.cpp/` directory, and checkout the `b8deef0ec0af5febac1d2cfd9119ff330ed0b762` commit:

```bash
cd llama.cpp
git checkout b8deef0ec0af5febac1d2cfd9119ff330ed0b762
```

The reason for checking out the `b8deef0ec0af5febac1d2cfd9119ff330ed0b762` commit is that it provides a stable base for applying the patch with the KleidiAI backend for llama.cpp.

### Step 3:

In the `llama.cpp/` directory, copy [this](0001-Updates-to-kleidiai-examples-llama_cpp.patch) patch, which includes the code changes for llama.cpp to enable the KleidiAI optimizations.


### Step 4:

Apply the patch with the KleidiAI backend:

```bash
git apply 0001-Updates-to-kleidiai-examples-llama_cpp.patch
```
<br>
<br>

# Building llama.cpp with KleidiAI

## Building for Android™ - Cross-compiling

### Step 1:

Build the llama.cpp project for Android™. To do so, set the Native Development Kit (NDK) path in an environment variable (for example, `NDK_PATH`):

```bash
export NDK_PATH="your-android-ndk-path"
```

> ℹ️ You can download the Android™ NDK package from <strong>[here](https://developer.android.com/ndk/downloads)</strong>. We recommend Android™ NDK version r27 or above.

Then, create a folder called `build`. Inside this folder, run the cmake command, and build the project:

```bash
mkdir build && cd build

cmake -DCMAKE_TOOLCHAIN_FILE=${NDK_PATH}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-23 ..

make -j4
```

> ℹ️ You can optionally enable the weights caching with -DGGML_KLEIDIAI_CACHE=ON. Weights caching is a feature available in the KleidiAI backend to improve the model loading time. Since the layout of the original model weights is transformed by KleidiAI to improve the performance of the matrix-multiplication routines, this option ensures that the weights transformation only happens the first time you run the model.

> ⚠️ If you enable weights caching, make sure to have enough storage memory as this feature stores another copy of the model, named `kai_transformed_weights.cache`, in the same location of your executable binaries.

## Building for Linux® - Cross-compiling

### Step 1:
Build the llama.cpp project for Linux®. To do so, set the Arm® GNU Toolchain path in an environment variable (for example, `GNU_TOOLCHAIN_PATH`):

```bash
export GNU_TOOLCHAIN_PATH="your-gnu-toolchain-path"
```

> ℹ️ You can download the Arm® GNU Toolchain from <strong>[here](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)</strong>. We recommend Arm® GNU Toolchain 13.3.rel1 or above.

Then, create a folder called `build`. Inside this folder, run the cmake command, and build the project:

```bash
mkdir build && cd build

cmake -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=arm -DCMAKE_CXX_COMPILER=$GNU_TOOLCHAIN_PATH/bin/aarch64-none-linux-gnu-g++ -DCMAKE_C_COMPILER=$GNU_TOOLCHAIN_PATH/bin/aarch64-none-linux-gnu-gcc ..

make -j4
```

> ℹ️ You can optionally enable the weights caching with -DGGML_KLEIDIAI_CACHE=ON. Weights caching is a feature available in the KleidiAI backend to improve the model loading time. Since the layout of the original model weights is transformed by KleidiAI to improve the performance of the matrix-multiplication routines, this option ensures that the weights transformation only happens the first time you run the model.

> ⚠️ If you enable weights caching, make sure to have enough storage memory as this feature stores another copy of the model, named `kai_transformed_weights.cache`, in the same location of your executable binaries.

## Building for Linux® - Native

Build the llama.cpp project natively for Linux®. To do so, create a folder called `build`. Inside this folder, run the cmake command, and build the project:

```bash
mkdir build && cd build

cmake ..

make -j4
```

> ℹ️ You can optionally enable the weights caching with -DGGML_KLEIDIAI_CACHE=ON. Weights caching is a feature available in the KleidiAI backend to improve the model loading time. Since the layout of the original model weights is transformed by KleidiAI to improve the performance of the matrix-multiplication routines, this option ensures that the weights transformation only happens the first time you run the model.

> ⚠️ If you enable weights caching, make sure to have enough storage memory as this feature stores another copy of the model, named `kai_transformed_weights.cache`, in the same location of your executable binaries.

## Building for macOS® - Native

```bash
mkdir build && cd build

cmake -DGGML_METAL=OFF -DGGML_BLAS=OFF ..

make -j4
```

## Building for Windows® on Arm®

- Install [Visual Studio 2022](https://visualstudio.microsoft.com/de/vs/community/)
- Install Required Components in Visual Studio Installer
  - Workload Tab: Desktop development with C++
  - Individual Components Tab (search for these components): C++ CMake Tools for Windows®, Git for Windows®, C++ Clang Compiler for Windows®, MSBuild Support for LLVM-Toolset (clang)
- Environment Setup:
  - If the host machine is x86-based, please use the integrated Developer Command Prompt / PowerShell in VS2022 for building and testing.
  - If the host machine is Arm64-based, please use the system's cmd and set environment variables by running `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64`as the integrated Developer Command Prompt / PowerShell in VS2022 is meant for x86
```cmd
cmake --preset arm64-windows-llvm-release -D KLEIDIAI_BUILD_TESTS=OFF -D GGML_OPENMP=OFF
cmake --build build-arm64-windows-llvm-release
```

The options GGML_KLEIDIAI_CACHE and KLEIDIAI_BUILD_TESTS are disabled on Windows®, as they are currently not supported. And please use llvm preset as MSVC is not supported either.

<br>
<br>

# Running and profiling llama.cpp with KleidiAI

### Step 1: (Optional)

Remove the relative path to `libllama.so` and `libggml.so` in the built binaries with `patchelf`:

```bash
patchelf --replace-needed ../../src/libllama.so libllama.so bin/llama-cli
patchelf --replace-needed ../../ggml/src/libggml.so libggml.so bin/llama-cli
patchelf --replace-needed ../ggml/src/libggml.so libggml.so src/libllama.so
patchelf --replace-needed ../../src/libllama.so libllama.so bin/llama-bench
patchelf --replace-needed ../../ggml/src/libggml.so libggml.so bin/llama-bench
```

### Step 2:

Download the Large Language Model (LLM) in `.gguf` format with `Q4_0` weights. For example, you can download the <strong>Phi-2</strong> model from [here](https://huggingface.co/TheBloke/phi-2-GGUF/blob/main/phi-2.Q4_0.gguf).

### Step 3:

Copy the `llama-cli` and `llama-bench` binaries with their required dynamic libraries to your target device. For example, if your target device is an Android™-based platform, you can push the binaries to `/data/local/tmp` using the following `adb` command:

```bash
adb push src/libllama.so /data/local/tmp/
adb push bin/llama-cli /data/local/tmp/
adb push bin/llama-bench /data/local/tmp/
adb push ggml/src/libggml.so /data/local/tmp/
```

If you are targeting a Linux®-based system, you could use `scp`.

### Step 4:

Copy the LLM model to your target device. For example, if your target device is an Android™-based platform, you can push the model to `/data/local/tmp` using the following `adb` command:

```bash
adb push phi-2.Q4_0.gguf /data/local/tmp
```

### Step 5:

Enter your target device. If your target device is an Android™-based platform, you can enter the device using the following `adb` command:

```bash
adb shell
```

If you are targeting a Linux®-based system, you could login using `ssh`.

### Step 6:

Enter the folder where you copied the llama.cpp binaries. For example, if your target device is an Android™-based platform, your directory might be `/data/local/tmp`:

```bash
cd /data/local/tmp
```

### Step 7:

Run the model inference using the `llama-cli` binary using 4 CPU cores:

```bash
export LD_LIBRARY_PATH=.

./llama-cli -m phi-2.Q4_0.gguf -p "Write a code in C for bubble sorting" -n 32 -t 4
```

### Step 8:

To profile the model inference we recommend using the `llama-bench` binary.

For example, to profile the performance on 4 CPU cores, you can use the following command:

```bash
export LD_LIBRARY_PATH=.

./llama-bench -t 4 -m phi-2.Q4_0.gguf -n 32 -p 64
```

The KleidiAI backend will automatically detect the available features at runtime and dispatch the suitable optimizations for the target device.


The performance results will be reported for the encoder (test = `pp64`) and decoder (test = `tg32`) phases in `tokens / second` (`t/s`). The higher the `t/s`, the better.

That’s all for this guide!
