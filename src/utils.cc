#include "utils.h"
#include <cstring>
#include <string>
#include <cstdlib>

bool uri2connection_type(const char * uri, ConnectionType &type)
{
    if (uri == NULL)
        return false;

    int length = strlen(uri);
    if (length <= 0)
        return false;

    const char * titles[] = {
            "coap+tcp://",
            "coap://",
            "coaps+tcp://",
            "coaps://"
    };

    const size_t tsize = sizeof(titles)/sizeof(titles[0]);
    for (size_t i = 0; i != tsize; i++)
    {
        const char * token = strstr(uri, titles[i]);
        if (token != NULL)
        {
            type = static_cast<ConnectionType>(i);
            return true;
        }
    }
    return false;
}

bool uri2hostname(const char * uri, std::string &hostname, int &port, bool uri4)
{
    if (uri == NULL)
        return false;

    int length = strlen(uri);
    if (length <= 0)
        return false;

    const char * begin4 = "://";
    const char * begin6 = "[";
    const char * end4 = ":";
    const char * end6 = "]:";

    const char * begin = strstr(uri, uri4 ? begin4 : begin6);

    if (begin == NULL || (uri4 && begin[strlen(begin4)] == begin6[0]))
        return false;

    hostname = &begin[uri4 ? strlen(begin4) : strlen(begin6)];

    size_t position = 0;
    position = hostname.find(uri4 ? end4 : end6);

    if (position > 0)
    {
        std::size_t offset = (uri4 ? strlen(end4) : strlen(end6)) + position;
        port = atoi(hostname.substr(offset).c_str());
        hostname.resize(position);
        return true;
    }
    return false;
}
