#ifndef _LWIP_DNS_RESOLVER
#define _LWIP_DNS_RESOLVER
#include "dns_resolver.h"
#include "socket.h"
#include <string>

class LwipDnsResolver : public DnsResolver
{
public:
    LwipDnsResolver()
    : DnsResolver()
    {}

    LwipDnsResolver(const char * uri)
    : DnsResolver(uri)
    {}

    LwipDnsResolver(const char * hostname, int port)
    : DnsResolver(hostname, port)
    {}

    ~LwipDnsResolver() = default;

public:
    void hostname2address(std::error_code &ec) override;
    SocketAddress * create_socket_address(std::error_code &ec) override;
};

#endif
