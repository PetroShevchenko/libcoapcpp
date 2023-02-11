#include "packet_helper.h"
#include "utils.h"
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bin_to_hex.h>

using namespace std;
using namespace coap;
using namespace spdlog;

const char *coap_options_to_string(coap::Packet &packet)
{
    static string line;
    line.clear();
    for(auto opt: packet.options())
    {
        line += coap_option_number_to_string(static_cast<OptionNumber>(opt.number()));
        line += " ";
    }
    return line.c_str();
}

void log_packet(MessageDirection direction, coap::Packet &packet)
{
    string line;
    line = (direction == INCOMMING) ? "<-- " : "--> ";
    line += "CoAP message: Type: {}; Code: {}; MID: {}; Token({})";
    line += packet.token_length() ? ": {:n}" : "{}";
    line += "; Options({}): ";
    line += coap_options_to_string(packet);
    info(line.c_str(),
            coap_message_type_to_string(static_cast<MessageType>(packet.type())),
            coap_message_code_to_string(static_cast<MessageCode>(packet.code_as_byte())),
            (int)packet.identity(),
            packet.token_length(),
            packet.token_length() ? to_hex(packet.token().begin(), packet.token().end()): to_hex(packet.token().begin(), packet.token().begin()),
            packet.options().size()
        );
}
