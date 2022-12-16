# STM32F746 device template files

Files under this directory have been derived from Keil CMSIS-pack STM32F7xx_DFP
version 2.15.1 (at the time of writing). In the original package too, they were
shared under Apache 2.0 license, as they are within this repository. See
[License.txt](LICENSE.txt).

These files are meant to be used as templates that users can override. See other
examples under Arm CMSIS GitHub repository here:

* [ARMCM7/Source directory](https://github.com/ARM-software/CMSIS_5/tree/5.9.0/Device/ARM/ARMCM7/Source)
* [startup_ARMCM7.s](https://github.com/ARM-software/CMSIS_5/blob/5.9.0/Device/ARM/ARMCM7/Source/ARM/startup_ARMCM7.s)
* [system_ARMCM7.c](https://github.com/ARM-software/CMSIS_5/blob/5.9.0/Device/ARM/ARMCM7/Source/system_ARMCM7.c)

The only change we have made to the startup assembly file is to the stack and heap sizes:

```asm
Stack_Size      EQU     0x00004000
Heap_Size       EQU     0x00014000
```
