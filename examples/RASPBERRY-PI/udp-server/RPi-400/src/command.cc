#include "command.h"
#include "log.h"
#include <cstring>

#define CMD_NAME_MAX_LEN (sizeof("clarify") - sizeof(char))

using namespace std;
using namespace spdlog;

namespace
{
const char * const help = "Use the following command to get help:\n"\
		"help -- shows the list of commands\n";

const char * const help_usage = "help\n"\
		"Response: list of commands\n";

const char * const clarify_usage = "clarify <command>\n"\
		"Response:\n"\
		"\trequested command format\n"\
		"\tError: <message>\n";

const char * const echo_usage = "echo <text string>, where:\n"\
		"\ttext string -- no more than 240 characters\n"\
		"Response:\n"
		"\tthe same text string\n"\
		"\tError: <message>\n";

const char * const help_response = "clarify <command> -- clarify command format\n"\
		"echo -- send echo request to receive the same response\n";
} // namespace

Command::CommandInfo Command::m_context[CMD_QTY] = {
	{CMD_HELP, "help", help_usage, help_cmd_handler},
	{CMD_CLARIFY, "clarify", clarify_usage, clarify_cmd_handler},
	{CMD_ECHO, "echo", echo_usage, echo_cmd_handler},
};

static Command command;

void command_handler(const string &request, std::string &answer, std::error_code &ec)
{
	Command::handler(&command, request, answer, ec);
}

void Command::handler(void *data, const string &request, string &answer, error_code &ec)
{
	DEBUG_LOG_ENTER();
	if (data == nullptr)
	{
        ec = make_system_error(EFAULT);
        DEBUG_LOG_EXIT();
        return;
	}

	Command *pObj = static_cast<Command *>(data);

	for (size_t i = 0; i < CMD_QTY; i++)
	{
		if (strncmp(request.data(), pObj->m_context[i].name, strlen(pObj->m_context[i].name)) == 0)
		{
			pObj->m_context[i].handler(request, answer, ec);
			DEBUG_LOG_EXIT();
			return;
		}
	}
	answer = help;

	DEBUG_LOG_EXIT();
}

void Command::help_cmd_handler(const string &request, string &answer, error_code &ec)
{
	const int len = strlen(help_response);
	debug("{}",request.c_str());
	if (len > answer.capacity())
	{
		debug("len={0:d}", len);
		debug("answer.capacity()={0:d}", answer.capacity());
		ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
		return;
	}
	answer = help_response;
}

void Command::clarify_cmd_handler(const string &request, string &answer, error_code &ec)
{
	char subcommand[CMD_NAME_MAX_LEN];

	sscanf(request.c_str(), "clarify %s", subcommand);

	debug("{}", subcommand);

	for (size_t i = 0; i < CMD_QTY; i++)
	{
		if (strncmp(subcommand, m_context[i].name, strlen(m_context[i].name)) == 0)
		{
			size_t len = strlen(m_context[i].help);
			if (len > answer.capacity())
			{
				debug("len={0:d}", len);
				debug("answer.capacity()={0:d}", answer.capacity());
				ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
				return;
			}
			answer = m_context[i].help;
			return;
		}
	}
}

void Command::echo_cmd_handler(const string &request, string &answer, error_code &ec)
{
	size_t pos = request.find("echo");
	size_t offset = pos + sizeof("echo");
	size_t len  = request.length() - offset;
	if (len > answer.capacity())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
		return;		
	}
	answer = request.substr(offset,len);
}
