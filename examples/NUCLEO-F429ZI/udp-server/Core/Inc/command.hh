#ifndef COMMAND_HH
#define COMMAND_HH
#include "error.h"
#include <string>

namespace
{
	extern const char * const help;
	extern const char * const help_usage;
	extern const char * const clarify_usage;
	extern const char * const echo_usage;
	extern const char * const puts_usage;
	extern const char * const get_usage;
	extern const char * const set_usage;
}

class Command
{
public:
	Command()
	{}

	~Command()
	{}

public:
	static void handler(void *data, const std::string &request, std::string &answer, std::error_code &ec);

private:
	typedef void (*CommandHandler)(const std::string &request, std::string &answer, std::error_code &ec);

	typedef enum {
		CMD_HELP = 0,
		CMD_CLARIFY,
		CMD_ECHO,
		CMD_PUTS,
		CMD_GET,
		CMD_SET,
		CMD_QTY
	} CommandCode;

	typedef struct {
		CommandCode code;
		const char *name;
		const char *help;
		CommandHandler handler;
	} CommandInfo;

private:
	static void help_cmd_handler(const std::string &request, std::string &answer, std::error_code &ec);
	static void clarify_cmd_handler(const std::string &request, std::string &answer, std::error_code &ec);
	static void echo_cmd_handler(const std::string &request, std::string &answer, std::error_code &ec);
	static void puts_cmd_handler(const std::string &request, std::string &answer, std::error_code &ec);
	static void get_cmd_handler(const std::string &request, std::string &answer, std::error_code &ec);
	static void set_cmd_handler(const std::string &request, std::string &answer, std::error_code &ec);

private:
	static CommandInfo m_context[CMD_QTY];
};

void command_handler(const std::string &request, std::string &answer, std::error_code &ec);

#endif
