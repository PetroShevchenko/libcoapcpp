# libcoapcpp

libcoapcpp is an open-source C++ implementattion of the constrained application protocol (CoAP).
This is a modified version of libcoapcpp for use with <a href=https://platformio.org/>PlatformIO</a>.

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
The examples provided in this version of libcoapcpp depend on the following third-party libraries:
* cJSON
* stm32-platformio-freertos
* stm32-platformio-lwip

## Requirements
The following software packages must be installed on your PC or laptop:
* PlatformIO IDE or PlatformIO CLI
* Python3
* Python3 virtual environment 

All required information to install the packages you can find on the official <a href=https://platformio.org/>PlatformIO</a> site.

## License
This library is distributed under Apache license version 2.0.
LICENSE file contains license terms.
All third party libraries are distributed under their own licenses.

## Examples
All the provided examples are intended to be run on NUCLEO-F429ZI development board from STMicroelectronics.
The examples can be compiled using PlatformIO IDE or PlatformIO CLI.
For more information, please read the README of the example you are interested in.
