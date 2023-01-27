#include "coap_server.h"
#include "trace.h"
#include "core_link.h"
#include <iostream>
#include <cstdint>
#include <arpa/inet.h>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

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
    TRACE("COAP Version: ", (int)m_packet.version(), "\n");
    TRACE("Message Type: ", (int)m_packet.type(), "\n");
    TRACE("Token length: ", (int)m_packet.token_length(), "\n");
    if (m_packet.token_length())
    {
        TRACE("Token:\n");
        TRACE_ARRAY(m_packet.token());
    }
    TRACE("Message Code: 0x", std::hex, (int)m_packet.code_as_byte(), "\n");
    TRACE("Message Id: ", std::dec, (int)m_packet.identity(), "\n");
    TRACE("Options: ", (int)m_packet.options().size(), "\n");

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
        debug("Error: {}", m_ec.message());
        prepare_error_response(m_ec);
    }
    EXIT_TRACE();
}

void CoapServer::processing(Buffer &buf)
{
    set_level(level::debug);

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
        TRACE("cf = ", cf, "\n");
        out[0] = cf >> 8;
        optionSize = sizeof(uint8_t);
    }
    else if (contentFormat <= UINT16_MAX)
    {
        uint16_t cf = htons(static_cast<uint16_t>(contentFormat));
        TRACE("cf = ", cf, "\n");
        memcpy(&out[0], &cf, sizeof(uint16_t));        
        optionSize = sizeof(uint16_t);
    }
    else if (contentFormat <= UINT32_MAX)
    {
        uint32_t cf = htonl(static_cast<uint32_t>(contentFormat));
        TRACE("cf = ", cf, "\n");
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
    serialize_response(ec);
    EXIT_TRACE();
}

void CoapServer::prepare_error_response(std::error_code &ec)
{
    ENTER_TRACE();
    prepare_acknowledge_response(ec, make_protocol_error_code(ec));
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
    bool status = block2.get_header(m_packet, &ec);// check if the packet contains a BLOCK_2 option
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    const uint8_t *payload = static_cast<const uint8_t *>(data);
    size_t payloadLength = size; 
    if (status) // there is a BLOCK_2 option in the packet
    {
        TRACE("BLOCK2 number = ", block2.number(), "\n");
        TRACE("BLOCK2 offset = ", block2.offset(), "\n");
        TRACE("BLOCK2 size = ", block2.size(), "\n");

        block2.total(size);
        payload += block2.offset();
        TRACE("BLOCK2 total = ", block2.total(), "\n");
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
        TRACE("BLOCK2 more = ", block2.more(), "\n");
        TRACE("BLOCK2 payload length = ", payloadLength, "\n");

        m_packet.options().clear(); // clear all options

        Option option;

        if (!block2.encode_block_option(option))
        {
            ec = make_error_code(CoapStatus::COAP_ERR_ENCODE_BLOCK_OPTION);
            EXIT_TRACE();
            return;
        }
        m_packet.options().push_back(option);
    }
    else m_packet.options().clear(); // clear all options

    uint8_t value[sizeof(MediaType)] = {0,};
    size_t optionSize = hton_content_format(ec, contentFormat, value);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    TRACE("contentFormat = ", contentFormat, "\n");
    TRACE("optionSize = ", optionSize, "\n");

    m_packet.add_option(CONTENT_FORMAT, value, optionSize, ec);
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
    size_t fileSize;
    ifs.seekg(0, std::ios::end);
    fileSize = ifs.tellg();
    ifs.seekg(0);

    Block2 block2;
    bool status = block2.get_header(m_packet, &ec);// check if the packet contains a BLOCK_2 option
    if (ec.value())
    {
        ifs.close();
        EXIT_TRACE();
        return;
    }

    if (!status && fileSize > 1024) // check if there is no BLOCK_2 option and the file size is greater than 1024
    {
        ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
        ifs.close();
        EXIT_TRACE();
        return;        
    }
    std::array<char, 1024> payload;
    size_t payloadLength;

    if (status) // there is a BLOCK_2 option in the packet
    {
        TRACE("BLOCK2 number = ", block2.number(), "\n");
        TRACE("BLOCK2 offset = ", block2.offset(), "\n");
        TRACE("BLOCK2 size = ", block2.size(), "\n");

        block2.total(fileSize);
        ifs.seekg(block2.offset());

        TRACE("BLOCK2 total = ", block2.total(), "\n");
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
        ifs.read(&payload[0], payloadLength);
        ifs.close();

        TRACE("BLOCK2 more = ", block2.more(), "\n");
        TRACE("BLOCK2 payload length = ", payloadLength, "\n");

        m_packet.options().clear(); // clear all options

        Option option;

        if (!block2.encode_block_option(option))
        {
            ec = make_error_code(CoapStatus::COAP_ERR_ENCODE_BLOCK_OPTION);
            EXIT_TRACE();
            return;
        }
        m_packet.options().push_back(option);
    }
    else 
    {
        m_packet.options().clear(); // clear all options
        payloadLength = fileSize;
    } 

    uint8_t value[sizeof(MediaType)] = {0,};
    size_t optionSize = hton_content_format(ec, contentFormat, value);
    if (ec.value())
    {
        EXIT_TRACE();
        return;
    }
    TRACE("contentFormat = ", contentFormat, "\n");
    TRACE("optionSize = ", optionSize, "\n");

    m_packet.add_option(CONTENT_FORMAT, value, optionSize, ec);
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
    TRACE("path = ", path.c_str(), "\n");

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
