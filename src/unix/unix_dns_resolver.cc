#include "unix_dns_resolver.h"
#include "utils.h"
#include "spdlog/spdlog.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

using namespace spdlog;

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

bool UnixDnsResolver::hostname2address()
{
    struct addrinfo hints;
    struct addrinfo * servinfo = nullptr;
    char port_str[8];
    char address_str[40];
    int result;
    bool status = false;
    std::string hostname;

    set_level(level::debug);

    if (uri().size() == 0)
        return false;

    if (!uri2hostname(uri().c_str(), hostname, port(), true)
        && !uri2hostname(uri().c_str(), hostname, port(), false))
    {
        hostname = uri();
    }

    if (is_address6(hostname.c_str()))
    {
        address6() = hostname.c_str();
        return true;
    }

    if (is_address4(hostname.c_str()))
    {
        address4() = hostname.c_str();
          return true;
    }
    if (port() == -1)
        return false;

    memset(address_str, 0, sizeof(address_str));
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    snprintf((char *)port_str, sizeof(port_str), "%d", port());

    result = getaddrinfo(hostname.c_str(), port_str, &hints, &servinfo);

    if (result == 0)
    {
        for (struct addrinfo * p = servinfo; p != nullptr; p = p->ai_next)
        {
            inet_ntop(p->ai_family, p->ai_family == AF_INET ?
                    (void *) & ((struct sockaddr_in *) p->ai_addr)->sin_addr :
                    (void *) & ((struct sockaddr_in6 *) p->ai_addr)->sin6_addr, address_str, sizeof(address_str));
            if (p->ai_family == AF_INET6) {
                address6() = address_str;
                status = true;
            }
            else if (p->ai_family == AF_INET) {
                address4() = address_str;
                status = true;
            }
        }
    }
    else status = false;

    if (servinfo != nullptr)
    {
        freeaddrinfo(servinfo);
    }

    return status;
}
