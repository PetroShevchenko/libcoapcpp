#include "unix_udp_server.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

using namespace spdlog;

namespace Unix
{

UdpServerConnection::UdpServerConnection(int port, bool version4, std::shared_ptr<Buffer> bufferPtr, std::error_code &ec)
    : ServerConnection(UDP, port, version4, ec),
      m_bound{false},
      m_socket{new UnixSocket(version4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0, ec)},
      m_address{new UnixSocketAddress()},
      m_bufferPtr{std::move(bufferPtr)}
{
    if (ec.value()) return;
    UnixSocketAddress *sa = static_cast<UnixSocketAddress *>(m_address);
    if (version4)
    {
        sa->type(SOCKET_TYPE_IP_V4);
        sa->address4().sin_family = AF_INET;
        sa->address4().sin_addr.s_addr = INADDR_ANY;
        sa->address4().sin_port = htons(port);
    }
    else
    {
        sa->type(SOCKET_TYPE_IP_V6);
        sa->address6().sin6_family = AF_INET6;
        sa->address6().sin6_addr = in6addr_any;
        sa->address6().sin6_scope_id = 0;
        sa->address6().sin6_port = htons(port);
    }
    const int on = 1; // reuse address option

    m_socket->setsockoption(SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int), ec);
    if (ec.value()) return;

    if (!version4)
    {
        const int off = 0; // disable IPv6 only
        m_socket->setsockoption(IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(int), ec);
    }       
}

void UdpServerConnection::close(std::error_code &ec)
{
    ec.clear();
    m_bound = false;

    if (m_socket)
    {
        delete m_socket;
        m_socket = nullptr;
    }
    if (m_address)
    {
        delete m_address;
        m_address = nullptr;
    }
}

void UdpServerConnection::send(const void * buffer, size_t length, std::error_code &ec)
{
    if (!m_bound)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_SOCKET_NOT_BOUND);
        return;
    }
    ssize_t sent = m_socket->sendto(buffer, length, static_cast<const SocketAddress *>(m_address), ec);
    if (!ec.value())
    {
        if (static_cast<size_t>(sent) != length)
        {
            ec = make_error_code(CoapStatus::COAP_ERR_INCOMPLETE_SEND);
        }
    }
}

void UdpServerConnection::receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds)
{
    if (!m_bound)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_SOCKET_NOT_BOUND);
        return;
    }
    if (seconds)
    {
        m_socket->set_timeout(seconds, ec);
        if(ec.value())
            return;
    }
    ssize_t received = m_socket->recvfrom(ec, buffer, length);
    if (!ec.value())
    {
        length = static_cast<size_t>(received);
    }
}

void UdpServerConnection::bind(std::error_code &ec)
{
    m_socket->bind(m_address, ec);
    if (ec.value())
    {
        debug("bind error: {}", ec.message());
    }
    else m_bound = true;
}

} //namespace Unix
