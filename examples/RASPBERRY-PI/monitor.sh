#!/bin/bash

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

check_minicom()
{
    if [[ `minicom --version` ]];then
        return 1
    else 
        return 0
    fi
}

echo "$PROMPT ${INFO_COLOUR}Checking if minicom is already installed...${RESET}"
check_minicom
if [[ $? -eq 1 ]];then
    minicom "-D /dev/serial0 -b 115200"
else
    echo "$PROMPT ${ERROR_COLOUR}Please install minicom first${RESET}"
    exit 0
fi