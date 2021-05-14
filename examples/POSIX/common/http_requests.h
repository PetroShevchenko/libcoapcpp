#ifndef _HTTP_REQUESTS_H
#define _HTTP_REQUESTS_H
#include "tcp_client.h"
#include "error.h"

void http_get_request(TcpClient *client, std::error_code &ec);


#endif
