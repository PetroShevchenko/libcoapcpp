#!/bin/bash
# This bash script is intended to compile all provided for Raspberry Pi Pico examples

TARGET=RASPBERRY-PI
BOARD_NAME=Pico

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
LIBCOAPCPP_ROOT=../..

BOARD_PATH=${LIBCOAPCPP_ROOT}/build/${TARGET}/${BOARD_NAME}

print_done()
{
    echo "$PROMPT ${DONE_COLOUR}Done${RESET}"
}

usage()
{
    echo "$PROMPT ${WARNING_COLOUR}Use \"./build.sh all\" to compile all Pico examples${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}Use \"./build.sh clean\" to remove automatically generated files${RESET}"
}

prepare_build_directory()
{
    mkdir -p ${LIBCOAPCPP_ROOT}/build
    mkdir -p ${LIBCOAPCPP_ROOT}/build/${TARGET}
    mkdir -p ${LIBCOAPCPP_ROOT}/build/${TARGET}/${BOARD_NAME}
    for i in $(ls -d */);
    do 
        if [[ ${i%%/} != "common" ]] && [[ ${i%%/} != "script" ]] && [[ ${i%%/} != "doc" ]];then
            mkdir -p ${BOARD_PATH}/${i%%/}
            mkdir -p ${BOARD_PATH}/${i%%/}/obj 
        fi
    done
}

compile_examples()
{
    for i in $(ls -d */);
    do 
        if [[ ${i%%/} != "common" ]] && [[ ${i%%/} != "script" ]] && [[ ${i%%/} != "doc" ]];then
            echo "$PROMPT ${INFO_COLOUR}Compiling of ${i%%/} application...${RESET}"
            CMAKE_FILE_PATH=`pwd`/${i%%/}/RPi-Pico
            cd ${BOARD_PATH}/${i%%/} && cmake ${CMAKE_FILE_PATH} && make -j$(nproc || echo 2)
            cd ${CMAKE_FILE_PATH}/../..
            print_done
        fi
    done
}

clean()
{
    echo "$PROMPT ${INFO_COLOUR}Removing of automatically generated files...${RESET}"
    echo "rm -rf ${BOARD_PATH}"
    rm -rf ${BOARD_PATH}
    print_done
}

if [ "$1" = "all" ];then
    prepare_build_directory
    compile_examples
elif [ "$1" = "clean" ];then
    clean
else
    usage
fi
