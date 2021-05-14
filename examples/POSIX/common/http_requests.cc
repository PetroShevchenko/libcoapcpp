#include "http_requests.h"
#include "tcp_client.h"
#include "error.h"
#include "spdlog/spdlog.h"
#include <spdlog/fmt/fmt.h>
#include <cstring>

using namespace std;
using namespace spdlog;

void http_get_request(TcpClient *client, std::error_code &ec)
{
    set_level(level::debug);
    if (client == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    client->connect(ec);

    if (ec.value())
    {
        debug("connect() : {}", ec.message());
        return;
    }

    const char * payload = "GET /wp-content/uploads/2020/01/response.txt HTTP/1.1\r\n\
Host: cxemotexnika.org\r\n\
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:88.0) Gecko/20100101 Firefox/88.0\r\n\
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n\
Accept-Language: en-US,en;q=0.5\r\n\
Accept-Encoding: gzip, deflate\r\n\
Connection: keep-alive\r\n\
Upgrade-Insecure-Requests: 1\r\n\
Cache-Control: max-age=0\r\n\r\n";

    client->send(payload, strlen(payload), ec);

    debug("send() : {}", ec.message());

    if (ec.value())
    {
        return;
    }

    char response[4096];
    size_t sz = sizeof(response);
    size_t seconds = 1;

    client->receive(ec, response, sz, seconds);

    debug("receive() : {}", ec.message());

    if (ec.value())
    {
        return;
    }

    debug("There were received {0:d} bytes :",sz);
    debug("{}", response);

    client->disconnect(ec);
    debug("disconnect() : {}", ec.message());
}




