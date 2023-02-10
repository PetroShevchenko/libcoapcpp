#ifndef _COAP_CLIENT_H
#define _COAP_CLIENT_H
#include "consts.h"
#include "senml_json.h"
#include "packet.h"
#include "uri.h"
#include "buffer.h"
#include "blockwise.h"
#include "error.h"
#include <fstream>
#include <ctime>

namespace posix
{

class CoapClient
{
    enum FsaState
    {
        FSA_STATE_PREPARE_REQUEST = 0,
        FSA_STATE_PARSE_RESPONSE,
        FSA_STATE_DONE
    };
    #define FSA_STATE_DEFAULT FSA_STATE_PREPARE_REQUEST

public:
	CoapClient(
			std::error_code &ec,
			coap::MethodCode method,
			int port,
			const char *uriPath,
			const char *filename,
			coap::MediaType payloadType,
			std::vector<uint8_t> *payload,
			Buffer &message,
			bool useBlockwise,
			size_t blockSize = 64
		);
	~CoapClient();

private:
	static void prepare_request(void *data);
	static void parse_response(void *data);
	static void done(void *data);

    typedef void (*fsa_state_handler_ptr)(void *);
    static const fsa_state_handler_ptr s_fsa_state_handler[];

    static void prepare_get_request(void *data, std::error_code &ec);
    static void prepare_post_request(void *data, std::error_code &ec);
    static void prepare_put_request(void *data, std::error_code &ec);
    static void prepare_delete_request(void *data, std::error_code &ec);

    static void parse_get_response(void *data, std::error_code &ec);
    static void parse_post_response(void *data, std::error_code &ec);
    static void parse_put_response(void *data, std::error_code &ec);
    static void parse_delete_response(void *data, std::error_code &ec);

    typedef void (*method_handler_ptr)(void *, std::error_code &ec);
    static const method_handler_ptr s_prepare_request_method_handler[];
    static const method_handler_ptr s_parse_response_method_handler[];

private:
	void prepare_request();
	void parse_response();
	void done();

private:
	void serialize_request(std::error_code &ec);

    void prepare_get_request(std::error_code &ec);
    void prepare_post_request(std::error_code &ec);
    void prepare_put_request(std::error_code &ec);
    void prepare_delete_request(std::error_code &ec);

    void parse_get_response(std::error_code &ec);
    void parse_post_response(std::error_code &ec);
    void parse_put_response(std::error_code &ec);
    void parse_delete_response(std::error_code &ec);

public:
	void processing_step();

	bool is_running() const
	{ return m_running; }

	void start()
	{ m_running = true; }

	void stop()
	{ m_running = false; }

	time_t timeout() const
	{ return m_timeout; }

	bool do_send() const
	{ return m_doSend; }

	bool received() const
	{ return m_received; }

	void received(bool value)
	{ m_received = value; }

private:
	coap::MethodCode 	m_method;
	int 				m_port;
	coap::UriPath       m_uriPath;
	std::fstream  		*m_file;
	coap::MediaType     m_payloadType;
	const std::vector<uint8_t>
						*m_payload;
	time_t 		 		m_timeout;
	Buffer       		&m_message;
	coap::Packet 		m_packet;
	std::size_t     	m_attempts;
	std::uint16_t   	m_messageId;
	bool            	m_running;
	bool 				m_received;
	bool 				m_doSend;
	coap::Blockwise		*m_block;
	FsaState 			m_fsaState;
	std::error_code 	m_ec; 
};

}//namespace posix

#endif
