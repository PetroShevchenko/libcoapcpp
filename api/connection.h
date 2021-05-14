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

struct ClientConnection
{
    ClientConnection(ConnectionType type, const char * hostname, int port, std::error_code &ec)
        : m_type{type},
         m_uri{},
         m_hostname{hostname},
         m_port{port}
    {
        if (!is_connection_type(m_type))
        { ec = make_system_error(EINVAL); }
    }

    ClientConnection(const char * uri, std::error_code &ec)
        : m_type{TCP},
         m_uri{uri},
         m_hostname{},
         m_port{-1}
    {
        if (!uri2connection_type(uri, m_type))
        { ec = make_system_error(EINVAL); }
    }

    virtual ~ClientConnection() = default;
    virtual void connect(std::error_code &ec) = 0;
    virtual void disconnect(std::error_code &ec) = 0;
    virtual void send(const void * buffer, size_t length, std::error_code &ec) = 0;
    virtual void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) = 0;

    ConnectionType type() const
    { return m_type; }

    std::string uri() const
    { return m_uri; }

    std::string hostname() const
    { return m_hostname; }

    int port() const
    { return m_port; }

protected:
    ConnectionType  m_type;
    std::string     m_uri;
    std::string     m_hostname;
    int             m_port;
};

ClientConnection * create_client_connection(ConnectionType type, const char * hostname, int port, std::error_code &ec);
ClientConnection * create_client_connection(const char * uri, std::error_code &ec);

#endif
