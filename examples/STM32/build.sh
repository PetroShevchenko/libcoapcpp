#!/bin/bash

TARGET=DISCO_H747I
#TARGET=DISCO_F746NG
TOOLCHAIN=GCC_ARM

if [ "$TARGET" = "DISCO_H747I" ];then
LD_SCRIPT="mbed-os/targets/TARGET_STM/TARGET_STM32H7/TARGET_STM32H747xI/TARGET_STM32H747xI_CM7/TOOLCHAIN_GCC_ARM/STM32H747xI_CM7.ld"
elif [ "$TARGET" = "DISCO_F746NG" ];then
LD_SCRIPT="mbed-os/targets/TARGET_STM/TARGET_STM32F7/TARGET_STM32F746xG/TOOLCHAIN_GCC_ARM/STM32F746xG.ld"
else
    echo "Error: TARGET isn't specified"
fi

RED=`tput setaf 1`
GREEN=`tput setaf 2`
YELLOW=`tput setaf 3`
BLUE=`tput setaf 4`
GRAY=`tput setaf 8`
RESET=`tput sgr0`

WARNING_COLOUR=$YELLOW
INFO_COLOUR=$GREEN
DONE_COLOUR=$BLUE
PROMPT_COLOUR=$GRAY

PROMPT="$PROMPT_COLOUR[$0] :$RESET"

print_done()
{
    echo "$PROMPT ${DONE_COLOUR}done${RESET}"
}

usage()
{
    echo "$PROMPT ${WARNING_COLOUR}use \"./build.sh all\" to compile libraries and examples${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}use \"./build.sh lib\" to compile libraries${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}use \"./build.sh app\" to compile examples${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}use \"./build.sh clean\" to remove automatically generated files${RESET}"
}

mbed_compile_libraries()
{
    LIBCOAPCPP_ROOT=../..

    echo "$PROMPT ${INFO_COLOUR}importing of mbed-os source files...${RESET}"
    mbed import "https://github.com/ARMmbed/mbed-os" mbed-os
    print_done

    echo "$PROMPT ${INFO_COLOUR}compiling of mbed-os library...${RESET}"
    mbed compile --library -t ${TOOLCHAIN} -m ${TARGET} --source mbed-os --build build/${TARGET}/lib
    print_done

    echo "$PROMPT ${INFO_COLOUR}copying of libcoapcpp source files to coapcpp directory...${RESET}"
    echo "mkdir -p coapcpp"
    mkdir -p coapcpp
    echo "mkdir -p coapcpp/src"
    mkdir -p coapcpp/src
    echo "mkdir -p coapcpp/api"
    mkdir -p coapcpp/api
    echo "cp -r ${LIBCOAPCPP_ROOT}/src/*.cc coapcpp/src"
    cp -r ${LIBCOAPCPP_ROOT}/src/*.cc coapcpp/src
    echo "cp -r ${LIBCOAPCPP_ROOT}/src/*.h coapcpp/src"
    cp -r ${LIBCOAPCPP_ROOT}/src/*.h coapcpp/src
    echo "cp -r ${LIBCOAPCPP_ROOT}/api/*.h coapcpp/api"
    cp -r ${LIBCOAPCPP_ROOT}/api/*.h coapcpp/api
    print_done

    echo "$PROMPT ${INFO_COLOUR}compiling of coapcpp library...${RESET}"
    mbed compile --library -t ${TOOLCHAIN} -m ${TARGET} --source coapcpp --build build/${TARGET}/lib
    print_done
}

mbed_compile_examples()
{
    echo "$PROMPT ${INFO_COLOUR}compiling of coap-client application...${RESET}"
    mbed compile -t ${TOOLCHAIN} -m ${TARGET} --source coap-client --source build/${TARGET}/lib -l ${LD_SCRIPT} --build build/${TARGET}/bin
    print_done
}

clean()
{
    echo "$PROMPT ${INFO_COLOUR}removing of automatically generated files...${RESET}"
    echo "rm -rf build"
    rm -rf build
    echo "rm -rf coapcpp"
    rm -rf coapcpp
    echo "rm -rf mbed-os"
    rm -rf mbed-os
    echo "rm -f .mbed"
    rm -f .mbed
    print_done
}

if [ "$1" = "lib" ];then
    mbed_compile_libraries
elif [ "$1" = "app" ];then
    mbed_compile_examples
elif [ "$1" = "all" ];then
    mbed_compile_libraries
    mbed_compile_examples
elif [ "$1" = "clean" ];then
    clean
else
    usage
fi
