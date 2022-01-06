#ifndef COMMAND_HH
#define COMMAND_HH
#include "error.h"
#include <string>

class CommandHandler
{
public:
	CommandHandler()
	{}

	~CommandHandler()
	{}
};

void command_handler(const std::string &request, std::string &answer, std::error_code &ec);

#endif
