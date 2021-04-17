#ifndef _ERROR_H
#define _ERROR_H
#include <system_error>

enum class CoapStatus
{
    COAP_OK = 0,
    COAP_ERR_OPTION_NUMBER,
    COAP_ERR_PROTOCOL_VERSION,
    COAP_ERR_TOKEN_LENGTH,
    COAP_ERR_OPTION_DELTA,
    COAP_ERR_OPTION_LENGTH,
    COAP_ERR_URI_PATH,
    COAP_ERR_BUFFER_SIZE,
    COAP_ERR_CREATE_SOCKET,
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
};

namespace std
{
template<> struct is_error_condition_enum<CoapStatus> : public true_type {};
}

std::error_code make_error_code (CoapStatus e);
std::error_code make_system_error (int e);

#endif
