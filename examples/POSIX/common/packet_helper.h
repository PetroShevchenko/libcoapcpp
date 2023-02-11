#ifndef _PACKET_HELPER_H
#define _PACKET_HELPER_H
#include "trace.h"
#include "packet.h"

#ifdef TRACE_CODE
#define TRACE_PACKET(packet) do{\
    TRACE("COAP message: Type: ", coap_message_type_to_string(static_cast<MessageType>(packet.type())),\
             "; Code: ", coap_message_code_to_string(static_cast<MessageCode>(packet.code_as_byte())),\
             "; MID: ", (int)packet.identity(),\
             "; Token(", (int)packet.token_length(), ")\n");\
    if (packet.token_length()) {\
        TRACE("Token content:\n");\
        TRACE_ARRAY(packet.token());\
    }\
    if (packet.options().size()) {\
        TRACE("Options(", (int)packet.options().size(), "): ", coap_options_to_string(packet), "\n");\
    }\
    if (packet.payload().size()) {\
        TRACE("Payload content:\n");\
        TRACE_ARRAY(packet.payload());\
    }\
}while(0)
#else
#define TRACE_PACKET(packet)
#endif

enum MessageDirection {
    INCOMMING,
    OUTGOING
};

const char *coap_options_to_string(coap::Packet &packet);
void log_packet(MessageDirection direction, coap::Packet &packet);

#endif
