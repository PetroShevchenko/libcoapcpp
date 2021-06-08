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
	mkdir -p examples && cd examples && cmake ../../examples/POSIX && make -j$(nproc || echo 2)

    elif [ "$BUILD_TYPE" = "DOCKER" ];then
        cd script/docker && docker build -t libcoapcpp-image --rm -f ${DOCKER_FILE} ../..
        docker run --name=coap-container --rm -i -t libcoapcpp-image bash

    else
        echo "Error : BUILD_TYPE isn't specified"
    fi

elif [ "$TARGET" = "DISCO_H747I" ];then
    cd examples/STM32 && ./build.sh all

elif [ "$TARGET" = "DISCO_F746NG" ];then
    cd examples/STM32 && ./build.sh all

elif [ "$TARGET" = "ESP32" ];then
    cd examples/ESP32 && ./build.sh

else
    echo "Error: TARGET isn't specified"
fi
