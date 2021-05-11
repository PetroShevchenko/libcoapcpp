#include "unix_dns_resolver.h"
#include "error.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstdint>
#include <cstring>

using namespace std;
using namespace spdlog;

static const char * testUri = "coap://localhost:5683";
static const char * testUriIPv6 = "coap://[fe80::c802:4428:bcbb:363d]:5673";
static const char * testUriIPv4 = "coaps://192.168.0.1:5684";

TEST(testDnsResolver, resolveHostname)
{
    std::error_code ec;

    UnixDnsResolver resolver(testUri);

    resolver.hostname2address(ec);

    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    info("uri : {}", resolver.uri());
    info("address4 : {}", resolver.address4());
    info("port : {0:d}", resolver.port());
#endif
}

TEST(testDnsResolver, resolveIPv4)
{
    std::error_code ec;

    UnixDnsResolver resolver(testUriIPv4);

    resolver.hostname2address(ec);

    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    info("uri : {}", resolver.uri());
    info("address4 : {}", resolver.address4());
    info("port : {0:d}", resolver.port());
#endif
}

TEST(testDnsResolver, resolveIPv6)
{
    std::error_code ec;

    UnixDnsResolver resolver(testUriIPv6);

    resolver.hostname2address(ec);

    ASSERT_TRUE(!ec.value());

#ifdef PRINT_TESTED_VALUES
    info("uri : {}", resolver.uri());
    info("address6 : {}", resolver.address6());
    info("port : {0:d}", resolver.port());
#endif
}
