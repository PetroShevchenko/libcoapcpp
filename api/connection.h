#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "dns_resolver.h"
#include "socket.h"
#include "error.h"
#include <string>

enum ConnectionType
{
    TCP,
    UDP,
    TLS,
    DTLS
};

inline bool is_connection_type(ConnectionType type)
{ return !(type < TCP || type > DTLS); }

extern bool uri2connection_type(const char * uri, ConnectionType &type);

struct Connection
{
    Connection(ConnectionType type, int port, std::error_code &ec)
        : m_type{type},
          m_port{port}
    {
        if (!is_connection_type(m_type))
        { ec = make_system_error(EINVAL); }
    }

    virtual ~Connection() = default;
    virtual void send(const void * buffer, size_t length, std::error_code &ec) = 0;
    virtual void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) = 0;
    virtual void close(std::error_code &ec) = 0;

    ConnectionType type() const
    { return m_type; }

    int port() const
    { return m_port; }

protected:
    ConnectionType  m_type;
    int             m_port;
};

struct ClientConnection : public Connection
{
    ClientConnection(ConnectionType type, const char * hostname, int port, std::error_code &ec)
        : Connection(type, port, ec),
         m_uri{},
         m_hostname{hostname}
    {}

    ClientConnection(const char * uri, std::error_code &ec)
        : Connection(UDP, -1, ec),
         m_uri{uri},
         m_hostname{}
    {
        if (!uri2connection_type(uri, m_type))
        { ec = make_system_error(EINVAL); }
    }

    virtual ~ClientConnection() = default;
    virtual void connect(std::error_code &ec) = 0;

    std::string uri() const
    { return m_uri; }

    std::string hostname() const
    { return m_hostname; }

protected:
    std::string     m_uri;
    std::string     m_hostname;
};

struct ServerConnection : public Connection
{
    ServerConnection(ConnectionType type, int port, bool version4, std::error_code &ec)
        : Connection(type, port, ec),
         m_version4{version4}
    {}

    virtual ~ServerConnection() = default;
    virtual void bind(std::error_code &ec) = 0;
    virtual void listen(std::error_code &ec, int max_connections_in_queue = 1) = 0;
    virtual Socket * accept(std::error_code * ec = nullptr) = 0;

    bool version4() const
    { return m_version4; }

protected:
    bool            m_version4;
};


Socket * create_socket(ConnectionType type, DnsResolver *dns, std::error_code &ec);
ClientConnection * create_client_connection(ConnectionType type, const char * hostname, int port, std::error_code &ec);
ClientConnection * create_client_connection(const char * uri, std::error_code &ec);

#endif
