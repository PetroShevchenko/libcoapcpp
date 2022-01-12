# udp-server example

This example provides the firmware of an UDP server built into NUCLEO-F429ZI board,
that can control peripheral modules of the microcontroller. 
The example can be expanded to control and diagnostic external sensors, such as DHT11 and others.

## Introduction
This example uses Ethernet interface and LwIP TCP/IP stack to establish a connection
between builtin UDP server and UDP client launched on a computer.

## Connection to the UDP server

As a UDP client you can use ncat utility:
~~~
$ sudo apt install ncat -y
~~~
~~~
$ ncat -u <IP assigned to NUCLEO-F429ZI> <Port Number>
~~~
udp-server example uses port number 5555. 
IP address is assigned by DHCP.
You can find out IP addres from the log. 
Plug Ethernet cable into RJ-45 connector of NUCLEO-F429ZI then plug microUSB cable into builtin ST-Link connector.

Launch monitor.sh:
~~~
$ cd libcoapcpp/examples/NUCLEO-F429ZI
$ ./monitor.sh
~~~ 
Look at the log, extract the message with assigned IP address:
~~~
[StartIpAssignerTask:275] IP address assigned by DHCP: 192.168.000.103
~~~

Another way to communicate with the builtin UDP server is using a UDP client,
written in Python.
The UDP client is located in the NUCLEO-F429ZI/util directory under the name udp-client.py.  

Launch udp-client.py:
~~~
$ cd libcoapcpp/examples/NUCLEO-F429ZI/util
$ ./udp-client.py <IP asssigned to the board> 5555
~~~
* Note: Make sure bosh UDP server and client are located in the same network.

## Check command line interface (CLI)
CLI can be used to setup, control and diagnostic different parameters of the board
and peripheral modules of the microcontroller.

Launch ncat:
~~~
$ ncat -u <IP> 5555 
~~~

Use 'help' command to see all available commands:
~~~
help
clarify <command> -- clarify command format
echo -- send echo request to receive the same response
puts -- print string to peripheral module
get -- get parameter of peripheral module
set -- set parameter of peripheral module

~~~

Use 'clarify' command to find out the format of the commands from the list:
~~~
clarify echo
echo <text string>, where:
	text string -- no more than 240 characters
Response:
	the same text string
	Error: <message>

~~~
~~~
clarify puts
puts <module> <text string>, where:
	module -- peripheral module:
		spi -- SPI1 interface
		uart -- USART3 interface
		usb -- USB_OTG_FS interface
	text string -- no more than 240 characters
Response:
	OK
	Error: <message>
~~~
~~~
clarify get
get <module> <parameter>, where:
	module -- peripheral module:
		spi -- SPI1 interface
		uart -- USART3 interface
		usb -- USB_OTG_FS interface
		rtc -- RTC module
	parameter -- requested parameter:
		baudrate -- communication speed (bps)
		data_bits -- number of data bits
		stop_bits -- number of stop bits
		flow_ctrl -- flow control
		time -- current time
		date -- current date
Response:
	requested parameter value
	Error: <message>

~~~
~~~
clarify set
set <module> <parameter> <value>, where:
	module -- peripheral module:
		spi -- SPI1 interface
		uart -- USART3 interface
		usb -- USB_OTG_FS interface
		rtc -- RTC module
	parameter -- requested parameter:
		baudrate -- communication speed (bps)
		data_bits -- number of data bits
		stop_bits -- number of stop bits
		flow_ctrl -- flow control
		time -- current time in format: HH:MM:SS
		date -- current date in format: DD.MM.YYYY
value -- new value of parameter
Response:
	OK
	Error: <message>

~~~

## Setting the current date and time

Since the microcontroller of NUCLEO-F429ZI contains RTC (Real Time Clock) module,
we can set the date and time. 
But pay attantion that NUCLEO-F429ZI doen't have any battery supply source,
thus after the power reset RTC settings will be reset as well.

Use 'set' command to set date and time:
~~~
set rtc date 13.01.2022
OK
~~~
~~~
set rtc time 00:00:00
OK
~~~

## Getting the current date and time
To get the current date and time use 'get' command:
~~~
get rtc date
13.01.2022
~~~
~~~
get rtc time
00:02:04
~~~

## How to extend the functionality

All supported commands are described in C++ source file Core/Src/command.cc.

You can easyly add new commands to perform any required actions on NUCLEO-F429ZI
using modules of the microcontroller and the board components.

For example, add a command to control the LEDs on the board:

1. Modify help_response in Core/Src/command.cc by adding a new line:
~~~
const char * const help_response = "clarify <command> -- clarify command format\n"\
		"echo -- send echo request to receive the same response\n"\
		"puts -- print string to peripheral module\n"\
		"get -- get parameter of peripheral module\n"\
		"set -- set parameter of peripheral module\n"\
		"ctrl -- control of board component\n";
~~~
2. Add ctrl_usage next to help_response:
~~~
const char * const ctrl_usage = "ctrl <component> <command>, where:\n"\
		"\tcomponent -- board component:\n"\
		"\t\tled1 -- LED1(GREEN)\n"\
		"\t\tled2 -- LED2(BLUE)\n"\
		"\t\tled3 -- LED3(RED)\n"\
		"\tcommand -- requested action:\n"\
		"\t\ton -- turn on\n"\
		"\t\toff -- turn off\n"\
		"\t\ttoggle -- toggle state\n"\
		"Response:\n"\
		"\tOK\n"\
		"\tError: <message>\n";
~~~
3. Add a new record to the array Command::CommandInfo Command::m_context[CMD_QTY]:
~~~
Command::CommandInfo Command::m_context[CMD_QTY] = {
	...
	{CMD_CTRL, "ctrl", ctrl_usage, ctrl_cmd_handler},
};
~~~
4. Open Core/Inc/command.hh file.
Add ctrl_usage to the namespace:

~~~
namespace
{
	...
	extern const char * const ctrl_usage;
}
~~~
5. Add the new command code CMD_CTRL:
~~~
	typedef enum {
		...
		CMD_CTRL,
		CMD_QTY
	} CommandCode;
~~~
6. Add the declaration of the new command handler to the Command class:
~~~
class Command
{
	...
private:
	...
	static void ctrl_cmd_handler(cost std::string &request, std::string &answer, std::error_code &ec);
	...
};
~~~
7. The declared method should be implemented in Core/Src/command.cc:
~~~
void Command::ctrl_cmd_handler(const std::string &request, std::string &answer, std::error_code &ec)
{
	CMD_LOG_ENTER();
	size_t pos = request.find("ctrl");
	size_t offset = pos + sizeof("ctrl");
	size_t len  = request.length() - offset;
	if (len > answer.length())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
		CMD_LOG_EXIT();
		return;		
	}
	unsigned int ledNum;
	sscanf(request.substr(offset, len).c_str(), "led%u ", &ledNum);
	if (ledNum < 1 || ledNum > 3)
	{
		ec = make_system_error(EINVAL);
		CMD_LOG("LED=%u",ledNum);
		CMD_LOG_EXIT();
	}
	ledNum -= 1;// LED1..LED3 ==> index {0..2}
	char cmd[sizeof("toggle")];
	offset += strlen("led1 ");
	len = request.length() - offset;
	sscanf(request.substr(offset, len).c_str(), "%s", cmd);
	Led_TypeDef LED[3] = { LED1, LED2, LED3 };
	if (strncmp(cmd, "toggle", strlen("toggle")) == 0)
	{
		BSP_LED_Toggle(LED[ledNum]);
	}
	else if (strncmp(cmd, "on", strlen("on")) == 0)
	{
		BSP_LED_On(LED[ledNum]);
	}
	else if (strncmp(cmd, "off", strlen("off")) == 0)
	{
		BSP_LED_Off(LED[ledNum]);
	}
	else
	{
		ec = make_system_error(EINVAL);
		CMD_LOG("cmd=%s",cmd);
		CMD_LOG_EXIT();		
	}
	answer = "OK\n";
	CMD_LOG_EXIT();
}
~~~
8. Don't forget to include BSP header into command.cc file:
~~~
#include "stm32f4xx_nucleo_144.h"
~~~
9. Compile udp-server example together with others:
~~~
$ cd libcoapcpp/examples/NUCLEO-F429ZI
$ ./build.sh all
~~~
10. Upload the firmware:
~~~
$ ./flash.sh udp-server
~~~
11. Launch monitor then push Reset button (black one) on the board:
~~~
$ ./monitor.sh
~~~
12. Launch UDP client and control LEDs:
~~~
$ ncat -u <IP> 5555
~~~
~~~
help
clarify <command> -- clarify command format
echo -- send echo request to receive the same response
puts -- print string to peripheral module
get -- get parameter of peripheral module
set -- set parameter of peripheral module
ctrl -- control of board component
~~~
~~~
clarify ctrl
ctrl <component> <command>, where:
	component -- board component:
		led1 -- LED1(GREEN)
		led2 -- LED2(BLUE)
		led3 -- LED3(RED)
	command -- requested action:
		on -- turn on
		off -- turn off
		toggle -- toggle state
Response:
	OK
	Error: <message>
~~~
~~~
ctrl led1 on
OK
~~~
~~~
ctrl led2 on
OK
~~~
~~~
ctrl led3 on
OK
~~~
All user LEDs should be lit after executing the above commands.
Experiment with other commands to control LEDs.
