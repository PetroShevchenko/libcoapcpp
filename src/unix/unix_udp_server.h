#ifndef _UNIX_UDP_SERVER_H
#define _UNIX_UDP_SERVER_H
#include "connection.h"
#include "unix_socket.h"
#include "utils.h"
#include "error.h"

namespace Unix
{

class UdpServer : public ServerConnection
{
public:
    UdpServer(int port, bool version4, std::error_code &ec);
    ~UdpServer()
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
    { ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED); }

    Socket * accept(std::error_code * ec = nullptr) override
    { 
        if (ec) *ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
        return nullptr;
    }

private:
    bool           m_bound;
    Socket        *m_socket;
    SocketAddress *m_address;
};

} //namespace Unix

#endif
