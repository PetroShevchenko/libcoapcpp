#include "main.h"
#include "lwip.h"
#include "sockets.h"
#include "cmsis_os.h"
#include "log.h"
#include "commands.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdbool.h>

#define UDP_PORT_NUM 5555UL

static struct sockaddr_in serv_addr, client_addr;

static char request_buffer[REQ_BUF_MAX_LEN];
static char answer_buffer[ANSW_BUF_MAX_LEN];

static int UdpServerInit(uint16_t portnum)
{
	uint16_t port;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		LOG("socket() error");
		return -1;
	}

	port = htons((uint16_t)portnum);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = port;

	if(bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1) {
		LOG("bind() error\n");
		close(fd);
		return -1;
	}

	LOG("UDP Server is bound to port %d\n", portnum);
	return fd;
}

void StartUdpServerTask(void const * argument)
{
	int addr_len;
	int retval;
	struct pollfd d;

	if( ( d.fd = UdpServerInit(UDP_PORT_NUM) ) < 0 ) {
		LOG("UdpServerInit(%lu) error", UDP_PORT_NUM);
		return;
	}
	d.events = POLLIN;

	for(;;)
	{
		bzero(&client_addr, sizeof(client_addr));
		addr_len = sizeof(client_addr);

		retval = poll(&d, 1, 5000);

		if (retval == -1)
		{
			close(d.fd);
			break;
		}
		else if (retval)
		{
			ssize_t received;
			cmd_status_t status;
			size_t answer_len;

			if (d.revents & POLLIN )
			{
				d.revents = 0;

				memset(request_buffer, 0, REQ_BUF_MAX_LEN);

				received = recvfrom(d.fd, request_buffer, REQ_BUF_MAX_LEN, MSG_DONTWAIT, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);

				if (received > 0)
				{
					status = command_handler((const char *)request_buffer, (const size_t)received, answer_buffer, &answer_len);

					if (status == CMD_OK)
					{
						LOG("command was handles successfully\n");
					}
					else
					{
						LOG("command_handler() returned error code = %d\n", (int)status);

						if (status == CMD_ERR_FORMAT)
						{
							strncpy(answer_buffer, get_help(), sizeof(answer_buffer));
						}
						else
						{
							sprintf(answer_buffer, "Error: %s\n", status2str (status));
						}
						answer_len = strlen(answer_buffer);
					}
					if (sendto(d.fd, answer_buffer, answer_len,  MSG_DONTWAIT, (const struct sockaddr *)&client_addr, addr_len) == -1)
					{
						LOG("sendto() returned -1 \n");
					}
				}
			}
		}
		else
		{
			LOG("No data within five seconds.\n");
		}
	}
}
