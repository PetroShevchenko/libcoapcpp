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

        case CoapStatus::COAP_ERR_URI_PATH:
            return "Wrong URI path use something like /0/1 or /first/second";

        case CoapStatus::COAP_ERR_BUFFER_SIZE:
            return "Too small the buffer size";

        case CoapStatus::COAP_ERR_CREATE_SOCKET:
            return "Can not create socket";

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

