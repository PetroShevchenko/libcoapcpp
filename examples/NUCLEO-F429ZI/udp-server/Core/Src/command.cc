#include "command.hh"
#include "main.h"
#include "log.h"
#include "rtc.h"
#include <cstring>

#define CMD_LOG_ENTER() LOG("Entering %s", __func__)
#define CMD_LOG_EXIT() LOG("Leaving %s", __func__)
#define CMD_LOG(...) LOG(__VA_ARGS__)

#define CMD_NAME_MAX_LEN (sizeof("clarify") - sizeof(char))

extern UART_HandleTypeDef huart3;
extern RTC_HandleTypeDef hrtc;

using namespace std;

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

const char * const puts_usage = "puts <module> <text string>, where:\n"\
		"\tmodule -- peripheral module:\n"\
		"\t\tspi -- SPI1 interface\n"\
		"\t\tuart -- USART3 interface\n"\
		"\t\tusb -- USB_OTG_FS interface\n"\
		"\ttext string -- no more than 240 characters\n"\
		"Response:\n"\
		"\tOK\n"\
		"\tError: <message>\n";

const char * const get_usage = "get <module> <parameter>, where:\n"\
		"\tmodule -- peripheral module:\n"\
		"\t\tspi -- SPI1 interface\n"\
		"\t\tuart -- USART3 interface\n"\
		"\t\tusb -- USB_OTG_FS interface\n"\
		"\t\trtc -- RTC module\n"\
		"\tparameter -- requested parameter:\n"\
		"\t\tbaudrate -- communication speed (bps)\n"\
		"\t\tdata_bits -- number of data bits\n"\
		"\t\tstop_bits -- number of stop bits\n"\
		"\t\tflow_ctrl -- flow control\n"\
		"\t\ttime -- current time\n"\
		"\t\tdate -- current date\n"\
		"Response:\n"\
		"\trequested parameter value\n"\
		"\tError: <message>\n";

const char * const set_usage = "set <module> <parameter> <value>, where:\n"\
		"\tmodule -- peripheral module:\n"\
		"\t\tspi -- SPI1 interface\n"\
		"\t\tuart -- USART3 interface\n"\
		"\t\tusb -- USB_OTG_FS interface\n"\
		"\t\trtc -- RTC module\n"\
		"\tparameter -- requested parameter:\n"\
		"\t\tbaudrate -- communication speed (bps)\n"\
		"\t\tdata_bits -- number of data bits\n"\
		"\t\tstop_bits -- number of stop bits\n"\
		"\t\tflow_ctrl -- flow control\n"\
		"\t\ttime -- current time in format: HH:MM:SS\n"\
		"\t\tdate -- current date in format: DD.MM.YYYY\n"\
		"value -- new value of parameter\n"\
		"Response:\n"\
		"\tOK\n"\
		"\tError: <message>\n";

const char * const help_response = "clarify <command> -- clarify command format\n"\
		"echo -- send echo request to receive the same response\n"\
		"puts -- print string to peripheral module\n"\
		"get -- get parameter of peripheral module\n"\
		"set -- set parameter of peripheral module\n";
} // namespace

Command::CommandInfo Command::m_context[CMD_QTY] = {
	{CMD_HELP, "help", help_usage, help_cmd_handler},
	{CMD_CLARIFY, "clarify", clarify_usage, clarify_cmd_handler},
	{CMD_ECHO, "echo", echo_usage, echo_cmd_handler},
	{CMD_PUTS, "puts", puts_usage, puts_cmd_handler},
	{CMD_GET, "get", get_usage, get_cmd_handler},
	{CMD_SET, "set", set_usage, set_cmd_handler},
};

static Command command;

void command_handler(const string &request, std::string &answer, std::error_code &ec)
{
	Command::handler(&command, request, answer, ec);
}

void Command::handler(void *data, const string &request, string &answer, error_code &ec)
{
	CMD_LOG_ENTER();
	if (data == nullptr)
	{
        ec = make_system_error(EFAULT);
        CMD_LOG_EXIT();
        return;
	}

	Command *pObj = static_cast<Command *>(data);

	for (size_t i = 0; i < CMD_QTY; i++)
	{
		if (strncmp(request.data(), pObj->m_context[i].name, strlen(pObj->m_context[i].name)) == 0)
		{
			pObj->m_context[i].handler(request, answer, ec);
			CMD_LOG_EXIT();
			return;
		}
	}
	answer = help;

	CMD_LOG_EXIT();
}

void Command::help_cmd_handler(const string &request, string &answer, error_code &ec)
{
	const int len = strlen(help_response);
	CMD_LOG("%s",request.c_str());
	if (len > answer.length())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
		return;
	}
	answer = help_response;
}

void Command::clarify_cmd_handler(const string &request, string &answer, error_code &ec)
{
	char subcommand[CMD_NAME_MAX_LEN];

	sscanf(request.c_str(), "clarify %s", subcommand);

	CMD_LOG("%s", subcommand);

	for (size_t i = 0; i < CMD_QTY; i++)
	{
		if (strncmp(subcommand, m_context[i].name, strlen(m_context[i].name)) == 0)
		{
			size_t len = strlen(m_context[i].help);
			if (len > answer.length())
			{
				CMD_LOG("len=%zu", len);
				CMD_LOG("answer.length()=%zu", answer.length());
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
	if (len > answer.length())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
		return;		
	}
	answer = request.substr(offset,len);
}

void Command::puts_cmd_handler(const string &request, string &answer, error_code &ec)
{
	//puts <module> <text string>
	size_t pos = request.find("puts");
	size_t offset = pos + sizeof("puts");
	size_t len  = request.length() - offset;
	if (len > answer.length())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
		return;		
	}
	char moduleName[sizeof("uart")];
	sscanf(request.substr(offset,len).c_str(), "%s ", moduleName);
	if (strncmp(moduleName, "uart", strlen("uart")) == 0)
	{
		offset += strlen("uart ");
		size_t size = request.length() - offset;
		HAL_UART_Transmit(&huart3,
						(uint8_t *)request.substr(offset, size).c_str(),
						size,
						TRANSMIT_TIMEOUT
					);
		answer = "OK\n";
	}
	else
	{
		ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);		
	}
}

void Command::get_cmd_handler(const string &request, string &answer, error_code &ec)
{
	//get <module> <parameter>
	size_t pos = request.find("get");
	size_t offset = pos + sizeof("get");
	size_t len  = request.length() - offset;
	if (len > answer.length())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
		return;		
	}
	char moduleName[sizeof("uart")];
	sscanf(request.substr(offset,len).c_str(), "%s ", moduleName);
	if (strncmp(moduleName, "rtc", strlen("rtc")) == 0)
	{
		offset += strlen("rtc ");
		size_t size = request.length() - offset;
		const char *parameter = request.substr(offset, size).c_str();
  		RTC_TimeTypeDef time;
  		RTC_DateTypeDef date;

  	  	if(Rtc_Get_DateTime(&hrtc, &time, &date) != HAL_OK)
  	  	{
  	  		ec = make_system_error(EBUSY);
  	  		return;
  	  	}

		if (strncmp(parameter, "date", strlen("date")) == 0)
		{
			// current date in format: DD.MM.YYYY
			char dateStr[sizeof("DD.MM.YYYY\n")];
			sprintf(
					dateStr,
					"%02u.%02u.%04u\n",
					date.Date,
					RtcConvertMonthFromBSD2Bin(date.Month),
					date.Year+2000
				);
			answer = dateStr;
			return;
		}
		else if (strncmp(parameter, "time", strlen("time")) == 0)
		{
			// current time in format: HH:MM:SS
			char timeStr[sizeof("HH:MM:SS\n")];
			sprintf(
					timeStr,
					"%02u:%02u:%02u\n",
					time.Hours,
					time.Minutes,
					time.Seconds
				);
			answer = timeStr;
			return;
		}
	}
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
}

void Command::set_cmd_handler(const string &request, string &answer, error_code &ec)
{
	CMD_LOG_ENTER();
	//set <module> <parameter> <value>
	size_t pos = request.find("get");
	size_t offset = pos + sizeof("get");
	size_t len  = request.length() - offset;
	if (len > answer.length())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
		CMD_LOG_EXIT();
		return;		
	}
	char moduleName[sizeof("uart")];
	sscanf(request.substr(offset,len).c_str(), "%s ", moduleName);
	if (strncmp(moduleName, "rtc", strlen("rtc")) == 0)
	{
		offset += strlen("rtc ");
		size_t size = request.length() - offset;

		char parameter[sizeof("data_bits")];
		sscanf(request.substr(offset, size).c_str(), "%s ", parameter);

  		RTC_TimeTypeDef time;
  		RTC_DateTypeDef date;

  	  	if(Rtc_Get_DateTime(&hrtc, &time, &date) != HAL_OK)
  	  	{
  	  		ec = make_system_error(EBUSY);
  	  		CMD_LOG_EXIT();
  	  		return;
  	  	}
  	  	
		unsigned int year, month, day, hours, min, sec;

		if (strncmp(parameter, "date", strlen("date")) == 0)
		{
			// current date in format: DD.MM.YYYY
			offset += strlen("date ");
			size = request.length() - offset;

			const char *dateStr = request.substr(offset, size).c_str();
			sscanf(dateStr, "%02u.%02u.%04u", &day, &month, &year);
			date.Date = day;
			date.Month = RtcConvertMonthFromBin2BSD((int)month);
			date.Year = year - 2000;
		}
		else if (strncmp(parameter, "time", strlen("time")) == 0)
		{
			// current time in format: HH:MM:SS
			offset += strlen("time ");
			size = request.length() - offset;

			const char *timeStr = request.substr(offset, size).c_str();
			sscanf(timeStr, "%02u:%02u:%02u", &hours, &min, &sec);
			time.Hours = hours;
			time.Minutes = min;
			time.Seconds = sec;
		}
		else
		{
  	  		ec = make_system_error(EINVAL);
			CMD_LOG("parameter=%s", parameter);
  	  		CMD_LOG_EXIT();
  	  		return;
		}

		if (Rtc_Set_DateTime(
					&hrtc,
					date.Year,
					date.Month,
					date.Date,
					time.Hours,
					time.Minutes,
					time.Seconds,
					0
				) != HAL_OK)
		{
  	  		ec = make_system_error(EBUSY);
  	  		CMD_LOG_EXIT();
  	  		return;
		}
		answer = "OK\n";
	}
	else
	{
		ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
	}
	CMD_LOG_EXIT();
}
