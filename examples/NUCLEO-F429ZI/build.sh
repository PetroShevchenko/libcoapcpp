#!/bin/bash

TARGET=NUCLEO-F429ZI
#TARGET=NUCLEO-H743ZI

if [ "$TARGET" = "NUCLEO-F429ZI" ];then
BOARD_NAME="STM32F4xx_Nucleo_144"
elif [ "$TARGET" = "NUCLEO-H743ZI" ];then
BOARD_NAME="NUCLEO-H743ZI"
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
LIBCOAPCPP_ROOT=../..
BOARD_PATH=${LIBCOAPCPP_ROOT}/build/${BOARD_NAME}

print_done()
{
    echo "$PROMPT ${DONE_COLOUR}Done${RESET}"
}

usage()
{
    echo "$PROMPT ${WARNING_COLOUR}Use \"./build.sh all\" to compile libraries and examples${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}Use \"./build.sh lib\" to compile libraries${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}Use \"./build.sh app\" to compile examples${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}Use \"./build.sh clean\" to remove automatically generated files${RESET}"
}

prepare_build_directory()
{
    mkdir -p ${LIBCOAPCPP_ROOT}/build
    mkdir -p ${LIBCOAPCPP_ROOT}/build/${BOARD_NAME}
    for i in $(ls -d */);
    do 
        if [[ ${i%%/} != "common" ]] && [[ ${i%%/} != "script" ]];then
            mkdir -p ${BOARD_PATH}/${i%%/}
            mkdir -p ${BOARD_PATH}/${i%%/}/obj 
        fi
    done
    mkdir -p ${BOARD_PATH}/common-libraries
    mkdir -p ${BOARD_PATH}/common-libraries/obj
    mkdir -p ${BOARD_PATH}/common-libraries/lib
}

compile_libraries()
{
    cd script
    echo "$PROMPT ${INFO_COLOUR}Compiling of HAL library...${RESET}"
    make -f "hal.make" all -j$(nproc || echo 2)
    print_done

    echo "$PROMPT ${INFO_COLOUR}Compiling of BSP library...${RESET}"
    make -f "bsp.make" all -j$(nproc || echo 2)
    print_done

    echo "$PROMPT ${INFO_COLOUR}Compiling of FreeRTOS library...${RESET}"
    make -f "freertos.make" all -j$(nproc || echo 2)
    print_done

    echo "$PROMPT ${INFO_COLOUR}Compiling of LwIP library...${RESET}"
    make -f "lwip.make" all -j$(nproc || echo 2)
    print_done

    echo "$PROMPT ${INFO_COLOUR}Compiling of CoapCpp library...${RESET}"
    make -f "coapcpp.make" all -j$(nproc || echo 2)
    print_done

    cd ..
}

compile_examples()
{
    for i in $(ls -d */);
    do 
        if [[ ${i%%/} != "common" ]] && [[ ${i%%/} != "script" ]];then
            echo "$PROMPT ${INFO_COLOUR}Compiling of ${i%%/} application...${RESET}"
            cd ${i%%/}
            make -f Makefile all -j$(nproc || echo 2)
            cd ..
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

if [ "$1" = "lib" ];then
    prepare_build_directory
    compile_libraries
elif [ "$1" = "app" ];then
    prepare_build_directory
    compile_examples
elif [ "$1" = "all" ];then
    prepare_build_directory
    compile_libraries
    compile_examples
elif [ "$1" = "clean" ];then
    clean
else
    usage
fi
