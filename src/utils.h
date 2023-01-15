#ifndef _UTILS_H
#define _UTILS_H
#include <string>

enum ConnectionType
{
    TCP,
    UDP,
    TLS,
    DTLS
};

inline bool is_connection_type(ConnectionType type)
{ return !(type < TCP || type > DTLS); }

bool uri2connection_type(const char * uri, ConnectionType &type);
bool uri2hostname(const char * uri, std::string &hostname, int &port, bool uri4);

#endif
