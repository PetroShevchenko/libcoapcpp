#ifndef _ERROR_H
#define _ERROR_H
#include <system_error>

enum CoapStatus
{
    COAP_OK = 0,
    COAP_ERR_OPTION_NUMBER,
    COAP_ERR_PROTOCOL_VERSION,
    COAP_ERR_TOKEN_LENGTH,
    COAP_ERR_OPTION_DELTA,
    COAP_ERR_OPTION_LENGTH,
    COAP_ERR_OPTION_VALUE,
    COAP_ERR_URI_PATH,
    COAP_ERR_BUFFER_SIZE,
    COAP_ERR_CREATE_SOCKET,
    COAP_ERR_SOCKET_NOT_BOUND,
    COAP_ERR_INCOMPLETE_SEND,
    COAP_ERR_RECEIVE,
    COAP_ERR_RESOLVE_ADDRESS,
    COAP_ERR_PORT_NUMBER,
    COAP_ERR_CREATE_BLOCK_OPTION,
    COAP_ERR_COAP_SERIALIZE,
    COAP_ERR_SEND,
    COAP_ERR_TIMEOUT,
    COAP_ERR_PACKET_RECEIVE,
    COAP_ERR_RECEIVED_PACKET,
    COAP_ERR_URI_NOT_FOUND,
    COAP_ERR_SERVER_CODE,
    COAP_ERR_DECODE_BLOCK_OPTION,
    COAP_ERR_ENCODE_BLOCK_OPTION,
    COAP_ERR_SOCKET_DOMAIN,
    COAP_ERR_MEMORY_ALLOCATE,
    COAP_ERR_NOT_IMPLEMENTED,
    COAP_ERR_NOT_CONNECTED,
    COAP_ERR_EMPTY_HOSTNAME,
    COAP_ERR_EMPTY_ADDRESS,
    COAP_ERR_REMOVE_CONNECTION,
    COAP_ERR_NO_PAYLOAD,
    COAP_ERR_CONNECTIONS_EXCEEDED,
    COAP_ERR_DTLS_CTX_INIT,
    COAP_ERR_CREATE_JSON,
    COAP_ERR_PARSE_JSON,
    COAP_ERR_NO_JSON_FIELD,
    COAP_ERR_CREATE_CORE_LINK,
    COAP_ERR_PARSE_CORE_LINK,
    COAP_ERR_BAD_REQUEST,
    COAP_ERR_NOT_FOUND,
};

namespace std
{
template<> struct is_error_condition_enum<CoapStatus> : public true_type {};
}

std::error_code make_error_code (CoapStatus e);
std::error_code make_system_error (int e);

#endif
