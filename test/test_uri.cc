#include "uri.h"
#include "test_conf.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstdint>
#include <cstring>
#include <string>

using namespace std;
using namespace coap;
using namespace spdlog;

static void print_UriPath(UriPath &uriPath)
{
#ifdef PRINT_TESTED_VALUES
    info("m_path :{}", uriPath.path());
    info("m_uri.type() :{0:d}", uriPath.uri().type());
    info("m_uri.asString() :");
    fmt::print("{:}", fmt::join(uriPath.uri().asString(), ", "));
    fmt::print("\n");
    info("m_uri.asInteger() :");
    fmt::print("{:d}", fmt::join(uriPath.uri().asInteger(), ", "));
    fmt::print("\n");
#endif
}

TEST(testUri, uriPath)
{
    error_code ec;
{
    UriPath uriPath("/0/1/2/3", ec);
    ASSERT_TRUE(ec.value() == 0);
    print_UriPath(uriPath);
}

{
    UriPath uriPath("/first/second/third/forth", ec);
    ASSERT_TRUE(ec.value() == 0);
    print_UriPath(uriPath);
}

{
    char longStr[URI_PATH_MAX_LENGTH - 1];
    memset(longStr,'a', sizeof(longStr));

    UriPath uriPath(longStr, ec);

    ASSERT_TRUE(ec.value() == 0);
    print_UriPath(uriPath);
}

{
    char longStr[URI_PATH_MAX_LENGTH];
    memset(longStr,'a', sizeof(longStr));

    UriPath uriPath(longStr, ec);

    ASSERT_FALSE(ec.value() == 0);
}

{
    Uri uri;
    uri.type(URI_TYPE_INTEGER);
    uri.asInteger().push_back(10);
    uri.asInteger().push_back(20);
    uri.asInteger().push_back(30);
    uri.asInteger().push_back(40);

    UriPath uriPath(std::move(uri));
    print_UriPath(uriPath);

    ASSERT_EQ(uriPath.uri().type(), URI_TYPE_INTEGER);
    ASSERT_EQ(uriPath.uri().asInteger()[0], 10);
    ASSERT_EQ(uriPath.uri().asInteger()[1], 20);
    ASSERT_EQ(uriPath.uri().asInteger()[2], 30);
    ASSERT_EQ(uriPath.uri().asInteger()[3], 40);
}

{
    Uri uri;
    uri.type(URI_TYPE_STRING);
    uri.asString().push_back("path");
    uri.asString().push_back("to");
    uri.asString().push_back("required");
    uri.asString().push_back("resource");

    UriPath uriPath(std::move(uri));
    print_UriPath(uriPath);

    ASSERT_EQ(uriPath.uri().type(), URI_TYPE_STRING);

    ASSERT_TRUE(uriPath.uri().asString()[0] == "path");
    ASSERT_TRUE(uriPath.uri().asString()[1] == "to");
    ASSERT_TRUE(uriPath.uri().asString()[2] == "required");
    ASSERT_TRUE(uriPath.uri().asString()[3] == "resource");
}

}
