# POSIX examples

This directory contains examples to demonstrate how to use the libcoapcpp library on the POSIX system.
All examples have been tested on Ubuntu 20.04 LTS

## Directory structure

This directory contains the following sub-directories:
* common 
* dtls-echo-server
* tcp-client
* tls-client1
* tls-client2
* udp-echo-client
* udp-echo-server

The common directory contains common for all examples source files.
Any other directory contains a separate example to demonstrate the usage of the libcoap library.

## Compilation

All provided in the POSIX directory examples are compiled together by the build.sh located in the project's root directory.
When the examples are compiled their binaries will become available in build/examples/bin directory.

Everething you need is to set the following variables in the build.sh file:
* TARGET=POSIX
* BUILD_TYPE:

	- NATIVE if you're going to compile the library and the examples in your native POSIX system (Linux/Unix/MacOS etc)
	- DOCKER if you're going to compile the library and the examples in the Docker container

There are three Docker systems available:
* Fedora
* Debian
* Ubuntu

To select your preferred system set BUILD_TYPE to one of the following values:
* dockerfile.fedora
* dockerfile.debian
* dockerfile.ubuntu

## Overview of the examples
How to use each example is described in the corresponding README file.

Here is a short description:
* **tcp-client:** the TCP client sends to the HTTP server GET request and outputs an answer to STDOUT.
* **tls-client1:** the first TLS client uses the WolfSSL library to securery connect to the HTTPS server, sends GET request to the server and outputs a response to STDOUT.
* **tls-client2:** the second TLS client does the same as the first one does but uses the MBED-TLS library.
* **udp-echo-client:** the UDP echo client sends a UDP message to the server, receive an answer and outputs it to STDOUT,
should be used together with udp-echo-server.
* **udp-echo-server:** the UDP echo server receive a meassage and sends it back.

