#include "coap_server.h"
#include "trace.h"
#include "core_link.h"
#include "utils.h"
#include <iostream>
#include <cstdint>
#include <arpa/inet.h>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bin_to_hex.h>

using namespace spdlog;
using namespace coap;
using namespace std;

namespace posix
{

const CoapServer::fsa_state_handler_ptr CoapServer::s_fsa_state_handler[] = {
    parse_incomming_coap_packet,
    handle_rst_request,
    handle_get_request,
    handle_post_request,
    handle_put_request,
    handle_delete_request,
    done
};

void CoapServer::parse_incomming_coap_packet(void *data)
{
    ((CoapServer *)data)->parse_incomming_coap_packet();    
}

void CoapServer::handle_rst_request(void *data)
{
    ((CoapServer *)data)->handle_rst_request();
}

void CoapServer::handle_get_request(void *data)
{
    ((CoapServer *)data)->handle_get_request();
}

void CoapServer::handle_post_request(void *data)
{
    ((CoapServer *)data)->handle_post_request();
}

void CoapServer::handle_put_request(void *data)
{
    ((CoapServer *)data)->handle_put_request();
}

void CoapServer::handle_delete_request(void *data)
{
    ((CoapServer *)data)->handle_delete_request();
}

void CoapServer::done(void *data)
{
    ((CoapServer *)data)->done();
}

enum MessageDirection {
    INCOMMING,
    OUTGOING
};

static const char *coap_options_to_string(coap::Packet &packet)
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

static void log_packet(MessageDirection direction, coap::Packet &packet)
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

void CoapServer::parse_incomming_coap_packet()
{
    ENTER_TRACE();

    m_packet.parse(m_message->data(), m_message->offset(), m_ec);
    if (m_ec.value())
    {
        m_fsaState = FSA_STATE_DONE;
        EXIT_TRACE();
        return;     
    }

    log_packet(INCOMMING, m_packet);
    TRACE_PACKET(m_packet);

    switch (m_packet.code_as_byte())
    {
    case EMPTY:
        m_fsaState = FSA_STATE_HANDLE_RST_REQUEST;
        break;

    case GET:
        m_fsaState = FSA_STATE_HANDLE_GET_REQUEST;
        break;

    case POST:
        m_fsaState = FSA_STATE_HANDLE_POST_REQUEST;
        break;

    case PUT:
        m_fsaState = FSA_STATE_HANDLE_PUT_REQUEST;
        break;

    case DELETE:
        m_fsaState = FSA_STATE_HANDLE_DELETE_REQUEST;
        break;

    default:
        m_ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
        m_fsaState = FSA_STATE_DONE;
        break;
    }

    EXIT_TRACE();
}

void CoapServer::handle_rst_request()
{
    ENTER_TRACE();
    m_ec.clear();
    m_message->clear();
    size_t messageId = m_packet.identity();
    m_packet.clear();
    m_packet.identity(messageId);
    prepare_acknowledge_response(m_ec, EMPTY);
    m_fsaState = FSA_STATE_DONE;
    EXIT_TRACE();
}

void CoapServer::handle_get_request()
{
    ENTER_TRACE();
    m_fsaState = FSA_STATE_DONE;

    std::vector<Option *> uri_path;
    size_t qty;

    qty = m_packet.find_option(URI_PATH, uri_path);
    if (qty == 0) // GET request doesn't contain any Uri-Path option
    {
        m_ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
        EXIT_TRACE();
        return;
    }
    std::string path;
    for (size_t i = 0; i < qty; ++i)
    {
        TRACE("Uri-Path [", i, "]:\n");
        TRACE_ARRAY((uri_path[i])->value());
        path += "/" + std::string((uri_path[i])->value().begin(), (uri_path[i])->value().end());
    }
    TRACE("Full path: ", path, "\n");
    if (core_link::is_root(path))
    {
        prepare_content_response(m_ec, LINK_FORMAT, m_coreLink.c_str(), m_coreLink.length());
        if (m_ec.value())
        {
            EXIT_TRACE();
            return;
        }
    }
    else
    {
        process_uri_path(path, m_ec);
        if (m_ec.value())
        {
            EXIT_TRACE();
            return;            
        }
    }
    EXIT_TRACE();
}

void CoapServer::handle_post_request()
{
    ENTER_TRACE();
    m_ec = make_error_code(CoapStatus::COAP_ERR_METHOD_NOT_ALLOWED);
    m_fsaState = FSA_STATE_DONE;
    EXIT_TRACE();
}

void CoapServer::handle_put_request()
{
    ENTER_TRACE();
    m_ec = make_error_code(CoapStatus::COAP_ERR_METHOD_NOT_ALLOWED);
    m_fsaState = FSA_STATE_DONE;
    EXIT_TRACE();
}

void CoapServer::handle_delete_request()
{
    ENTER_TRACE();
    m_ec = make_error_code(CoapStatus::COAP_ERR_METHOD_NOT_ALLOWED);
    m_fsaState = FSA_STATE_DONE;
    EXIT_TRACE();
}

void CoapServer::done()
{
    ENTER_TRACE();
    m_processing = false;
    if (m_ec.value())
    {
        error("Error: {}", m_ec.message());
        prepare_error_response(m_ec);
    }
    EXIT_TRACE();
}

void CoapServer::processing(Buffer &buf)
{
    set_level(level::info);

    m_fsaState = FSA_STATE_DEFAULT;
    m_processing = true;
    m_message = &buf;

    TRACE_ARRAY(buf);

    while(m_processing)
    {
        if (s_fsa_state_handler[m_fsaState])
            s_fsa_state_handler[m_fsaState](this);
    }
}

MessageCode CoapServer::make_protocol_error_code(std::error_code &ec)
{
    switch(ec.value())
    {
    case CoapStatus::COAP_ERR_BAD_REQUEST:
        return BAD_REQUEST;
    case CoapStatus::COAP_ERR_NOT_IMPLEMENTED:
        return NOT_IMPLEMENTED;
    case CoapStatus::COAP_ERR_METHOD_NOT_ALLOWED:
        return METHOD_NOT_ALLOWED;
    default:
        return INTERNAL_SERVER_ERROR;
    }
}

size_t CoapServer::hton_content_format(
            std::error_code &ec,
            MediaType contentFormat,
            uint8_t(&out)[sizeof(MediaType)]
        )
{
    ENTER_TRACE();
    memset(&out[0], 0, sizeof(MediaType));
    size_t optionSize;
    if (contentFormat <= UINT8_MAX)
    {
        uint16_t cf = htons(static_cast<uint16_t>(contentFormat));
        TRACE("cf: ", cf, "\n");
        out[0] = cf >> 8;
        optionSize = sizeof(uint8_t);
    }
    else if (contentFormat <= UINT16_MAX)
    {
        uint16_t cf = htons(static_cast<uint16_t>(contentFormat));
        TRACE("cf: ", cf, "\n");
        memcpy(&out[0], &cf, sizeof(uint16_t));        
        optionSize = sizeof(uint16_t);
    }
    else if (contentFormat <= UINT32_MAX)
    {
        uint32_t cf = htonl(static_cast<uint32_t>(contentFormat));
        TRACE("cf: ", cf, "\n");
        memcpy(&out[0], &cf, sizeof(uint32_t));
        optionSize = sizeof(uint32_t);        
    }
    else
    {
        ec = make_error_code(CoapStatus::COAP_ERR_OPTION_VALUE);
        EXIT_TRACE();
        return 0;       
    }
    EXIT_TRACE();
    return optionSize;
}

void CoapServer::serialize_response(std::error_code &ec)
{
    ENTER_TRACE();
    size_t length = m_message->length();
    m_packet.serialize(ec, m_message->data(), length);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    m_message->offset(length);
    EXIT_TRACE();     
}

void CoapServer::prepare_acknowledge_response(std::error_code &ec, MessageCode resposeCode)
{
    ENTER_TRACE();
    m_packet.options().clear(); // clear all options
    m_packet.prepare_answer(
            ec,
            ACKNOWLEDGEMENT,
            resposeCode,
            m_packet.identity(),
            nullptr,
            0
        );
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    log_packet(OUTGOING, m_packet);
    TRACE_PACKET(m_packet);

    serialize_response(ec);
    EXIT_TRACE();
}

void CoapServer::prepare_error_response(std::error_code &ec)
{
    ENTER_TRACE();
    prepare_acknowledge_response(ec, make_protocol_error_code(ec));
    EXIT_TRACE();
}

void CoapServer::add_content_format_option(MediaType contentFormat, std::error_code &ec)
{
    ENTER_TRACE();
    uint8_t value[sizeof(MediaType)] = {0,};
    size_t optionSize = hton_content_format(ec, contentFormat, value);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    TRACE("Content Format: ", contentFormat, "\n");
    TRACE("Option Size: ", optionSize, "\n");
    m_packet.add_option(CONTENT_FORMAT, value, optionSize, ec);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    EXIT_TRACE();
}

void CoapServer::process_block2_option(Block2 &block2, size_t totalSize, size_t &payloadLength, std::error_code &ec)
{
    ENTER_TRACE();

    block2.total(totalSize);

    if (block2.size() < (block2.total() - block2.offset()))// if there is required to fragment the packet
    {
        block2.more(true);
        payloadLength = block2.size();
    }
    else
    {
        block2.more(false);
        payloadLength = block2.total() - block2.offset();
    }
    TRACE("BLOCK2: NUM:", block2.number(), ", M:", block2.more(), ", SZX:", block2.size(),
                            ", Offset:", block2.offset(), " Total:", block2.total(), "\n");
    TRACE("Payload Length: ", payloadLength, "\n");

    m_packet.options().clear(); // clear all options

    Option option;

    if (!block2.encode_block_option(option))
    {
        ec = make_error_code(CoapStatus::COAP_ERR_ENCODE_BLOCK_OPTION);
        EXIT_TRACE();
        return;
    }
    m_packet.options().push_back(option);
    EXIT_TRACE();
}

void CoapServer::prepare_content_response(
            std::error_code &ec,
            MediaType contentFormat,
            const void *data,
            size_t size
        )
{
    ENTER_TRACE();
    Block2 block2;
    bool status = block2.get_header(m_packet, &ec);// extract BLOCK_2 option from the incoming packet
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }

    const uint8_t *payload = static_cast<const uint8_t *>(data);
    size_t payloadLength;

    if (status) // there is BLOCK_2 option in the incomming packet
    {
        payload += block2.offset();

        process_block2_option(block2, size, payloadLength, ec);
        if (ec.value())
        {
            EXIT_TRACE();
            return;
        }
    }
    else
    {
        payloadLength = size;
        m_packet.options().clear(); // clear all options
    }

    add_content_format_option(contentFormat, ec);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    m_packet.prepare_answer(ec, ACKNOWLEDGEMENT, CONTENT, m_packet.identity(), payload, payloadLength);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    log_packet(OUTGOING, m_packet);
    TRACE_PACKET(m_packet);

    serialize_response(ec);
    EXIT_TRACE();
}

void CoapServer::prepare_content_response(
            std::error_code &ec,
            MediaType contentFormat,
            const char *filename
        )
{
    ENTER_TRACE();
    ifstream ifs(filename, ios::binary);
    if (!ifs.is_open())
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_FOUND);
        EXIT_TRACE();
        return;        
    }

    ifs.seekg(0, std::ios::end);
    size_t fileSize = ifs.tellg();
    ifs.seekg(0);
    Block2 block2;

    bool status = block2.get_header(m_packet, &ec);// extract BLOCK_2 option from the incoming packet
    if (ec.value())
    {
        ifs.close();
        EXIT_TRACE();
        return;
    }

    if (!status && fileSize > 1024) // if there is no BLOCK_2 option the file size must be up to 1024 bytes
    {
        ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
        ifs.close();
        EXIT_TRACE();
        return;        
    }

    std::array<char, 1024> payload;
    size_t payloadLength;

    if (status) // there is BLOCK_2 option in the incomming packet
    {
        ifs.seekg(block2.offset());
        process_block2_option(block2, fileSize, payloadLength, ec);
        if (ec.value())
        {
            EXIT_TRACE();
            ifs.close();
            return;
        }
    }
    else
    {
        m_packet.options().clear(); // clear all options
        payloadLength = fileSize;
    }

    ifs.read(&payload[0], payloadLength);
    ifs.close();

    add_content_format_option(contentFormat, ec);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    m_packet.prepare_answer(ec, ACKNOWLEDGEMENT, CONTENT, m_packet.identity(), payload.data(), payloadLength);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    log_packet(OUTGOING, m_packet);
    TRACE_PACKET(m_packet);

    serialize_response(ec);
    EXIT_TRACE();
}

void CoapServer::process_uri_path(std::string &path, std::error_code &ec)
{
    ENTER_TRACE();

    if (path[0] == '/')
        path.erase(0,1);
    if(path.back() == '/')
        path.pop_back();
    TRACE("path: ", path.c_str(), "\n");

    CoreLink parser;
    parser.parse_core_link(m_coreLink.c_str(), ec);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    for (vector<CoreLinkType>::const_iterator
            iter = parser.payload().begin(),
            end = parser.payload().end(); iter != end; ++iter)
    {
        if (core_link::is_record_matched(path, *iter))
        {
            std::vector<CoreLinkParameter>::const_iterator attribute_iterator;
            attribute_iterator = core_link::find_attribute("rt", *iter);
            if (attribute_iterator != iter->parameters.end()
                && core_link::is_attribute_matched("rt", "firmware", *attribute_iterator))
            {
                path = "data/" + path;
                prepare_content_response(ec, OCTET_STREAM, path.c_str());
                EXIT_TRACE();
                return;
            }
            // TODO prepare Core-Link payload 
        }
    }
    //XXX
    ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
    // TODO prepare Core-Link payload
    EXIT_TRACE();
}

}
