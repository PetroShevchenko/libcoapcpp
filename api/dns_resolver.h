#ifndef _DNS_RESOLVER_H
#define _DNS_RESOLVER_H
#include <string>

class DnsResolver
{
public:
    DnsResolver()
    : m_port{-1}
    {}

    DnsResolver(const char * uri)
    : m_uri{uri}, m_port{-1}
    {}

    DnsResolver(const char * hostname, int port)
    : m_uri{hostname}, m_port{port}
    {}

    virtual ~DnsResolver() = default;

public:
    virtual bool hostname2address() = 0;

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
