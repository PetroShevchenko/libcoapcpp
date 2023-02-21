#include "coap_client.h"
#include "packet_helper.h"
#include "trace.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bin_to_hex.h>

using namespace std;
using namespace coap;
using namespace spdlog;

namespace posix
{

enum {
	MAX_RECEIVE_ATTEMPTS = 4,
	MAX_RECEIVE_TIMEOUT_IN_SECONDS = 2,
};

const CoapClient::fsa_state_handler_ptr CoapClient::s_fsa_state_handler[] = {
    prepare_request,
    parse_response,
    done
};

const CoapClient::method_handler_ptr CoapClient::s_prepare_request_method_handler[] = {
	prepare_get_request,
	prepare_post_request,
	prepare_put_request,
	prepare_delete_request
};

const CoapClient::method_handler_ptr CoapClient::s_parse_response_method_handler[] = {
	parse_get_response,
	parse_post_response,
	parse_put_response,
	parse_delete_response
};

static inline bool is_method_correct(MethodCode code)
{
	return (code >= METHOD_GET && code <= METHOD_DELETE);
}

CoapClient::CoapClient(
		std::error_code &ec,
		MethodCode method,
		int port,
		const char *uriPath,
		const char *filename,
		MediaType payloadType,
		vector<uint8_t> *payload,
		Buffer &message,
		bool useBlockwise,
		size_t blockSize
	)
	: m_method{method},
	  m_port{port},
	  m_uriPath{uriPath, ec},
	  m_file{nullptr},
	  m_payloadType{payloadType},
	  m_payload{payload},
	  m_timeout{0},
	  m_message{message},
	  m_packet{},
	  m_attempts{0},
	  m_messageId{0},
	  m_running{false},
	  m_received{false},
	  m_doSend{false},
	  m_doReceive{false},
	  m_block{nullptr},
	  m_fsaState{FSA_STATE_PREPARE_REQUEST},
	  m_ec{}
{
	if (ec.value())
		return;

	size_t size = 0;// payload size for PUT, POST

	if (!is_method_correct(method))
	{
		ec = make_error_code(CoapStatus::COAP_ERR_METHOD_NOT_ALLOWED);
		return;		
	}
	if (filename && method != METHOD_DELETE)
	{
		std::ios_base::openmode mode = (method == METHOD_GET) ? (std::ios::out | std::ios::binary | std::ios::app) : std::ios::in;
		m_file = new std::fstream(filename, mode);
		if (m_file == nullptr)
		{
			ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
			return;
		}
		if (method == METHOD_PUT
			|| method == METHOD_POST)// determine file size to set for block option
		{
			m_file->seekg(0, std::ios::end);
			size = m_file->tellg();
			m_file->seekg(0);
		}
	}
	else if (m_payload
			 && (method == METHOD_POST
			 || method == METHOD_PUT))// determine payload size to set for block option
		size = m_payload->size();
	if (useBlockwise && method != METHOD_DELETE)
	{
		if (method == METHOD_PUT || method == METHOD_POST)
			m_block = new Block1();
		else if (method == METHOD_GET)
			m_block = new Block2();		

		if (m_block == nullptr)
		{
			ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
			return;
		}
		if (blockSize < 16
		  || blockSize > 1024
		  || (blockSize % 16))
		{
			ec = make_error_code(CoapStatus::COAP_ERR_BLOCK_SIZE);
			return;
		}
		m_block->size(blockSize);
		m_block->total(size);
		m_block->offset(0);
	}
	if (!useBlockwise && size > BUFFER_SIZE)
		ec = make_error_code(CoapStatus::COAP_ERR_LARGE_PAYLOAD);
}

CoapClient::~CoapClient()
{
	if (m_block)
		delete m_block;
	if (m_file)
	{
		m_file->close();
		delete m_file;
	}
}

void CoapClient::prepare_request(void *data)
{
	((CoapClient *)data)->prepare_request();
}

void CoapClient::parse_response(void *data)
{
	((CoapClient *)data)->parse_response();
}

void CoapClient::done(void *data)
{
	((CoapClient *)data)->done();
}

void CoapClient::prepare_get_request(void *data, std::error_code &ec)
{
	((CoapClient *)data)->prepare_get_request(ec);	
}

void CoapClient::prepare_post_request(void *data, std::error_code &ec)
{
	((CoapClient *)data)->prepare_post_request(ec);	
}

void CoapClient::prepare_put_request(void *data, std::error_code &ec)
{
	((CoapClient *)data)->prepare_put_request(ec);	
}

void CoapClient::prepare_delete_request(void *data, std::error_code &ec)
{
	((CoapClient *)data)->prepare_delete_request(ec);	
}

void CoapClient::parse_get_response(void *data, std::error_code &ec)
{
	((CoapClient *)data)->parse_get_response(ec);	
}

void CoapClient::parse_post_response(void *data, std::error_code &ec)
{
	((CoapClient *)data)->parse_post_response(ec);	
}

void CoapClient::parse_put_response(void *data, std::error_code &ec)
{
	((CoapClient *)data)->parse_put_response(ec);	
}

void CoapClient::parse_delete_response(void *data, std::error_code &ec)
{
	((CoapClient *)data)->parse_delete_response(ec);	
}

void CoapClient::serialize_request(std::error_code &ec)
{
	ENTER_TRACE();
	size_t length;
    m_packet.serialize(ec, nullptr, length, true); //determine buffer length
	if (ec.value())
	{
		EXIT_TRACE();
		return;		
	}
	m_packet.serialize(ec, m_message.data(), length);
	m_packet.payload().clear(); // to prevent this payload from being used in the next message
	if (ec.value())
	{
		EXIT_TRACE();
		return;		
	}
	m_message.offset(length);
	EXIT_TRACE();
}

void CoapClient::prepare_get_request(std::error_code &ec)
{
	ENTER_TRACE();
	if (m_block && !((Block2 *)m_block)->set_header(m_port, m_uriPath, m_packet, &ec))
	{
		EXIT_TRACE();
		return;
	}
	if (m_messageId == 0)
	{
		m_messageId = generate_identity();
	}
	if (!m_block)
	{
		if (!add_uri_path_option(m_port, m_uriPath, m_packet, ec))
		{
			EXIT_TRACE();
			return;
		}
	}
	m_packet.make_request(ec, CONFIRMABLE, GET, m_messageId, nullptr, 0);
	if (ec.value())
	{
		EXIT_TRACE();
		return;		
	}
	serialize_request(ec);
	if (ec.value())
	{
		EXIT_TRACE();
		return;		
	}
	EXIT_TRACE();
}

void CoapClient::prepare_post_request(std::error_code &ec)
{
	ENTER_TRACE();
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
/*	if (m_method != METHOD_POST
		|| m_method != METHOD_PUT)
	{
		ec = make_error_code(CoapStatus::COAP_ERR_METHOD_NOT_ALLOWED);
		EXIT_TRACE();
		return;		
	}
	if (m_block && !((Block1 *)m_block)->set_header(m_port, m_uriPath, m_packet, &ec))
	{
		EXIT_TRACE();
		return;
	}
	if (m_messageId == 0)
	{
		m_messageId = generate_identity();
	}
	size_t size;
	if (m_file)
	{
		if (m_block)
		{
			size = ((Block1 *)m_block)->size();
			m_file->seekg(((Block1 *)m_block)->offset());
		}
		else {

		}

	}
	else if (m_payload)
	{

	}
	m_packet.make_request(ec, CONFIRMABLE, m_method == METHOD_POST ? POST : PUT, m_messageId, nullptr, 0);
	if (ec.value())
	{
		EXIT_TRACE();
		return;		
	}
	serialize_request(ec);
	if (ec.value())
	{
		EXIT_TRACE();
		return;		
	}*/
	EXIT_TRACE();
}

void CoapClient::prepare_put_request(std::error_code &ec)
{
	ENTER_TRACE();
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
	EXIT_TRACE();
}

void CoapClient::prepare_delete_request(std::error_code &ec)
{
	ENTER_TRACE();
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
	EXIT_TRACE();
}

void CoapClient::parse_get_response(std::error_code &ec)
{
	ENTER_TRACE();
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
	EXIT_TRACE();	
}

void CoapClient::parse_post_response(std::error_code &ec)
{
	ENTER_TRACE();
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
	EXIT_TRACE();	
}

void CoapClient::parse_put_response(std::error_code &ec)
{
	ENTER_TRACE();
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
	EXIT_TRACE();	
}

void CoapClient::parse_delete_response(std::error_code &ec)
{
	ENTER_TRACE();
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
	EXIT_TRACE();	
}

void CoapClient::prepare_request()
{
	ENTER_TRACE();
	if (s_prepare_request_method_handler[m_method])
		s_prepare_request_method_handler[m_method](this, m_ec);

	if (m_ec.value())
	{
		m_fsaState = FSA_STATE_DONE;
		EXIT_TRACE();
		return;		
	}
	m_doSend = true; 							// need to send message
	m_doReceive = false; 						// receiving is not expected
	m_timeout = MAX_RECEIVE_TIMEOUT_IN_SECONDS; // receive timeout
	m_fsaState = FSA_STATE_PARSE_RESPONSE;		// next state
	EXIT_TRACE();
}

void CoapClient::parse_response()
{
	ENTER_TRACE();
	if (s_parse_response_method_handler[m_method])
		s_parse_response_method_handler[m_method](this, m_ec);

	if (m_ec.value())
	{
		m_fsaState = FSA_STATE_DONE;
		EXIT_TRACE();
		return;		
	}
	m_doSend = false; 							// no need to send message
	m_doReceive = false; 						// receiving is not expected
	EXIT_TRACE();
}

void CoapClient::done()
{
	ENTER_TRACE();
	stop();
	//TODO
	EXIT_TRACE();
}

void CoapClient::processing_step()
{
    set_level(level::info);
    if (m_running && s_fsa_state_handler[m_fsaState])
        s_fsa_state_handler[m_fsaState](this);
    m_timeout = m_doReceive ? MAX_RECEIVE_TIMEOUT_IN_SECONDS : 0;
}

} //namespace posix
