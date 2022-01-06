#include "lwip/init.h"
#if (LWIP_VERSION_MAJOR >= 2) && (LWIP_VERSION_MAJOR >= 1)
#include "lwip_dns_resolver.h"
#include "utils.h"
#include "lwip_socket.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

static bool is_address4(const char * hostname)
{
    struct sockaddr_in addr;
    return (bool)inet_pton(AF_INET, hostname, &addr);
}

static bool is_address6(const char * hostname)
{
    struct sockaddr_in6 addr;
    return (bool)inet_pton(AF_INET6, hostname, &addr);
}

void LwipDnsResolver::hostname2address(std::error_code &ec)
{
    struct addrinfo hints;
    struct addrinfo * servinfo = nullptr;
    char port_str[8];
    char address_str[40];
    int result;
    std::string hostname;

    ec.clear();

    if (uri().size() == 0)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_EMPTY_HOSTNAME);
        return;
    }

    if (!uri2hostname(uri().c_str(), hostname, port(), true)
        && !uri2hostname(uri().c_str(), hostname, port(), false))
    {
        hostname = uri();
    }

    if (is_address6(hostname.c_str()))
    {
        address6() = hostname.c_str();
        return;
    }

    if (is_address4(hostname.c_str()))
    {
        address4() = hostname.c_str();
        return;
    }

    if (port() == -1)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_PORT_NUMBER);
        return;
    }

    memset(address_str, 0, sizeof(address_str));
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    snprintf((char *)port_str, sizeof(port_str), "%d", port());

    result = lwip_getaddrinfo(hostname.c_str(), port_str, &hints, &servinfo);

    if (result == 0)
    {
        for (struct addrinfo * p = servinfo; p != nullptr; p = p->ai_next)
        {
            inet_ntop(p->ai_family, p->ai_family == AF_INET ?
                    (void *) & ((struct sockaddr_in *) p->ai_addr)->sin_addr :
                    (void *) & ((struct sockaddr_in6 *) p->ai_addr)->sin6_addr, address_str, sizeof(address_str));
            if (p->ai_family == AF_INET6) {
                address6() = address_str;
            }
            else if (p->ai_family == AF_INET) {
                address4() = address_str;
            }
        }
    }
    else
    {
        ec = make_error_code(CoapStatus::COAP_ERR_RESOLVE_ADDRESS);
    }

    if (servinfo != nullptr)
    {
        lwip_freeaddrinfo(servinfo);
    }
}

SocketAddress * LwipDnsResolver::create_socket_address(std::error_code &ec)
{
    struct in_addr * inp;
    SocketAddress *sap = nullptr;

    if(m_port == -1)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_PORT_NUMBER);
    return sap;
    }

    if (m_address6.size())
    {
        struct sockaddr_in6 sa;
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(m_port);
        inp = reinterpret_cast<in_addr *>(&sa.sin6_addr.s6_addr);
        inet_aton(m_address6.c_str(), inp);
        sap = new LwipSocketAddress(sa);
    }
    else if (m_address4.size())
    {
        struct sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(m_port);
        inp = reinterpret_cast<in_addr *>(&sa.sin_addr.s_addr);
        inet_aton(m_address4.c_str(), inp);
        sap = new LwipSocketAddress(sa);
    }
    else
    {
        ec = make_error_code(CoapStatus::COAP_ERR_EMPTY_ADDRESS);
    return sap;
    }

    if (sap == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
    }
    return sap;
}

#endif
