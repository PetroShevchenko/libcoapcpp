#include "endpoint.h"
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

namespace coap
{

void ClientEndpoint::idle()
{
	debug("{}",__func__);
	m_nextState = m_currentState = IDLE;
}

void ClientEndpoint::make_request()
{
	debug("{}",__func__);
	m_nextState = SEND_REQUEST;
}

void ClientEndpoint::send_request()
{
	debug("{}",__func__);
	m_nextState = RECEIVE_ANSWER;
}

void ClientEndpoint::receive_answer()
{
	debug("{}",__func__);
	m_nextState = HANDLE_ANSWER;
}

void ClientEndpoint::handle_answer()
{
	debug("{}",__func__);
	m_nextState = COMPLETE;
}

void ClientEndpoint::error()
{
	debug("{}",__func__);
	debug("Error occured: {}", m_ec.message());
	m_nextState = IDLE;
}

void ClientEndpoint::complete()
{
	debug("{}",__func__);
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
	debug("Current state is {0:d}", m_currentState);

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

}// namespace coap
