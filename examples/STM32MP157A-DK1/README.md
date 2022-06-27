# STM32MP157A-DK1 examples

This directory contains examples to demonstrate using libcoapcpp library on the STM32MP157A-DK1 board.


## Introduction
The STM32MP157A-DK1 board contains two parts:
* two Cortex-A8 cores
* one Cortex-M4 core

The Cortex-A8 part is intended to use an Embedded Linux.
The Cortex-M4 part can be used in the ordenary way as a microcontroller with lots of periphery.
Interconnection between these two parts can be realised using a virtual COM port.

The examples contain source files for both parts.

## Toolchain
There are two toolchains, one is for the Cortex-A8 part and the other is for the Cortex-M4 one.


### STM32MP1 OpenSTLinux Developer Package
<p>To compile examples for the Cortex-A8 part the STM32MP1 OpenSTLinux Developer Package is used.</p>
<ol>
<li>Download the developer package by visiting the following resource https://www.st.com/en/embedded-software/stm32mp1dev.html.</li>
<li>Unpack the downloaded archive file en.SDK-x86_64-stm32mp1-openstlinux-5.10-dunfell-mp1-21-11-17_tar.xz.</li>
<li>
Launch the Bash script to install the SDK:

~~~
$ chmod u+x st-image-weston-openstlinux-weston-stm32mp1-x86_64-toolchain-3.1.11-openstlinux-5.10-dunfell-mp1-21-11-17.sh
$ ./st-image-weston-openstlinux-weston-stm32mp1-x86_64-toolchain-3.1.11-openstlinux-5.10-dunfell-mp1-21-11-17.sh
~~~

<li>After installation the default path to the SDK should be the following:</li>

~~~
/usr/local/oecore-x86_64
~~~
</li>
</ol>

## Compilation
To built the libcoapcpp library and the examples for the STM32MP157A-DK1 board you should select TARGER variable in main build.sh first:

~~~
TARGET=STM32MP157A-DK1
~~~ 

After that just launch build.sh:

~~~
$ ./build.sh
~~~

All compiled binaries will be available in the build/STM32MP157A-DK1 directory

## Installation
<p>
To install built library and examples, copy all binaries to the target board.
You can use ssh utility to get access to the Linux command line interface.
Make sure you plugged Ethernet cable to the board, the other side of the cable should be
connected to your network equipment.
</p>
<ol>
<li>
To see all available devices in the local network use the nmap utility:

~~~
$ nmap -sP <Local Network Addres>
~~~

For example:
~~~
$ nmap -sP 192.168.0.0/24
Starting Nmap 7.92 ( https://nmap.org ) at 2022-06-27 20:16 EEST
Nmap scan report for _gateway (192.168.0.1)
Host is up (0.0011s latency).
Nmap scan report for 192.168.0.100
Host is up (0.10s latency).
Nmap scan report for 192.168.0.101
Host is up (0.091s latency).
Nmap scan report for 192.168.0.105
Host is up (0.098s latency).
Nmap scan report for 192.168.0.107
Host is up (0.0020s latency).
Nmap scan report for DellVostro (192.168.0.109)
Host is up (0.00021s latency).
Nmap done: 256 IP addresses (6 hosts up) scanned in 4.21 seconds
~~~
</li>
<li>
Connect to the board via ssh:

~~~
$ ssh root@<Board IP>
~~~

For example:
~~~
$ ssh root@192.168.0.107
The authenticity of host '192.168.0.107 (192.168.0.107)' can't be established.
RSA key fingerprint is SHA256:O8juvQd3+RQ3zNNjtUzlDi+26KQ8KQ4gWjUvWOiPBjw.
Are you sure you want to continue connecting (yes/no/[fingerprint])? yes
Warning: Permanently added '192.168.0.107' (RSA) to the list of known hosts.
root@stm32mp1:~# 
~~~
</li>
<li>
To copy required binaries, use the scp utility:

~~~
$ scp <Source> <Destination>
~~~

For example:
~~~
$ scp -r build/STM32MP157A-DK1/examples/bin root@192.168.0.107:/home/root
coap-server                                                                                                                                                                                                                                      100%   14MB   3.3MB/s   00:04
firmware.bin                                                                                                                                                                                                                                     100%  296KB   2.3MB/s   00:00
well-known_core.wlnk                                                                                                                                                                                                                             100%  485    10.1KB/s   00:00
~~~
</li>
<li>
Run one of the examples:

~~~
root@stm32mp1:~/bin# ./coap-server
[2022-06-27 17:26:26.703] [debug] ./coap-server has been started
[2022-06-27 17:26:26.703] [debug] port: 5683
[2022-06-27 17:26:26.703] [debug] use IPv4: false
[2022-06-27 17:26:26.704] [debug] core link content:
</sensors>;ct=40;title="Sensor Index",
</sensors/temp>;rt="temperature-c";if="sensor",
</sensors/hum>;rt="humidity-%";if="sensor",
</t>;anchor="/sensors/temp";rel="alternate",
</h>;anchor="/sensors/hum";rel="alternate",
<coap://192.168.0.104/sensors/DHT11/t>;anchor="/sensors/temp"
;rel="describedby",
<coap://192.168.0.104/sensors/DHT11/h>;anchor="/sensors/hum"
;rel="describedby",
</firmware>;ct=40;title="Firmware Index",
</firmware/NUCLEO-F429ZI/udp-server>;rt="firmware";sz=302816
[2022-06-27 17:26:26.704] [debug] creating a new connection...
[2022-06-27 17:26:26.704] [debug] OK
[2022-06-27 17:26:26.704] [debug] binding socket with IP and port...
[2022-06-27 17:26:26.704] [debug] OK
[2022-06-27 17:26:26.704] [debug] creating a new CoAP server...
[2022-06-27 17:26:26.705] [debug] OK
[2022-06-27 17:26:26.705] [debug] server is launching...
[2022-06-27 17:26:26.705] [debug] OK
[2022-06-27 17:26:27.706] [debug] select() timeout 1 sec expired
[2022-06-27 17:26:28.708] [debug] select() timeout 1 sec expired
^C[2022-06-27 17:26:29.224] [debug] SIGINT cought
[2022-06-27 17:26:29.224] [debug] select() error, errno = 4
[2022-06-27 17:26:29.224] [debug] ./coap-server has been finished
root@stm32mp1:~/bin#
~~~
</li>
</ol>
