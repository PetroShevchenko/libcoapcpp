#include "udp_server.h"
#include "log.h"
#include <string>

using namespace spdlog;
using namespace std;

#define WAITTIME_IN_SECONDS 5U

namespace Unix
{

void UdpServer::init(std::error_code &ec)
{
	DEBUG_LOG_INIT();
	DEBUG_LOG_ENTER();
	bind(ec);
	DEBUG_LOG_EXIT();
}

void UdpServer::process()
{
    int status;
    fd_set readDescriptors;
    struct timeval tv;
    std::error_code ec;
    const UnixSocket * sock =  reinterpret_cast<const UnixSocket *>(socket());
    
	DEBUG_LOG_INIT();
	DEBUG_LOG_ENTER();

    while (m_running)
    {
        FD_ZERO (&readDescriptors);
        FD_SET (sock->descriptor(), &readDescriptors);

        tv.tv_sec = WAITTIME_IN_SECONDS;
        tv.tv_usec = 0;

        status = ::select (FD_SETSIZE, &readDescriptors, NULL, NULL, &tv);

        if (status == 0)
        {
            debug("select() : receive timeout {0:d} seconds is over", WAITTIME_IN_SECONDS);
            continue;
        }

        if (status == -1)
        {
            debug("select() : error code {0:d}", errno);
            stop();
            continue;
        }
        // if there was received something on socket
        if (FD_ISSET(sock->descriptor(), &readDescriptors))
        {
            handle_request(ec);
            if (ec.value())
            {
                debug("handle_request() : error: {}", ec.message().c_str());
            }
        }
    } // while
	DEBUG_LOG_EXIT();
}

void UdpServer::handle_request(std::error_code &ec)
{
    UnixSocketAddress clientAddress;
    size_t received = BUFFER_SIZE;
	DEBUG_LOG_INIT();
    DEBUG_LOG_ENTER();

    bufferPtr().get()->clear();

    receive(bufferPtr().get()->data(), received, reinterpret_cast<SocketAddress *>(&clientAddress), ec, 0);

    if (ec.value())
    {
        debug("receive() error: {}", ec.message().c_str());
        DEBUG_LOG_EXIT();
    	return;
    }

    string request(reinterpret_cast<char *>(bufferPtr().get()->data()));
    request.resize(received);
    string answer;
    answer.reserve(BUFFER_SIZE);

    m_callback(request, answer, ec);

    if (ec.value())
    {
        debug("m_callback() error: {}", ec.message().c_str());
        answer = "Error: " + ec.message();
        ec.clear();
    }

    send(answer.data(), answer.length(), static_cast<const SocketAddress *>(&clientAddress), ec);
	if (ec.value())
    {
        debug("send() error: {}", ec.message().c_str());
    }
    DEBUG_LOG_EXIT();
}

}// namespace Unix
