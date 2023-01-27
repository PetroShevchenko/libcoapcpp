#ifndef _COAP_SERVER_H
#define _COAP_SERVER_H
#include <atomic>
#include <ctime>
#include "buffer.h"
#include "error.h"
#include "packet.h"
#include "blockwise.h"

namespace posix
{

class CoapServer
{
    enum FsaState
    {
        FSA_STATE_PARSE_INCOMMING_COAP_PACKET = 0,
        FSA_STATE_HANDLE_RST_REQUEST,
        FSA_STATE_HANDLE_GET_REQUEST,
        FSA_STATE_HANDLE_POST_REQUEST,
        FSA_STATE_HANDLE_PUT_REQUEST,
        FSA_STATE_HANDLE_DELETE_REQUEST,
        FSA_STATE_DONE
    };
    #define FSA_STATE_DEFAULT FSA_STATE_PARSE_INCOMMING_COAP_PACKET

public:
    CoapServer(
            const char *name,
            const char *coreLink
        ) : m_name{name},
            m_coreLink{coreLink},
            m_timeout{0},
            m_running{false},
            m_processing{false},
            m_ec{},
            m_fsaState{FSA_STATE_DEFAULT},
            m_message{nullptr},
            m_packet{}
        {}
    ~CoapServer()
        {}

private:
    static void parse_incomming_coap_packet(void *data);
    static void handle_rst_request(void *data);
    static void handle_get_request(void *data);
    static void handle_post_request(void *data);
    static void handle_put_request(void *data);
    static void handle_delete_request(void *data);    
    static void done(void *data);
    
    typedef void (*fsa_state_handler_ptr)(void *);
    static const fsa_state_handler_ptr s_fsa_state_handler[];

private:
    void parse_incomming_coap_packet();
    void handle_rst_request();
    void handle_get_request();
    void handle_post_request();
    void handle_put_request();
    void handle_delete_request();
    void done();

private:
    static coap::MessageCode make_protocol_error_code(std::error_code &ec);
    static size_t hton_content_format(
            std::error_code &ec,
            coap::MediaType contentFormat,
            uint8_t(&out)[sizeof(coap::MediaType)]
        );

private:
    void serialize_response(std::error_code &ec);
    void prepare_acknowledge_response(std::error_code &ec, coap::MessageCode resposeCode);
    void prepare_error_response(std::error_code &ec);
    void prepare_content_response(std::error_code &ec, coap::MediaType contentFormat, const void *data, size_t size);
    void prepare_content_response(std::error_code &ec, coap::MediaType contentFormat, const char *filename);    
    void process_uri_path(std::string &path, std::error_code &ec);

public:
    void processing(Buffer &buf);

public:
    void start()
    { m_running = true; }

    void stop()
    { m_running = false; }

    bool is_running() const
    { return m_running;}

    void timeout(time_t value)
    { m_timeout = value; }

    time_t timeout() const
    { return m_timeout; }

    Buffer *message()
    { return m_message; }

    void message(Buffer *value)
    { m_message = value; }

private:
    std::string                 m_name;
    std::string                 m_coreLink;
    time_t                      m_timeout;
    std::atomic<bool>           m_running;
    std::atomic<bool>           m_processing;
    std::error_code             m_ec;
    FsaState                    m_fsaState;
    Buffer                      *m_message;
    coap::Packet                m_packet;
};

}

#endif
