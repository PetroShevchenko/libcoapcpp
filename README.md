# libcoapcpp

libcoapcpp is an open-source C++ implementattion of the constrained application protocol (CoAP).

## Library content

libcoapcpp supports the features described in the following RFCs:
* RFC7252 The Constrained Application Protocol (CoAP) 							--> Status: Implemented
* RFC7959 Block-Wise Transfers in the Constrained Application Protocol (CoAP) 	--> Status: Implemented
* RFC8428 Sensor Measurement Lists (SenML) 										--> Status: Implemented
* RFC6690 Constrained RESTful Environments (CoRE) Link Format 					--> Status: Implemented
* RFC7641 Observing Resources in the Constrained Application Protocol (CoAP) 	--> Status: Planned to implement 

## Introduction
libcoapcpp is a library that can be linked from your own source code.
This implementation also includes several examples of using the library.
libcoapcpp uses the following third-party libraries:
* googletest
* spdlog
* mbedtls
* wolfssl
* cJSON

Individual examples for different hardwares require additional third-party libraries to be installed:
* STM32CubeF4
* STM32CubeH7
* STM32CubeMP1
* STM32MP1 OpenSTLinux Developer Package
* Raspberry Pi Pico C/C++ SDK

## Requirements
Your computer must have at least 10.0 Gbytes of free disk space to be able to download all third party libraries. 

To build the library, use the following utilities:
* cmake >= 3.5
* GNU make >= 4.1
* g++ >= 7.4
* arm-none-eabi-g++ >= 10.3

Docker container technology can be used to build the library and examples.

## License
This library is distributed under Apache license version 2.0.
LICENSE file contains license terms.
All third party libraries are distributed under their own licenses.

## Getting the library
Use the following git command to clone the library:

`$ git clone --recurse-submodules https://github.com/PetroShevchenko/libcoapcpp.git` 

## Building
To build the library and the examples, use build.sh script:

`$ ./build.sh`

If you want to use build in a Docker container, first install Docker
following the instructions https://docs.docker.com/get-docker/. 

You can configure your build with script variables:
* TARGET - select the target platform to be used

Currently, the following platforms are supported:
* POSIX
* RASPBERRY-PI
* NUCLEO-F429ZI
* STM32MP157A-DK1

The following options are available only if TARGET=POSIX:
* BUILD_TYPE - select build in host system (NATIVE) or docker container(DOCKER)
* DOCKER_FILE - if you set BUILD_TYPE=DOCKER, there are three options availabe:

	- dockerfile.fedora - Fedora Linux Docker image
	- dockerfile.debian - Debian Linux Docker image
	- dockerfile.ubuntu - Ubuntu Linux Docker image

## Testing
libcoapcpp provides unit tests, placed in the test directory, all of them will be compiled when build.sh is run.
The unit tests use the Google Test framework. 
Before using the unit tests, make sure you have the Google Test library installed (third-party/googletest)
To run the unit tests, use test_run.sh script:

`$ ./test_run.sh`   

## Examples
All provided examples will be compiled together with the library after running build.sh.
There are the binaries of the examples in libcoapcpp/build directory.
For more information, please read the README of the example you are interested in.