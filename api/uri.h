#ifndef _URI_H
#define _URI_H
#include <cstdint>
#include <string>
#include <vector>
#include "error.h"

namespace coap
{

const std::uint8_t URI_MAX_SEGMENTS = 4; // for example: 4 segments equals '/first/second/third/forth' or '/0/1/2/3'
const std::size_t URI_PATH_MAX_LENGTH = 256;

enum UriType
{
    URI_TYPE_INTEGER,
    URI_TYPE_STRING
};

struct Uri
{
    Uri()
    : m_type{URI_TYPE_INTEGER}, m_asString{}, m_asInteger{}
    {}
    virtual ~Uri()
    {}

    Uri &operator=(Uri &&other)
    {
        if(this != &other)
        {
            std::swap(m_type, other.m_type);
            m_asString = std::move(other.m_asString);
            m_asInteger = std::move(other.m_asInteger);
        }
        return *this;
    }

    Uri(Uri &&other)
    : m_type(other.m_type),
      m_asString(std::move(other.m_asString)),
      m_asInteger(std::move(other.m_asInteger))
    { other.m_type = URI_TYPE_INTEGER; }

    Uri &operator=(const Uri &other)
    {
        if(this != &other)
        {
            m_type = other.m_type;
            m_asString = other.m_asString;
            m_asInteger = other.m_asInteger;
        }
        return *this;
    }

    Uri(const Uri &other)
    { operator=(other); }

    UriType type() const
    { return m_type; }

    void type(UriType type)
    { m_type = type; }

    std::vector<std::string> & asString()
    { return m_asString; }

    const std::vector<std::string> & asString() const
    { return static_cast<const std::vector<std::string> &>(m_asString); }

    std::vector<long int> & asInteger()
    { return m_asInteger; }

    void clear();

private:
    UriType                     m_type;
    std::vector<std::string>    m_asString;
    std::vector<long int>       m_asInteger;
};

class UriPath
{
public:
    UriPath()
    : m_path{}, m_uri{}
    {}
    UriPath(const char *, std::error_code &);
    UriPath(Uri &&);
    UriPath(UriPath &&);
    UriPath(const UriPath &);
    UriPath &operator=(const UriPath &other);
    UriPath &operator=(UriPath &&other);

    virtual ~UriPath()
    {}

public:
    const std::string &path() const
    {
        return m_path;
    }

    void path(const char *path)
    {
        m_path = path;
        path_to_uri();
    }

    Uri & uri()
    { return m_uri; }

    const Uri & uri() const
    { return static_cast<const Uri&>(m_uri); }

    void clear();

public:
    bool path_to_uri();
    void uri_to_path();

private:
    std::string m_path;
    Uri         m_uri;
};

}//coap


#endif
