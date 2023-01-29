#include "utils.h"
#include <cstring>
#include <string>
#include <cstdlib>

using namespace coap;

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

const char *coap_message_type_to_string(MessageType type)
{
    switch (type)
    {
    case CONFIRMABLE:
        return "CONFIRMABLE";
    case NON_CONFIRMABLE:
        return "NON_CONFIRMABLE";
    case ACKNOWLEDGEMENT:
        return "ACKNOWLEDGEMENT";
    case RESET:
        return "RESET";
    default:
        return "UNKNOWN";
    }
}

const char *coap_message_code_to_string(MessageCode code)
{
    switch (code)
    {
    case EMPTY:
        return "EMPTY";
    case GET:
        return "GET";
    case POST:
        return "POST";
    case PUT:
        return "PUT";
    case DELETE:
        return "DELETE";
    case FETCH:
        return "FETCH";
    case PATCH:
        return "PATCH";
    case IPATCH:
        return "IPATCH";
    case CREATED:
        return "CREATED";
    case DELETED:
        return "DELETED";
    case VALID:
        return "VALID";
    case CHANGED:
        return "CHANGED";
    case CONTENT:
        return "CONTENT";
    case CONTINUE:
        return "CONTINUE";
    case BAD_REQUEST:
        return "BAD_REQUEST";
    case UNAUTHORIZED:
        return "UNAUTHORIZED";
    case BAD_OPTION:
        return "BAD_OPTION";
    case FORBIDDED:
        return "FORBIDDED";
    case NOT_FOUND:
        return "NOT_FOUND";
    case METHOD_NOT_ALLOWED:
        return "METHOD_NOT_ALLOWED";
    case NOT_ACCEPTABLE:
        return "NOT_ACCEPTABLE";
    case REQUEST_ENTITY_INCOMPLETE:
        return "REQUEST_ENTITY_INCOMPLETE";
    case PRECONDITION_FAILED:
        return "PRECONDITION_FAILED";
    case REQUEST_ENTITY_TOO_LARGE:
        return "REQUEST_ENTITY_TOO_LARGE";
    case UNSUPPORTED_CONTENT_FORMAT:
        return "UNSUPPORTED_CONTENT_FORMAT";
    case INTERNAL_SERVER_ERROR:
        return "INTERNAL_SERVER_ERROR";
    case NOT_IMPLEMENTED:
        return "NOT_IMPLEMENTED";
    case BAD_GATEWAY:
        return "BAD_GATEWAY";
    case SERVICE_UNAVAILABLE:
        return "SERVICE_UNAVAILABLE";
    case GATEWAY_TIMEOUT:
        return "GATEWAY_TIMEOUT";
    case PROXYING_NOT_SUPPORTED:
        return "PROXYING_NOT_SUPPORTED";
    case UNASSIGNED:
        return "UNASSIGNED";
    case RELEASE:
        return "RELEASE";
    case ABORT:
        return "ABORT";
    default:
        return "UNKNOWN";
    }
}

const char *coap_option_number_to_string(OptionNumber number)
{
    switch (number)
    {
    case IF_MATCH:
        return "IF_MATCH";
    case URI_HOST:
        return "URI_HOST";
    case ETAG:
        return "ETAG";
    case IF_NONE_MATCH:
        return "IF_NONE_MATCH";
    case URI_PORT:
        return "URI_PORT";
    case LOCATION_PATH:
        return "LOCATION_PATH";
    case URI_PATH:
        return "URI_PATH";
    case CONTENT_FORMAT:
        return "CONTENT_FORMAT";
    case MAX_AGE:
        return "MAX_AGE";
    case URI_QUERY:
        return "URI_QUERY";
    case ACCEPT:
        return "ACCEPT";
    case LOCATION_QUERY:
        return "LOCATION_QUERY";
    case BLOCK_2:
        return "BLOCK_2";
    case BLOCK_1:
        return "BLOCK_1";
    case SIZE_2:
        return "SIZE_2";
    case PROXY_URI:
        return "PROXY_URI";
    case PROXY_SCHEME:
        return "PROXY_SCHEME";
    case SIZE_1:
        return "SIZE_1";
    default:
        return "UNKNOWN";
    }
}

const char *coap_media_type_to_string(MediaType type)
{
    switch (type)
    {
    case TEXT_PLAIN:
        return "TEXT_PLAIN";
    case LINK_FORMAT:
        return "LINK_FORMAT";
    case XML:
        return "XML";
    case OCTET_STREAM:
        return "OCTET_STREAM";
    case EXI:
        return "EXI";
    case COAP_JSON:
        return "COAP_JSON";
    case SENML_CBOR:
        return "SENML_CBOR";
    case LWM2M_TLV:
        return "LWM2M_TLV";
    default:
        return "UNKNOWN";
    }
}
