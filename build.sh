#!/bin/bash

TARGET=POSIX
#TARGET=ESP32
#TARGET=DISCO_H747I
#TARGET=DISCO_F746NG

BUILD_TYPE=NATIVE
#BUILD_TYPE=DOCKER

#DOCKER_FILE=dockerfile.ubuntu
#DOCKER_FILE=dockerfile.debian
DOCKER_FILE=dockerfile.fedora

if [ -z "$GTEST_DIR" ] ; then
   export GTEST_DIR=$(pwd)/third-party/googletest/googletest
fi
if [ -z "$GMOCK_DIR" ] ; then
   export GMOCK_DIR=$(pwd)/third-party/googletest/googlemock
fi

if [ -z "$GTEST_DIR" ] ; then
   echo "GTEST_DIR is not defined (Google Test location)! Abort..."
   exit -1
fi

mkdir -p third-party
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

elif [ "$TARGET" = "DISCO_H747I" ];then
    echo "To build libcoapcpp for DISCO_H747I please use build.sh from examples/STM32"

elif [ "$TARGET" = "DISCO_F746NG" ];then
    echo "To build libcoapcpp for DISCO_F746NG please use build.sh from examples/STM32"

elif [ "$TARGET" = "ESP32" ];then
    echo "To build libcoapcpp for ESP32 please use build.sh from examples/ESP32"

else
    echo "Error: TARGET isn't specified"
fi
