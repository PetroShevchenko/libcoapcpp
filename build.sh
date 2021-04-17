#!/bin/bash

TARGET=POSIX
#TARGET=STM32H747I-DISCO

BUILD_TYPE=NATIVE
#BUILD_TYPE=DOCKER

#DOCKER_FILE=dockerfile.ubuntu
#DOCKER_FILE=dockerfile.debian
DOCKER_FILE=dockerfile.fedora

mkdir -p build

if [ "$TARGET" = "POSIX" ];then

    if [ "$BUILD_TYPE" = "NATIVE" ];then
        cd build && cmake .. && make -j$(nproc || echo 2)

    elif [ "$BUILD_TYPE" = "DOCKER" ];then
        cd script/docker && docker build -t libcoapcpp-image --rm -f ${DOCKER_FILE} ../..
        docker run --name=coap-container --rm -i -t libcoapcpp-image bash

    else
        echo "Error : BUILD_TYPE isn't specified"
    fi

elif [ "$TARGET" = "STM32H747I-DISCO" ];then
    echo "Warning: The STM32H747I-DISCO target hasn't been implemented yet."
    echo "Please, set TARGET=POSIX"

else
    echo "Error: TARGET isn't specified"
fi
