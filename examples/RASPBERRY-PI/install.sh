#!/bin/bash
# This bash script installs FreeRTOS-Kernel repository that are required for Raspberry Pi Pico examples

cd ~/
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel
cd FreeRTOS-Kernel
git checkout smp
FREERTOS_KERNEL_PATH=`pwd`
echo 'export FREERTOS_KERNEL_PATH='${FREERTOS_KERNEL_PATH} >> ~/.bashrc
. ~/.bashrc 

