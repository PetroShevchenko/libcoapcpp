#ifndef _UTILS_H
#define _UTILS_H
#include <string>
#include "connection.h"

bool uri2connection_type(const char * uri, ConnectionType &type);
bool uri2hostname(const char * uri, std::string &hostname, int &port, bool uri4);

#endif
