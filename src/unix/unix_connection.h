#ifndef _UNIX_CONNECTION_H
#define _UNIX_CONNECTION_H
#include "connection.h"
#include "error.h"

class UnixConnection : public Connection
{
public:
    UnixConnection(ConnectionType type, const char * hostname, int port, std::error_code &ec);
    UnixConnection(const char * uri, std::error_code &ec);
    ~UnixConnection();

public:
    void connect(std::error_code &ec) override;
    void disconnect(std::error_code &ec) override;
    void send(std::error_code &ec) override;
    void receive(std::error_code &ec) override;

private:
    Socket * create_socket(std::error_code &ec);

private:
    ConnectionType m_type;
    DnsResolver * m_dns;
    Socket * m_socket;
    SocketAddress * m_sockAddr;
};


#endif
