#include "commands.h"
#include "log.h"
#include <string.h>

typedef enum {
	CMD_HELP = 0,
	CMD_CLARIFY,
	CMD_ECHO,
	CMD_PUTS,
	CMD_GET,
	CMD_SET,
	CMD_QTY
} cmd_t;

typedef cmd_status_t (*command_handler_t)(const char *request, const size_t req_size, char *answer, size_t *answ_size);

typedef struct {
	cmd_t code;
	const char *name;
	const char *help;
	command_handler_t handler;
} cmd_info_t;

static cmd_status_t help_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size);
static cmd_status_t clarify_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size);
static cmd_status_t echo_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size);
static cmd_status_t puts_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size);
static cmd_status_t get_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size);
static cmd_status_t set_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size);

static const char * const help_usage = "help\n"\
		"Response: list of commands\n";

static const char * const clarify_usage = "clarify <command>\n"\
		"Response:\n"\
		"\trequested command format\n"\
		"\tError: <message>\n";

static const char * const echo_usage = "echo <text string>, where:\n"\
		"\ttext string -- no more than 240 characters\n"\
		"Response:\n"
		"\tthe same text string\n"\
		"\tError: <message>\n";

static const char * const puts_usage = "puts <module> <text string>, where:\n"\
		"\tmodule -- peripheral module:\n"\
		"\t\tspi -- SPI1 interface\n"\
		"\t\tuart -- USART3 interface\n"\
		"\t\tusb -- USB_OTG_FS interface\n"\
		"\ttext string -- no more than 240 characters\n"\
		"Response:\n"\
		"\tOK\n"\
		"\tError: <message>\n";

static const char * const get_usage = "get <module> <parameter>, where:\n"\
		"\tmodule -- peripheral module:\n"\
		"\t\tspi -- SPI1 interface\n"\
		"\t\tuart -- USART3 interface\n"\
		"\t\tusb -- USB_OTG_FS interface\n"\
		"\tparameter -- requested parameter:\n"\
		"\t\tbaudrate -- communication speed (bps)\n"\
		"\t\tdata_bits -- number of data bits\n"\
		"\t\tstop_bits -- number of stop bits\n"\
		"\t\tflow_ctrl -- flow control\n"\
		"Response:\n"\
		"\trequested parameter value\n"\
		"\tError: <message>\n";

static const char * const set_usage = "set <module> <parameter> <value>, where:\n"\
		"\tmodule -- peripheral module:\n"\
		"\t\tspi -- SPI1 interface\n"\
		"\t\tuart -- USART3 interface\n"\
		"\t\tusb -- USB_OTG_FS interface\n"\
		"\tparameter -- requested parameter:\n"\
		"\t\tbaudrate -- communication speed (bps)\n"\
		"\t\tdata_bits -- number of data bits\n"\
		"\t\tstop_bits -- number of stop bits\n"\
		"\t\tflow_ctrl -- flow control\n"\
		"value -- new value of parameter\n"\
		"Response:\n"\
		"\tOK\n"\
		"\tError: <message>\n";

static const char * const help_response = "clarify <command> -- clarify command format\n"\
		"echo -- send echo request to receive the same response\n"\
		"puts -- print string to peripheral module\n"\
		"get -- get parameter of peripheral module\n"\
		"set -- set parameter of peripheral module\n";

static cmd_info_t command_list[CMD_QTY] = {
		{CMD_HELP, 		"help", 	help_usage, 	help_cmd_handler},
		{CMD_CLARIFY,	"clarify",	clarify_usage, 	clarify_cmd_handler},
		{CMD_ECHO, 		"echo", 	echo_usage, 	echo_cmd_handler},
		{CMD_PUTS, 		"puts", 	puts_usage, 	puts_cmd_handler},
		{CMD_GET, 		"get", 		get_usage, 		get_cmd_handler},
		{CMD_SET, 		"set", 		set_usage, 		set_cmd_handler}
};

#define CMD_NAME_MAX_LEN (sizeof("clarify") - sizeof(char))

static const char *help = "Use the following command to get help:\n"\
		"help -- shows the list of commands\n";

static cmd_status_t help_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size)
{
	*answ_size = strlen(help_response);
	if (*answ_size > ANSW_BUF_MAX_LEN)
	{
		return CMD_ERR_BUF_OVERFLOW;
	}
	memcpy(answer, help_response, *answ_size);
	return CMD_OK;
}

static cmd_status_t clarify_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size)
{
	char subcommand[CMD_NAME_MAX_LEN];

	sscanf(request, "clarify %s", subcommand);

	LOG("%s", subcommand);

	for (size_t i = 0; i < CMD_QTY; i++)
	{
		if (strncmp(subcommand, command_list[i].name, strlen(command_list[i].name)) == 0)
		{
			*answ_size = strlen(command_list[i].help);
			if (*answ_size > ANSW_BUF_MAX_LEN)
			{
				return CMD_ERR_BUF_OVERFLOW;
			}
			memcpy(answer, command_list[i].help, *answ_size);
			return CMD_OK;
		}
	}
	return CMD_ERR_SUBCOMMAND;
}

static cmd_status_t echo_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size)
{
	char * p = strstr(request, "echo");
	if (p == NULL)
	{
		return CMD_ERR_FORMAT;
	}
	*answ_size = req_size - sizeof("echo");
	if (*answ_size > ANSW_BUF_MAX_LEN)
	{
		return CMD_ERR_BUF_OVERFLOW;
	}
	p += sizeof("echo");
	memcpy(answer, p, *answ_size);
	return CMD_OK;
}

static cmd_status_t puts_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size)
{
	return CMD_ERR_NOT_IMPLEMENTED;
}

static cmd_status_t get_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size)
{
	return CMD_ERR_NOT_IMPLEMENTED;
}

static cmd_status_t set_cmd_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size)
{
	return CMD_ERR_NOT_IMPLEMENTED;
}

const char * status2str(cmd_status_t status)
{
	switch(status)
	{
	case CMD_OK:
		return "OK";
	case CMD_ERR_ARG:
		return "Wrong argument";
	case CMD_ERR_FORMAT:
		return "Wrong format";
	case CMD_ERR_NOT_IMPLEMENTED:
		return "Not implemented";
	case CMD_ERR_BUF_OVERFLOW:
		return "Buffer overflow";
	case CMD_ERR_SUBCOMMAND:
		return "Wrong subcommand";
	default:
		return "Unknown error";
	}
}

const char * get_help() { return help; }

cmd_status_t command_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size)
{
	if (request == NULL
			|| answer == NULL
			|| answ_size == NULL
			|| req_size == 0
			|| req_size > REQ_BUF_MAX_LEN)
	{
		return CMD_ERR_ARG;
	}

	for (size_t i = 0; i < CMD_QTY; i++)
	{
		if (strncmp(request, command_list[i].name, strlen(command_list[i].name)) == 0)
		{
			return command_list[i].handler(request, req_size, answer, answ_size);
		}
	}

	return CMD_ERR_FORMAT;
}
