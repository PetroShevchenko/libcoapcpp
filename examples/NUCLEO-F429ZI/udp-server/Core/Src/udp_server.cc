#include "udp_server.hh"
#include "log.h"
#include "cmsis_os.h"

#define WAITTIME_IN_SECONDS 5U

#define UDP_LOG_ENTER() LOG("Entering %s", __func__)
#define UDP_LOG_EXIT() LOG("Leaving %s", __func__)
#define UDP_LOG(...) LOG(__VA_ARGS__)

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

void UdpServer::handle_request(std::error_code &ec)
{
    ssize_t received = -1;
    LwipSocketAddress clientAddress;
    LwipSocket * sock =  &m_serverSocket;

    UDP_LOG_ENTER();

    memset(m_receiveBuffer, 0, RECEIVE_BUFFER_SIZE);

    received = sock->recvfrom(ec, m_receiveBuffer, RECEIVE_BUFFER_SIZE, &clientAddress);
    if (ec.value())
    {
        UDP_LOG("sock->recvfrom() error: %s", ec.message().c_str());
        UDP_LOG_EXIT();
        return;
    }
    if (received <= 0)
    {
        UDP_LOG("sock->recvfrom() returned %d", received);
        UDP_LOG_EXIT();
        ec = make_error_code(CoapStatus::COAP_ERR_RECEIVE);
        return;
    }

    memset(m_transmitBuffer, 0, TRANSMIT_BUFFER_SIZE);

    string answer(m_transmitBuffer);
    answer.resize(sizeof(m_transmitBuffer));
    string request(m_receiveBuffer);
    request.resize(received);

    m_callback(request, answer, ec);

    if (ec.value())
    {
        UDP_LOG("m_callback() error: %s", ec.message().c_str());
        answer = "Error: " + ec.message();
        ec.clear();
    }
    sock->sendto(answer.data(), answer.length(), static_cast<const SocketAddress *>(&clientAddress), ec);
    if (ec.value())
    {
        UDP_LOG("sock->sendto() error: %s", ec.message().c_str());
    }
    UDP_LOG_EXIT();
}

void UdpServer::process()
{
    int status;
    fd_set readDescriptors;
    struct timeval tv;
    std::error_code ec;
    LwipSocket * sock =  &m_serverSocket;
    
    UDP_LOG_ENTER();

    while (m_running)
    {
        FD_ZERO (&readDescriptors);
        FD_SET (sock->descriptor(), &readDescriptors);

        tv.tv_sec = WAITTIME_IN_SECONDS;
        tv.tv_usec = 0;

        status = ::lwip_select (FD_SETSIZE, &readDescriptors, NULL, NULL, &tv);

        if (status == 0)
        {
            UDP_LOG("select() : receive timeout %d seconds is over", WAITTIME_IN_SECONDS);
            continue;
        }

        if (status == -1)
        {
            UDP_LOG("select() : error code %d", errno);
            continue;
        }
        // if there was received something on socket
        if (FD_ISSET(sock->descriptor(), &readDescriptors))
        {
            handle_request(ec);
            if (ec.value())
            {
                UDP_LOG("handle_request() : error: %s", ec.message().c_str());
            }
        }
    } // while
    UDP_LOG_EXIT();
}

void UdpServer::process2()
{
    UDP_LOG_ENTER();

    int status;
    std::error_code ec;
    LwipSocket * sock =  &m_serverSocket;

    struct pollfd d;
    d.events = POLLIN;
    d.fd = sock->descriptor();

    while (m_running)
    {
        status = ::lwip_poll (&d, 1, WAITTIME_IN_SECONDS*1000);
 
        if (status == 0)
        {
            UDP_LOG("poll() : receive timeout %d seconds is over", WAITTIME_IN_SECONDS);
            continue;
        }

        if (status == -1)
        {
            UDP_LOG("poll() : error code %d", errno);
            ::lwip_close(d.fd);
            break;
        }
        // if there was received something on socket
        if (d.revents & POLLIN )
        {
            d.revents = 0;
            handle_request(ec);
            if (ec.value())
            {
                UDP_LOG("handle_request() : error: %s", ec.message().c_str());
            }
        }
    } // while
    UDP_LOG_EXIT();
}
