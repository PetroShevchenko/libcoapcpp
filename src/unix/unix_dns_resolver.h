#ifndef _UNIX_DNS_RESOLVER
#define _UNIX_DNS_RESOLVER
#include "dns_resolver.h"
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
    bool hostname2address() override;
};

#endif
