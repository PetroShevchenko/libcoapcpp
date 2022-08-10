#include <cstring>
#include <memory>
#include "pico_protocol.h"
#include "log.h"
#include "error.h"

using namespace spdlog;
using namespace std;

namespace Pico
{

void transfer(const std::string &request, std::string &answer, std::error_code &ec)
{
	static PicoProtocol proto(ec);
	if (ec.value()) return;

	if (!proto.constructed())
	{
		ec = proto.ec();
		return;
	}

	PicoProtocol::transfer(&proto, request, answer, ec);
}

} // namespace Pico 

void PicoProtocol::transfer(void *data, const std::string &request, std::string &answer, std::error_code &ec)
{
	DEBUG_LOG_ENTER();
	if (data == nullptr)
	{
        ec = make_system_error(EFAULT);
        DEBUG_LOG_EXIT();
        return;
	}
	PicoProtocol * pp = static_cast<PicoProtocol *>(data);

	pp->m_request = std::move(request);
	pp->m_nextState = MAKE_REQUEST;

	while(pp->m_currentState != COMPLETE && pp->m_currentState != ERROR)
	{
		debug("m_currentState = {0:d}", pp->m_currentState);
		debug("m_nextState = {0:d}", pp->m_nextState);

		switch(pp->m_nextState)
		{
			case MAKE_REQUEST:
				pp->make_request();
				break;

			case SEND_REQUEST:
				pp->send_request();
				break;

			case RECEIVE_HEADER:
				pp->receive_header();
				break;

			case RECEIVE_DATA:
				pp->receive_data();
				break;

			case ERROR:
				pp->error();
				break;

			case COMPLETE:
				pp->complete();
				break;

			default:
				break;
		}
	}

	answer = pp->m_answer;
	ec = pp->m_ec;

	DEBUG_LOG_EXIT();
}

uint8_t PicoProtocol::calculate_crc(uint8_t * data, size_t len)
{
    uint8_t crc = 0xff;
    size_t i, j;
    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

void PicoProtocol::make_request()
{
	DEBUG_LOG_ENTER();
	m_currentState = MAKE_REQUEST;
	if (m_request.length() > PICO_PROTOCOL_DATA_MAX_LENGTH)
	{
		m_ec = make_system_error(EMSGSIZE);
		m_nextState = ERROR;
		DEBUG_LOG_EXIT();
		return;
	}
	size_t msgLen = PICO_PROTOCOL_HEADER_LENGTH + m_request.length();
	m_buffer = std::make_shared<Buffer>(msgLen);

	m_buffer.get()->data()[0] = m_signature[0];
	m_buffer.get()->data()[1] = m_signature[1];
	m_buffer.get()->data()[2] = m_request.length();

	memcpy(&m_buffer.get()->data()[4], reinterpret_cast<const uint8_t *>(m_request.data()), m_request.length());
	m_buffer.get()->offset(msgLen);

	m_buffer.get()->data()[3] = calculate_crc(&m_buffer.get()->data()[4], m_request.length());
	m_nextState = SEND_REQUEST;
	DEBUG_LOG_EXIT();
}	

void PicoProtocol::send_request()
{
	DEBUG_LOG_ENTER();
	m_currentState = SEND_REQUEST;

	m_spi.init(m_ec);
	if (m_ec.value())
	{
		m_nextState = ERROR;
		DEBUG_LOG_EXIT();
		return;
	}

	m_spi.write_blocking(static_cast<const Buffer &>(*m_buffer.get()), m_ec);

	std::shared_ptr<Buffer> deletePtr = std::move(m_buffer);

	if (m_ec.value())
	{
		m_nextState = ERROR;
		DEBUG_LOG_EXIT();
		return;		
	}
	m_nextState = RECEIVE_HEADER;

	DEBUG_LOG_EXIT();
}

void PicoProtocol::receive_header()
{
	DEBUG_LOG_ENTER();
	m_currentState = RECEIVE_HEADER;

	uint8_t header[PICO_PROTOCOL_HEADER_LENGTH];

	m_spi.read_blocking(0, header, PICO_PROTOCOL_HEADER_LENGTH, m_ec);
	if (m_ec.value())
	{
		m_nextState = ERROR;
		DEBUG_LOG_EXIT();
		return;
	}

	if (header[0] != m_signature[0]
	 || header[1] != m_signature[1])
	{
		m_ec = make_system_error(EPROTO);
		m_nextState = ERROR;
		DEBUG_LOG_EXIT();
		return;		
	}
	m_length = header[2];
	m_crc = header[3];
	m_nextState = RECEIVE_DATA;

	DEBUG_LOG_EXIT();
}

void PicoProtocol::receive_data()
{
	DEBUG_LOG_ENTER();
	m_currentState = RECEIVE_DATA;

	if (m_length > PICO_PROTOCOL_DATA_MAX_LENGTH)
	{
		m_ec = make_system_error(EBADMSG);
		m_nextState = ERROR;
		DEBUG_LOG_EXIT();
		return;
	}

	Buffer& rxData = m_spi.read_blocking(0, m_length, m_ec);
	if (m_ec.value())
	{
		m_nextState = ERROR;
		DEBUG_LOG_EXIT();
		return;		
	}

	uint8_t crc = calculate_crc(rxData.data(), m_length);
	if (crc != m_crc)
	{
		m_ec = make_system_error(EBADMSG);
		m_nextState = ERROR;
		DEBUG_LOG_EXIT();
		return;
	}

	m_answer.clear();
	m_answer.reserve(m_length);
	m_answer = reinterpret_cast<const char *>(rxData.data());

	m_nextState = COMPLETE;

	DEBUG_LOG_EXIT();
}

void PicoProtocol::error()
{
	DEBUG_LOG_ENTER();
	m_currentState = ERROR;
	DEBUG_LOG_EXIT();
}

void PicoProtocol::complete()
{
	DEBUG_LOG_ENTER();
	m_currentState = COMPLETE;
	DEBUG_LOG_EXIT();	
}
