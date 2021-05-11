#ifndef _UNIX_DNS_RESOLVER
#define _UNIX_DNS_RESOLVER
#include "dns_resolver.h"
#include "socket.h"
#include <string>

class UnixDnsResolver : public DnsResolver
{
public:
    UnixDnsResolver()
    : DnsResolver()
    {}

    UnixDnsResolver(const char * uri)
    : DnsResolver(uri)
    {}

    UnixDnsResolver(const char * hostname, int port)
    : DnsResolver(hostname, port)
    {}

    ~UnixDnsResolver() = default;

public:
    void hostname2address(std::error_code &ec) override;
    SocketAddress * create_socket_address(std::error_code &ec) override;
};

#endif
