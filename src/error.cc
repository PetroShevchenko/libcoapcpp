#include "error.h"
#include <string>

namespace
{

struct CoapErrorCategory : public std::error_category
{
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* CoapErrorCategory::name() const noexcept
{ return "libcoapcpp"; }

std::string CoapErrorCategory::message(int ev) const
{
    switch((CoapStatus)ev)
    {
        case CoapStatus::COAP_OK:
            return "Success";

        case CoapStatus::COAP_ERR_OPTION_NUMBER:
            return "Unsopported option number";

        case CoapStatus::COAP_ERR_PROTOCOL_VERSION:
            return "Unsupported CoAP version";

        case CoapStatus::COAP_ERR_TOKEN_LENGTH:
            return "Too long the token length";

        case CoapStatus::COAP_ERR_OPTION_DELTA:
            return "Wrong the option parameter delta";

        case CoapStatus::COAP_ERR_OPTION_LENGTH:
            return "Too long the option length";

        case CoapStatus::COAP_ERR_OPTION_VALUE:
            return "Wrong option value";

        case CoapStatus::COAP_ERR_URI_PATH:
            return "Wrong URI path use something like /0/1 or /first/second";

        case CoapStatus::COAP_ERR_BUFFER_SIZE:
            return "Too small the buffer size";

        case CoapStatus::COAP_ERR_CREATE_SOCKET:
            return "Can not create socket";

        case CoapStatus::COAP_ERR_SOCKET_NOT_BOUND:
            return "The socket is not bound";

        case CoapStatus::COAP_ERR_INCOMPLETE_SEND:
            return "The buffer was incompletely sent";

        case CoapStatus::COAP_ERR_RECEIVE:
            return "Receive error";

        case CoapStatus::COAP_ERR_RESOLVE_ADDRESS:
            return "Unable to resolve IP address";

        case CoapStatus::COAP_ERR_PORT_NUMBER:
            return "Wrong port number";

        case CoapStatus::COAP_ERR_CREATE_BLOCK_OPTION:
            return "Unable to create block-wise option";

        case CoapStatus::COAP_ERR_COAP_SERIALIZE:
            return "Unable to serialize the COAP packet";

        case CoapStatus::COAP_ERR_SEND:
            return "Unable to send the COAP packet";

        case CoapStatus::COAP_ERR_TIMEOUT:
            return "The COAP server does not response";

        case CoapStatus::COAP_ERR_PACKET_RECEIVE:
            return "Unable to receive the COAP packet";

        case CoapStatus::COAP_ERR_RECEIVED_PACKET:
            return "Unable to deserialize the received packet";

        case CoapStatus::COAP_ERR_URI_NOT_FOUND:
            return "URI is not found";

        case CoapStatus::COAP_ERR_SERVER_CODE:
            return "The COAP server sent an error code";

        case CoapStatus::COAP_ERR_DECODE_BLOCK_OPTION:
            return "Unable to decode block-wise option";

        case CoapStatus::COAP_ERR_ENCODE_BLOCK_OPTION:
            return "Unable to encode block-wise option";

        case CoapStatus::COAP_ERR_SOCKET_DOMAIN:
            return "Unsupported socket domain";

        case CoapStatus::COAP_ERR_MEMORY_ALLOCATE:
            return "Unable to allocate memory";

        case CoapStatus::COAP_ERR_NOT_IMPLEMENTED:
            return "This feature is not implemented";

        case CoapStatus::COAP_ERR_NOT_CONNECTED:
            return "The connection is not established";

        case CoapStatus::COAP_ERR_EMPTY_HOSTNAME:
            return "Hostname or URI is not presented";

        case CoapStatus::COAP_ERR_EMPTY_ADDRESS:
            return "IPv4 or IPv6 adreess is not presented";

        case CoapStatus::COAP_ERR_REMOVE_CONNECTION:
            return "Unable to remove connection";

        case CoapStatus::COAP_ERR_NO_PAYLOAD:
            return "There is no payload to handle";

        case CoapStatus::COAP_ERR_CONNECTIONS_EXCEEDED:
            return "The maximum number of connections has been exceeded";

        case CoapStatus::COAP_ERR_DTLS_CTX_INIT:
            return "Failed to initialize DTLS context";

        case CoapStatus::COAP_ERR_CREATE_JSON:
            return "Failed to create JSON content";

        case CoapStatus::COAP_ERR_PARSE_JSON:
            return "Failed to parse JSON content";

        case CoapStatus::COAP_ERR_NO_JSON_FIELD:
            return "Failed to find JSON field";

        case CoapStatus::COAP_ERR_CREATE_CORE_LINK:
            return "Failed to create CoRe-Link content";

        case CoapStatus::COAP_ERR_PARSE_CORE_LINK:
            return "Failed to parse CoRe-Link content";

        case CoapStatus::COAP_ERR_NO_ENDPOINT:
            return "There is no andpoint corresponding to CoRe-Link";

        case CoapStatus::COAP_ERR_ENDPOINT_ANSWER:
            return "Failed to get answer from endpoint";

        case CoapStatus::COAP_ERR_RECORD_FORMAT:
            return "Wrong record format";

        case CoapStatus::COAP_ERR_BLOCK_SIZE:
            return "Wrong Block Size";

        case CoapStatus::COAP_ERR_LARGE_PAYLOAD:
            return "The payload size is too large";

        case CoapStatus::COAP_ERR_BAD_REQUEST:
            return "4.00 Bad Request";

        case CoapStatus::COAP_ERR_NOT_FOUND:
            return "4.04 Not Found";

        case CoapStatus::COAP_ERR_METHOD_NOT_ALLOWED:
            return "4.05 Method Not Allowed";
    }
    return "Unknown error";
}

const CoapErrorCategory theCoapErrorCategory {};

} // namespace

std::error_code make_error_code (CoapStatus e)
{
    return {static_cast<int>(e), theCoapErrorCategory};
}

std::error_code make_system_error (int e)
{
    return {e, std::generic_category()};
}

const char* error_code_to_str(CoapStatus e)
{
    switch(e)
    {
        case CoapStatus::COAP_OK:
            return "COAP_OK";

        case CoapStatus::COAP_ERR_OPTION_NUMBER:
            return "COAP_ERR_OPTION_NUMBER";

        case CoapStatus::COAP_ERR_PROTOCOL_VERSION:
            return "COAP_ERR_PROTOCOL_VERSION";

        case CoapStatus::COAP_ERR_TOKEN_LENGTH:
            return "COAP_ERR_TOKEN_LENGTH";

        case CoapStatus::COAP_ERR_OPTION_DELTA:
            return "COAP_ERR_OPTION_DELTA";

        case CoapStatus::COAP_ERR_OPTION_LENGTH:
            return "COAP_ERR_OPTION_LENGTH";

        case CoapStatus::COAP_ERR_URI_PATH:
            return "COAP_ERR_URI_PATHd";

        case CoapStatus::COAP_ERR_BUFFER_SIZE:
            return "COAP_ERR_BUFFER_SIZE";

        case CoapStatus::COAP_ERR_CREATE_SOCKET:
            return "COAP_ERR_CREATE_SOCKET";

        case CoapStatus::COAP_ERR_SOCKET_NOT_BOUND:
            return "COAP_ERR_SOCKET_NOT_BOUND";

        case CoapStatus::COAP_ERR_INCOMPLETE_SEND:
            return "COAP_ERR_INCOMPLETE_SEN";

        case CoapStatus::COAP_ERR_RECEIVE:
            return "COAP_ERR_RECEIVE";

        case CoapStatus::COAP_ERR_RESOLVE_ADDRESS:
            return "COAP_ERR_RESOLVE_ADDRESS";

        case CoapStatus::COAP_ERR_PORT_NUMBER:
            return "COAP_ERR_PORT_NUMBER";

        case CoapStatus::COAP_ERR_CREATE_BLOCK_OPTION:
            return "COAP_ERR_CREATE_BLOCK_OPTION";

        case CoapStatus::COAP_ERR_COAP_SERIALIZE:
            return "COAP_ERR_COAP_SERIALIZE";

        case CoapStatus::COAP_ERR_SEND:
            return "COAP_ERR_SEND";

        case CoapStatus::COAP_ERR_TIMEOUT:
            return "COAP_ERR_TIMEOUT";

        case CoapStatus::COAP_ERR_PACKET_RECEIVE:
            return "COAP_ERR_PACKET_RECEIVEt";

        case CoapStatus::COAP_ERR_RECEIVED_PACKET:
            return "COAP_ERR_RECEIVED_PACKET";

        case CoapStatus::COAP_ERR_URI_NOT_FOUND:
            return "COAP_ERR_URI_NOT_FOUND";

        case CoapStatus::COAP_ERR_SERVER_CODE:
            return "COAP_ERR_SERVER_CODE";

        case CoapStatus::COAP_ERR_DECODE_BLOCK_OPTION:
            return "COAP_ERR_DECODE_BLOCK_OPTION";

        case CoapStatus::COAP_ERR_SOCKET_DOMAIN:
            return "COAP_ERR_SOCKET_DOMAIN";

        case CoapStatus::COAP_ERR_MEMORY_ALLOCATE:
            return "COAP_ERR_MEMORY_ALLOCATE";

        case CoapStatus::COAP_ERR_NOT_IMPLEMENTED:
            return "COAP_ERR_NOT_IMPLEMENTED";

        case CoapStatus::COAP_ERR_NOT_CONNECTED:
            return "COAP_ERR_NOT_CONNECTED";

        case CoapStatus::COAP_ERR_EMPTY_HOSTNAME:
            return "COAP_ERR_EMPTY_HOSTNAME";

        case CoapStatus::COAP_ERR_EMPTY_ADDRESS:
            return "COAP_ERR_EMPTY_ADDRESS";

        case CoapStatus::COAP_ERR_REMOVE_CONNECTION:
            return "COAP_ERR_REMOVE_CONNECTION";

        case CoapStatus::COAP_ERR_NO_PAYLOAD:
            return "COAP_ERR_NO_PAYLOAD";

        case CoapStatus::COAP_ERR_CONNECTIONS_EXCEEDED:
            return "COAP_ERR_CONNECTIONS_EXCEEDED";

        case CoapStatus::COAP_ERR_DTLS_CTX_INIT:
            return "COAP_ERR_DTLS_CTX_INI";

        case CoapStatus::COAP_ERR_CREATE_JSON:
            return "COAP_ERR_CREATE_JSON";

        case CoapStatus::COAP_ERR_PARSE_JSON:
            return "COAP_ERR_PARSE_JSON";

        case CoapStatus::COAP_ERR_NO_JSON_FIELD:
            return "COAP_ERR_NO_JSON_FIELD";

        case CoapStatus::COAP_ERR_CREATE_CORE_LINK:
            return "COAP_ERR_CREATE_CORE_LINK";

        case CoapStatus::COAP_ERR_PARSE_CORE_LINK:
            return "COAP_ERR_PARSE_CORE_LINK";
    }
    return "Unknown";
}
