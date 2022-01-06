#ifndef _DNS_RESOLVER_H
#define _DNS_RESOLVER_H
#include <string>
#include "socket.h"
#include "error.h"

class DnsResolver
{
public:
    DnsResolver()
    : m_uri{}, m_address4{}, m_address6{}, m_port{-1}
    {}

    DnsResolver(const char * uri)
    : m_uri{uri}, m_address4{}, m_address6{}, m_port{-1}
    {}

    DnsResolver(const char * hostname, int port)
    : m_uri{hostname}, m_address4{}, m_address6{}, m_port{port}
    {}

    virtual ~DnsResolver() = default;

public:
    virtual void hostname2address(std::error_code &ec) = 0;
    virtual SocketAddress * create_socket_address(std::error_code &ec) = 0;

public:
    std::string & uri()
    { return m_uri; }

    std::string & address4()
    { return m_address4; }

    std::string & address6()
    { return m_address6; }

    int & port()
    { return m_port; }

protected:
    std::string m_uri;
    std::string m_address4;
    std::string m_address6;
    int m_port;
};

#endif
