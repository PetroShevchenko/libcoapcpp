#include "command.hh"
#include "main.h"
#include "log.h"

void command_handler(const std::string &request, std::string &answer, std::error_code &ec)
{
	LOG("Entering command_handler()");
	LOG("request.length()=%d", request.length());
	answer = request;
	LOG("answer.length()=%d", answer.length());
	LOG("Exiting command_handler()");
}	