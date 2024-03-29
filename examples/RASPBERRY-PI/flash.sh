#!/bin/bash

TARGET=RASPBERRY-PI
BOARD_NAME=Pico

RED=`tput setaf 1`
GREEN=`tput setaf 2`
YELLOW=`tput setaf 3`
BLUE=`tput setaf 4`
GRAY=`tput setaf 8`
RESET=`tput sgr0`

WARNING_COLOUR=$YELLOW
ERROR_COLOUR=$RED
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

print_failed()
{
    echo "$PROMPT ${ERROR_COLOUR}Failed${RESET}"    
}

usage()
{
    echo "$PROMPT ${WARNING_COLOUR}Use \"./flash.sh <Project Name>\" to upgrade the firmware${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}<Project Name> -- name of the project, that should be uploaded to the device${RESET}"
    echo "$PROMPT ${WARNING_COLOUR}Use \"ls\" command to see project names (except \"common\" and \"script\")${RESET}"    
    echo "$PROMPT ${WARNING_COLOUR}Example : ./flash.sh udp-server${RESET}"
}

upgrade_firmware()
{
    FIRMWARE=${BOARD_PATH}/"$1"/"$1.elf"
    `openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c init -c "reset init" -c "halt" -c "flash probe 0" -c "flash write_image erase ${FIRMWARE} 0x00000000 elf" -c shutdown`
}

check_updater()
{
    if `openocd --version`;then
        return 1
    else 
        return 0
    fi
}

if [ $# -eq 0 ];then
    usage 
else
    for i in $(ls -d */);
    do 
        if [[ ${i%%/} != "common" ]] && [[ ${i%%/} != "script" ]];then
            if [ ${i%%/} == "$1" ];then
                echo "$PROMPT ${INFO_COLOUR}Checking if OpenOCD is already installed...${RESET}"
                check_updater
                if [[ $? -eq 1 ]];then
                    print_done
                else
                    echo "$PROMPT ${ERROR_COLOUR}Please install OpenOCD first${RESET}"
                    exit 0
                fi
                echo "$PROMPT ${INFO_COLOUR}Upgrading of "$1" application...${RESET}"
                upgrade_firmware "$1"
                print_done
                exit 0
            fi
        fi
    done
    usage
fi
