#ifndef _UTILS_H
#define _UTILS_H
#include <string>
#include "consts.h"

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
const char *coap_message_type_to_string(coap::MessageType type);
const char *coap_message_code_to_string(coap::MessageCode code);
const char *coap_option_number_to_string(coap::OptionNumber number);
const char *coap_media_type_to_string(coap::MediaType type);

#endif
