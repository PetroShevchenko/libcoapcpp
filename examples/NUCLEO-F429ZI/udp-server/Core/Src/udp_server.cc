#include "udp_server.hh"
#include "log.h"
#include "cmsis_os.h"

#define WAITTIME_IN_SECONDS 5U

using namespace std;

UdpServer::UdpServer(int port, std::error_code &ec, bool version4)
    : m_port{port},
    m_version4{version4},
    m_serverAddress{},
    m_serverSocket((version4 ? AF_INET : AF_INET6), SOCK_DGRAM, 0, ec),
    m_receiveBuffer{},
    m_transmitBuffer{},
    m_running{false},
    m_callback{nullptr}
{
}

void UdpServer::init(std::error_code &ec)
{
    LwipSocketAddress *sa = &m_serverAddress;

    if (m_version4)
    {
        sa->type(SOCKET_TYPE_IP_V4);
        sa->address4().sin_family = AF_INET;
        sa->address4().sin_addr.s_addr = INADDR_ANY;
        sa->address4().sin_port = htons(m_port);
    }
    else
    {
        sa->type(SOCKET_TYPE_IP_V6);
        sa->address6().sin6_family = AF_INET6;
        sa->address6().sin6_addr = in6addr_any;
        sa->address6().sin6_scope_id = 0;
        sa->address6().sin6_port = htons(m_port);
    }

    m_serverSocket.bind(&m_serverAddress, ec);
}

void UdpServer::process()
{
    int status;
    fd_set readDescriptors;
    struct timeval tv;
    std::error_code ec;
    LwipSocket * sock =  &m_serverSocket;
    
    LOG("Entering the communication loop");

    while (m_running)
    {
        FD_ZERO (&readDescriptors);
        FD_SET (sock->descriptor(), &readDescriptors);

        tv.tv_sec = WAITTIME_IN_SECONDS;
        tv.tv_usec = 0;

        LOG("select");

        status = ::lwip_select (FD_SETSIZE, &readDescriptors, NULL, NULL, &tv);

        if (status == 0)
        {
            LOG("select() : receive timeout %d seconds is over", WAITTIME_IN_SECONDS);
            continue;
        }

        if (status == -1)
        {
            LOG("select() : error code %d", errno);
            continue;
        }

        ssize_t received = -1;
        LwipSocketAddress clientAddress;

        LOG("check received");
        // if there was received something on socket
        if (FD_ISSET(sock->descriptor(), &readDescriptors))
        {
        	LOG("sock->descriptor()");
            memset(m_receiveBuffer, 0, RECEIVE_BUFFER_SIZE);
        	received = sock->recvfrom(ec, m_receiveBuffer, RECEIVE_BUFFER_SIZE, &clientAddress);
 			if (ec.value())
 			{
 				LOG("sock->recvfrom() error: %s", ec.message().c_str());
 				continue;
 			}
 			if (received <= 0)
 			{
 				LOG("sock->recvfrom() returned %d", received);
 				continue;
 			}

            memset(m_transmitBuffer, 0, TRANSMIT_BUFFER_SIZE);
            string answer(m_transmitBuffer);

 			m_callback(string(m_receiveBuffer), answer, ec);

 			if (ec.value())
 			{
 				LOG("m_callback() error: %s", ec.message().c_str());
 				continue;
 			}
 			sock->sendto(answer.data(), answer.length(), static_cast<const SocketAddress *>(&clientAddress), ec);
 			if (ec.value())
 			{
 				LOG("sock->sendto() error: %s", ec.message().c_str());
 			}
        }
    } // while
    LOG("Exiting the communication loop");
}
