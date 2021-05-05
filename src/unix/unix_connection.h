#ifndef _UNIX_CONNECTION_H
#define _UNIX_CONNECTION_H
#include "connection.h"
#include "error.h"

class UnixConnection : public Connection
{
public:
    UnixConnection(ConnectionType type, const char * hostname, int port, std::error_code &ec);
    UnixConnection(const char * uri, std::error_code &ec);
    virtual ~UnixConnection();

public:
    ConnectionType type() const override
    { return m_type; }

protected:
    Socket * create_socket(std::error_code &ec);

protected:
    ConnectionType m_type;
    DnsResolver * m_dns;
    Socket * m_socket;
    SocketAddress * m_sockAddr;
};

class UnixUdpClientConnection : public UnixConnection
{
public:
    UnixUdpClientConnection(const char * hostname, int port, std::error_code &ec)
    : UnixConnection(UDP,hostname, port, ec)
    {}

    UnixUdpClientConnection(const char * uri, std::error_code &ec)
    : UnixConnection(uri, ec)
    {}

    ~UnixUdpClientConnection() = default;

public:
    void connect(std::error_code &ec) override;
    void disconnect(std::error_code &ec) override;
    void send(const void * buffer, size_t length, std::error_code &ec) override;
    void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) override;
};

class UnixDtlsClientConnection : public UnixConnection
{
public:
    UnixDtlsClientConnection(const char * hostname, int port, std::error_code &ec)
    : UnixConnection(DTLS,hostname, port, ec)
    {}

    UnixDtlsClientConnection(const char * uri, std::error_code &ec)
    : UnixConnection(uri, ec)
    {}

    ~UnixDtlsClientConnection() = default;

public:
    void connect(std::error_code &ec) override;
    void disconnect(std::error_code &ec) override;
    void send(const void * buffer, size_t length, std::error_code &ec) override;
    void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) override;
};

Connection * create_client_connection(ConnectionType type, const char * hostname, int port, std::error_code &ec);
Connection * create_client_connection(const char * uri, std::error_code &ec);

#endif
