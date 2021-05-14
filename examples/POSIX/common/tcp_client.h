#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H
#include "error.h"


class TcpClient
{
public:
    TcpClient(const char *server, int port)
        : m_server{server}, m_port{port}
    {}

    virtual ~TcpClient() = default;

public:
    virtual void connect(std::error_code &ec) = 0;
    virtual void send(const void * data, size_t size, std::error_code &ec) = 0;
    virtual void receive(std::error_code &ec, void * data, size_t &size, size_t seconds = 0) = 0;
    virtual void disconnect(std::error_code &ec) = 0;

public:
    std::string server() const
    { return m_server; }

    int port() const
    { return m_port; }

protected:
    std::string m_server;
    int         m_port;
};


#endif
