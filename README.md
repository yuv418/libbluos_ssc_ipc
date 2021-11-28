# libbluos_ssc_ipc

`libbluos_ssc.so` is an MQA decoder library extracted from the firmware of a certain MQA-capable DAC. It is an ARM library, for ARMv7 devices.
Integrating this with an x86_64 system is a pain, as to use the library, you have to create an ARMv7 executable using `dlopen`, but that can only run within
a QEMU binary translaation environment.


This repository will use shared memory IPC to communicate between a wrapper executable that uses `dlopen` and calls `libbluos_ssc.so` functions within a binary translation environment and a regular amd64 (or whatever else you want that supports QEMU binary translation) environment. Doing this will allow us to be able to easily send FLAC binary "chunks" to the ARM environment for processing, one at a time, which will allow for real-time MQA audio playback from any userspace amd64 program by simply invoking this library.


