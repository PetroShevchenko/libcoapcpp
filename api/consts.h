#ifndef _CONSTS_H
#define _CONSTS_H
#include <cstdint>

namespace coap
{

const uint8_t COAP_VERSION = 0x1;
const uint8_t PACKET_HEADER_SIZE = 4;
const uint8_t PACKET_MIN_LENGTH = PACKET_HEADER_SIZE;
const uint8_t TOKEN_MAX_LENGTH = 8;
const uint8_t MINUS_THIRTEEN_OPT_VALUE = 13;
const uint16_t MINUS_TWO_HUNDRED_SIXTY_NINE_OPT_VALUE = 269;
const uint16_t OPTION_MAX_LENGTH = 256;
const uint8_t PAYLOAD_MARKER = 0xFF;

enum MessageOffset
{
    HEADER_OFFSET       = 0x0,
    HEADER_SIZE         = 0x1,
    CODE_OFFSET         = HEADER_OFFSET + HEADER_SIZE,
    CODE_SIZE           = 0x1,
    MESSAGE_ID_OFFSET   = CODE_OFFSET + CODE_SIZE,
    MESSAGE_ID_SIZE     = 0x2,
    TOKEN_OFFSET        = MESSAGE_ID_OFFSET + MESSAGE_ID_SIZE
};

enum MessageType
{
    CONFIRMABLE     = 0x0,
    NON_CONFIRMABLE = 0x1,
    ACKNOWLEDGEMENT = 0x2,
    RESET           = 0x3
};

enum MessageCodeClass
{
    METHOD          = (0x0 << 5),
    SUCCESS         = (0x2 << 5),
    CLIENT_ERROR    = (0x4 << 5),
    SERVER_ERROR    = (0x5 << 5),
    SIGNALING_CODES = (0x7 << 5)
};

enum MessageCode
{
    EMPTY   = METHOD | 0x0,
    GET     = METHOD | 0x1,
    POST    = METHOD | 0x2,
    PUT     = METHOD | 0x3,
    DELETE  = METHOD | 0x4,
    FETCH   = METHOD | 0x5,
    PATCH   = METHOD | 0x6,
    IPATCH  = METHOD | 0x7,
    CREATED     = SUCCESS | 0x1,
    DELETED     = SUCCESS | 0x2,
    VALID       = SUCCESS | 0x3,
    CHANGED     = SUCCESS | 0x4,
    CONTENT     = SUCCESS | 0x5,
    CONTINUE    = SUCCESS | 0x3F,
    BAD_REQUEST     = CLIENT_ERROR | 0x0,
    UNAUTHORIZED    = CLIENT_ERROR | 0x1,
    BAD_OPTION      = CLIENT_ERROR | 0x2,
    FORBIDDED       = CLIENT_ERROR | 0x3,
    NOT_FOUND       = CLIENT_ERROR | 0x4,
    METHOD_NOT_ALLOWED  = CLIENT_ERROR | 0x5,
    NOT_ACCEPTABLE      = CLIENT_ERROR | 0x6,
    REQUEST_ENTITY_INCOMPLETE   = CLIENT_ERROR | 0x8,
    CONFLICT            = CLIENT_ERROR | 0x9,
    PRECONDITION_FAILED = CLIENT_ERROR | 0xC,
    REQUEST_ENTITY_TOO_LARGE    = CLIENT_ERROR | 0xD,
    UNSUPPORTED_CONTENT_FORMAT  = CLIENT_ERROR | 0xF,
    INTERNAL_SERVER_ERROR       = SERVER_ERROR | 0x0,
    NOT_IMPLEMENTED             = SERVER_ERROR | 0x1,
    BAD_GATEWAY                 = SERVER_ERROR | 0x2,
    SERVICE_UNAVAILABLE         = SERVER_ERROR | 0x3,
    GATEWAY_TIMEOUT             = SERVER_ERROR | 0x4,
    PROXYING_NOT_SUPPORTED      = SERVER_ERROR | 0x5,
    UNASSIGNED  = SIGNALING_CODES  | 0x0,
    CSM         = SIGNALING_CODES  | 0x1,
    PING        = SIGNALING_CODES  | 0x2,
    PONG        = SIGNALING_CODES  | 0x3,
    RELEASE     = SIGNALING_CODES  | 0x4,
    ABORT       = SIGNALING_CODES  | 0x5
};

enum OptionDelta
{
    MINUS_THIRTEEN = 13,
    MINUS_TWO_HUNDRED_SIXTY_NINE = 14,
    RESERVED_FOR_FUTURE = 15
};

enum OptionNumber
{
    IF_MATCH        = 0,
    URI_HOST        = 3,
    ETAG            = 4,
    IF_NONE_MATCH   = 5,
    URI_PORT        = 7,
    LOCATION_PATH   = 8,
    URI_PATH        = 11,
    CONTENT_FORMAT  = 12,
    MAX_AGE         = 14,
    URI_QUERY       = 15,
    ACCEPT          = 17,
    LOCATION_QUERY  = 20,
    BLOCK_2         = 23,
    BLOCK_1         = 27,
    SIZE_2          = 28,
    PROXY_URI       = 35,
    PROXY_SCHEME    = 39,
    SIZE_1          = 60,
    OPTION_MAX_NUMBER = SIZE_1
};

enum MediaType
{
    TEXT_PLAIN      = 0,
    LINK_FORMAT     = 40,   //application/link-format
    XML             = 41,
    OCTET_STREAM    = 42,   //application/octet-stream
    EXI             = 47,
    COAP_JSON       = 50,
    SENML_JSON      = 110,  //application/senml+json
    SENML_CBOR      = 112,  //application/senml+cbor
    LWM2M_TLV       = 11542,//application/vnd.oma.lwm2m+tlv
    LWM2M_JSON      = 11543 //application/vnd.oma.lwm2m+json
};

enum MethodCode
{
    METHOD_GET,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHODS_COUNT
};

} // namespace coap

#endif
