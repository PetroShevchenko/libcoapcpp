#ifndef _UNIX_UDP_SERVER_H
#define _UNIX_UDP_SERVER_H
#include <memory>
#include <array>
#include "connection.h"
#include "unix_socket.h"
#include "utils.h"
#include "error.h"
#include <cstring>

namespace Unix
{

#define BUFFER_SIZE 1600UL

class Buffer
{
public:
    Buffer(const size_t length)
    : m_length{length},
      m_data{new uint8_t [length]}
    { memset(m_data, 0, length); }

    ~Buffer()
    { delete [] m_data; }

    size_t length() const
    { return m_length; }

    uint8_t *data()
    { return m_data; }

private:
    const size_t m_length;
    uint8_t     *m_data;
};

class UdpServerConnection : public ServerConnection
{
public:
    UdpServerConnection(int port, bool version4, std::shared_ptr<Buffer> bufferPtr, std::error_code &ec);
    UdpServerConnection(int port, bool version4, std::error_code &ec)
     : UdpServerConnection(port, version4, std::move(std::make_shared<Buffer>(BUFFER_SIZE)), ec)
    {}
    ~UdpServerConnection()
    {
        std::error_code ec;
        close(ec);
    }

public:
    void close(std::error_code &ec) override;
    void send(const void * buffer, size_t length, std::error_code &ec) override;
    void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) override;
    void bind(std::error_code &ec) override;

    void listen(std::error_code &ec, int max_connections_in_queue = 1) override
    { 
        (void)max_connections_in_queue;
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
    }

    Socket * accept(std::error_code * ec = nullptr) override
    { 
        if (ec) *ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
        return nullptr;
    }

public:
    bool bound() const
    { return m_bound; }

    const Socket * socket() const
    { return static_cast<const Socket *>(m_socket); }

    const SocketAddress * address() const
    { return static_cast<const SocketAddress *>(m_address); }

    std::shared_ptr<Buffer> &bufferPtr()
    { return m_bufferPtr; }

private:
    bool                      m_bound;
    Socket                    *m_socket;
    SocketAddress             *m_address;
    std::shared_ptr<Buffer>   m_bufferPtr;
};

} //namespace Unix

#endif
