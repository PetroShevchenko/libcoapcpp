#ifndef COMMANDS_H
#define COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define REQ_BUF_MAX_LEN 256U
#define ANSW_BUF_MAX_LEN 512U

typedef enum {
	CMD_OK = 0,
	CMD_ERR_UNKNOWN = -1,
	CMD_ERR_ARG = -2,
	CMD_ERR_FORMAT = -3,
	CMD_ERR_NOT_IMPLEMENTED = -4,
	CMD_ERR_BUF_OVERFLOW = -5,
	CMD_ERR_SUBCOMMAND = -6,
} cmd_status_t;

const char * status2str(cmd_status_t status);
const char * get_help(void);
cmd_status_t command_handler(const char *request, const size_t req_size, char *answer, size_t *answ_size);

#ifdef __cplusplus
}
#endif

#endif
