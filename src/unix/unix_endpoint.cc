#include "unix_endpoint.h"
#include "unix_udp_server.h"
#ifdef USE_SPDLOG
#include "spdlog/spdlog.h"
#endif

#ifndef USE_SPDLOG
#define set_level(level)
#define debug(...)
#endif

using namespace std;
#ifdef USE_SPDLOG
using namespace spdlog;
#endif

using namespace coap; 

namespace Unix
{

void ClientEndpoint::idle()
{
	debug("handler: {}",__func__);
	m_nextState = m_currentState = IDLE;
}

void ClientEndpoint::make_request()
{
	debug("handler: {}",__func__);
	timeout(10);
	m_nextState = SEND_REQUEST;
}

void ClientEndpoint::send_request()
{
	debug("handler: {}",__func__);
	m_nextState = RECEIVE_ANSWER;
}

void ClientEndpoint::receive_answer()
{
	debug("handler: {}",__func__);
	m_nextState = HANDLE_ANSWER;
}

void ClientEndpoint::handle_answer()
{
	debug("handler: {}",__func__);
	m_nextState = COMPLETE;
}

void ClientEndpoint::error()
{
	debug("handler: {}",__func__);
	debug("Error occured: {}", m_ec.message());
	m_nextState = IDLE;
}

void ClientEndpoint::complete()
{
	debug("handler: {}",__func__);
	m_nextState = IDLE;
}

void ClientEndpoint::registration_step(std::error_code &ec)
{ 
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
}

void ClientEndpoint::transaction_step(std::error_code &ec)
{
	m_currentState = m_nextState;

	set_level(level::debug);
	debug("Current state: {0:d}", m_currentState);

	switch(m_currentState)
	{
		case IDLE:
			idle();
			break;

		case MAKE_REQUEST:
			make_request();
			break;

		case SEND_REQUEST:
			send_request();
			break;

		case RECEIVE_ANSWER:
			receive_answer();
			break;

		case HANDLE_ANSWER:
			handle_answer();
			break;

		default:
		case ERROR:
			error();
			break;

		case COMPLETE:
			complete();
			break;
	}
	ec = m_ec;
}

void ServerEndpoint::idle()
{
	debug("handler: {}",__func__);
	m_receiving = false;
	m_sending = false;
	m_nextState = IDLE;
}

void ServerEndpoint::receive_request()
{
	debug("handler: {}",__func__);
	m_timeout = 1;

	m_receiving = true;
	m_sending = false;
	m_nextState = HANDLE_REQUEST;
}

void ServerEndpoint::handle_request()
{
	debug("handler: {}",__func__);

	debug("received message length: {0:d}", m_buffer.offset());
	debug("message : {}", m_buffer.data());

	m_receiving = false;
	m_sending = false;
	m_nextState = COMPLETE;
}

void ServerEndpoint::error()
{
	debug("handler: {}",__func__);
	debug("Error occured: {}", m_ec.message());
	m_buffer.clear();
	m_nextState = IDLE;
}

void ServerEndpoint::complete()
{
	debug("handler: {}",__func__);
	m_buffer.clear();
	m_nextState = IDLE;
}

void ServerEndpoint::registration_step(std::error_code &ec)
{ 
	ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
}

void ServerEndpoint::transaction_step(std::error_code &ec)
{
	m_currentState = m_nextState;

	set_level(level::debug);
	debug("Current state: {0:d}", m_currentState);

	switch(m_currentState)
	{
		case IDLE:
			idle();
			break;

		case RECEIVE_REQUEST:
			receive_request();
			break;

		case HANDLE_REQUEST:
			handle_request();
			break;

		default:
		case ERROR:
			error();
			break;

		case COMPLETE:
			complete();
			break;
	}
	ec = m_ec;
}

}// namespace Unix
