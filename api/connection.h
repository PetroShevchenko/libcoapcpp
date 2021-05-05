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
    virtual void send(std::error_code &ec) = 0;
    virtual void receive(std::error_code &ec) = 0;
};

#endif

