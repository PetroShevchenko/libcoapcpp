#!/bin/bash

TARGET=POSIX
#TARGET=RASPBERRY-PI
#TARGET=STM32MP157A-DK1
#TARGET=NUCLEO-F429ZI
#TARGET=NUCLEO-H743ZI

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
        if [[ $1 = "clean" ]] ; then
            rm -rf build/POSIX
            exit 0
        fi
        mkdir -p build/POSIX
        cd build/POSIX && cmake ../.. && make -j$(nproc || echo 2)
        #mkdir -p examples && cd examples && cmake ../../../examples/POSIX && make -j$(nproc || echo 2) install

    elif [ "$BUILD_TYPE" = "DOCKER" ];then
        if [[ $1 = "clean" ]] ; then
            docker container prune -f        # remove all stopped containers
            docker image rm libcoapcpp-image # remove docker image named libcoapcpp-image
            exit 0
        fi
        cd script/docker && docker build -t libcoapcpp-image --rm -f ${DOCKER_FILE} ../..
        docker run --name=coap-container --rm -i -t libcoapcpp-image bash

    else
        echo "Error : BUILD_TYPE isn't specified"
    fi

elif [ "$TARGET" = "RASPBERRY-PI" ];then
        if [[ $1 = "clean" ]] ; then
            rm -rf "build/RASPBERRY-PI"
            exit 0
        fi
        mkdir -p "build/RASPBERRY-PI"
        cd "build/RASPBERRY-PI" && cmake ../.. && make -j$(nproc || echo 2)
        mkdir -p examples && cd examples && cmake "../../../examples/RASPBERRY-PI" && make -j$(nproc || echo 2)

elif [ "$TARGET" = "STM32MP157A-DK1" ];then
        if [[ $1 = "clean" ]] ; then
            rm -rf build/STM32MP157A-DK1
            exit 0
        fi
        . "/usr/local/oecore-x86_64/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi"
        mkdir -p build/STM32MP157A-DK1
        cd build/STM32MP157A-DK1 && cmake ../.. && make -j$(nproc || echo 2)
        mkdir -p examples && cd examples && cmake ../../../examples/STM32MP157A-DK1 && make -j$(nproc || echo 2) install

elif [ "$TARGET" = "NUCLEO-F429ZI" ] || [ "$TARGET" = "NUCLEO-H743ZI" ];then
    if [[ $1 = "clean" ]] ; then
        cd examples/$TARGET && ./build.sh clean
        exit 0
    fi
    cd examples/$TARGET && ./build.sh all

else
    echo "Error: TARGET isn't specified"
fi
