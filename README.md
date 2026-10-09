# libcoapcpp

[![CI](https://github.com/PetroShevchenko/libcoapcpp/actions/workflows/ci.yml/badge.svg?branch=private%2Fdevelopment)](https://github.com/PetroShevchenko/libcoapcpp/actions/workflows/ci.yml)
[![Fuzz](https://github.com/PetroShevchenko/libcoapcpp/actions/workflows/fuzz.yml/badge.svg?branch=private%2Fdevelopment)](https://github.com/PetroShevchenko/libcoapcpp/actions/workflows/fuzz.yml)
[![License](https://img.shields.io/github/license/PetroShevchenko/libcoapcpp)](LICENSE)

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

### Disk space
| Component                     | Size                                           |
|-------------------------------|------------------------------------------------|
| Library sources               | < 1 MB                                         |
| Dependencies (git submodules) | up to 100 MB                                   |
| Build directory               | up to 500 MB, depending on optimization level  |

### Build tools
- CMake >= 3.5
- GNU Make >= 4.1
- g++ >= 7.4
- arm-none-eabi-g++ >= 10.3

Alternatively, the library and examples can be built inside a Docker container.

## Getting the library

Use the following git command to clone the library together with its submodules:

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
* RASPBERRY-PI-4

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

## PlatformIO

PlatformIO support is maintained in a separate `platformio` branch.

### Building the example

If you have already cloned the repository, switch to that branch:

```sh
git checkout platformio
```

Otherwise, clone the branch directly:

```sh
git clone -b platformio https://github.com/PetroShevchenko/libcoapcpp.git
cd libcoapcpp
```

Then build the `coap-server` example and upload it to the board:

```sh
cd examples/coap-server
pio run
pio run -t upload
```

> **Note:** the `coap-server` example is a work in progress. It currently only blinks an LED
> and serves as a project template that verifies the library builds for NUCLEO-F429ZI.
> The CoAP server itself is not implemented yet.

Alternatively, open `examples/examples.code-workspace` in VS Code with the PlatformIO IDE extension installed.

### Using the library in your own project

No cloning is needed. Add the library to your `platformio.ini`:

```ini
[env:your_board]
platform = ststm32
framework = stm32cube
lib_deps =
    https://github.com/PetroShevchenko/libcoapcpp.git#platformio
```

The dependencies (cJSON, FreeRTOS and lwIP) are downloaded automatically by PlatformIO.

## Use of AI tools

The library source code (`api/`, `src/`, `examples/`) is written by hand.

AI assistants are used as supporting tools for:

- code review and finding bugs;
- unit tests and fuzzing;
- build and CI scripts;
- documentation.

Each proposal from AI assistants is verified by the maintainer before being implemented in the project.

## License

This library is distributed under Apache license version 2.0.
LICENSE file contains license terms.
All third party libraries are distributed under their own licenses.