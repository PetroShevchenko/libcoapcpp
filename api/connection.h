#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "dns_resolver.h"
#include "socket.h"
#include "error.h"

enum ConnectionType
{
    TCP,
    UDP,
    TLS,
    DTLS
};

inline bool is_connection_type(ConnectionType type)
{ return !(type < TCP || type > DTLS); }

struct Connection
{
    virtual ~Connection() = default;
    virtual void connect(std::error_code &ec) = 0;
    virtual void disconnect(std::error_code &ec) = 0;
    virtual void send(const void * buffer, size_t length, std::error_code &ec) = 0;
    virtual void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) = 0;
    virtual ConnectionType type() const = 0;
};

Connection * create_client_connection(ConnectionType type, const char * hostname, int port, std::error_code &ec);
Connection * create_client_connection(const char * uri, std::error_code &ec);

#endif

