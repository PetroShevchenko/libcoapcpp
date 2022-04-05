#include "uri.h"
#include <cstring>
#include <cstdlib>
//#include <spdlog/spdlog.h>
//#include <spdlog/fmt/fmt.h>

using namespace std;
//using namespace spdlog;

namespace coap
{

UriPath::UriPath(const char * path, error_code &ec)
    : m_path{path}, m_uri{}
{
    if (!path_to_uri())
    {
        ec = make_error_code(CoapStatus::COAP_ERR_URI_PATH);
        return;
    }
}

UriPath::UriPath(Uri &&uri)
    : m_path{}, m_uri{std::move(uri)}
{
    uri_to_path();
}

UriPath::UriPath(UriPath &&other)
{
    if (this != &other)
    {
        m_path = std::move(other.m_path);
        m_uri = std::move(other.m_uri);
    }
}

UriPath &UriPath::operator=(UriPath &&other)
{
    if(this != &other)
    {
        m_path = std::move(other.m_path);
        m_uri = std::move(other.m_uri);
    }
    return *this;
}

bool UriPath::path_to_uri()
{
    char buffer[URI_PATH_MAX_LENGTH];
    char * token = buffer;
    //set_level(level::debug);

    if (m_path.size() >= sizeof(buffer))
    {
        return false;
    }

    strncpy(token, m_path.c_str(), m_path.size() + 1);

    token = strtok(token, "/");

    if (token == NULL)
    {
        return false;
    }

    if (((token[0] & 0xF0)^0x30) == 0)
    {
        m_uri.type(URI_TYPE_INTEGER);
    }
    else
    {
        m_uri.type(URI_TYPE_STRING);
    }

    do
    {
        if(m_uri.type() == URI_TYPE_STRING)
        {
            m_uri.asString().push_back(token);
        }
        else if (m_uri.type() == URI_TYPE_INTEGER)
        {
            m_uri.asInteger().push_back(strtol (token, NULL, 10));
        }
        token = strtok(NULL, "/");
    } while (token != NULL);

    return true;
}

void UriPath::uri_to_path()
{
    if (m_uri.type() == URI_TYPE_STRING)
    {
        for (size_t i = 0; i < m_uri.asString().size(); i++)
        {
            m_path += "/";
            m_path += m_uri.asString()[i];
        }
    }
    else if (m_uri.type() == URI_TYPE_INTEGER)
    {
        for (size_t i =0; i < m_uri.asInteger().size(); i++)
        {
            m_path += "/";
            m_path += std::to_string(m_uri.asInteger()[i]);
        }
    }
}

void Uri::clear()
{
    m_type = URI_TYPE_INTEGER;
    m_asString.clear();
    m_asInteger.clear();
}

void UriPath::clear()
{
    m_path.clear();
    m_uri.clear();
}

}
