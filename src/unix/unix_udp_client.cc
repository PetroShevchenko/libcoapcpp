#include "unix_udp_client.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

using namespace spdlog;

namespace Unix
{

void UdpClientConnection::connect(std::error_code &ec)
{
    m_dns->hostname2address(ec);
    if (ec.value())
    {
        return;
    }

    m_address = m_dns->create_socket_address(ec);
    if (ec.value())
    {
        return;
    }

    if (m_socket)
    {
        delete m_socket;
        m_socket = nullptr;
    }

    m_socket = create_socket(type(), m_dns, ec);
    if (ec.value())
    {
        delete m_address;
        m_address = nullptr;
    }
}

void UdpClientConnection::close(std::error_code &ec)
{
    ec.clear();

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

void UdpClientConnection::send(const void * buffer, size_t length, const SocketAddress *destAddr, std::error_code &ec)
{
    if (m_socket == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_CONNECTED);
        return;
    }
    ssize_t sent = m_socket->sendto(buffer, length, destAddr, ec);
    if (!ec.value())
    {
        if (static_cast<size_t>(sent) != length)
        {
            ec = make_error_code(CoapStatus::COAP_ERR_INCOMPLETE_SEND);
        }
    }
}

void UdpClientConnection::receive(void * buffer, size_t &length, SocketAddress *srcAddr, std::error_code &ec, size_t seconds)
{
    if (m_socket == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_CONNECTED);
        return;
    }

    if (seconds)
    {
        m_socket->set_timeout(seconds, ec);
        if(ec.value())
            return;
    }

    ssize_t received = m_socket->recvfrom(ec, buffer, length, srcAddr);
    if (!ec.value())
    {
        length = static_cast<size_t>(received);
    }
}

void UdpClientConnection::send(const void * buffer, size_t length, std::error_code &ec)
{
    if (m_socket == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_CONNECTED);
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

void UdpClientConnection::receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds)
{
    if (m_socket == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_CONNECTED);
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

}// namespace unix
