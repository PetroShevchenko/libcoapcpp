#include "pico_protocol.h"
#include <cstring>

using namespace std;

void PicoProtocol::process(std::error_code &ec)
{
    m_nextState = RECEIVE_HEADER;

    while(m_currentState != COMPLETE && m_currentState != ERROR)
    {
        printf("m_currentState = %d\n", m_currentState);
        printf("m_nextState = %d\n", m_nextState);

        switch(m_nextState)
        {
            case RECEIVE_HEADER:
                receive_header();
                break;

            case RECEIVE_DATA:
                receive_data();
                break;

            case HANDLE_REQUEST:
                handle_request();
                break;

            case MAKE_ANSWER:
                make_answer();
                break;

            case SEND_ANSWER:
                send_answer();
                break;

            case ERROR:
                error();
                break;

            case COMPLETE:
                complete();
                break;
            
            default:
                break;
        }
    }
    ec = m_ec;
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

void PicoProtocol::receive_header()
{
    m_currentState = RECEIVE_HEADER;

    uint8_t header[PICO_PROTOCOL_HEADER_LENGTH] = {};

    printf("m_spi = %p\n", m_spi);

    int received = spi_read_blocking(m_spi, 0, header, PICO_PROTOCOL_HEADER_LENGTH);

    if (received != PICO_PROTOCOL_HEADER_LENGTH)
    {
        m_ec = make_system_error(EPROTO);
        m_nextState = ERROR;
        return;
    }
	if (header[0] != m_signature[0]
	 || header[1] != m_signature[1])
	{
		m_ec = make_system_error(EPROTO);
		m_nextState = ERROR;
		return;		
	}
	m_length = header[2];
	m_crc = header[3];
	m_nextState = RECEIVE_DATA;
}	

void PicoProtocol::receive_data()
{
    m_currentState = RECEIVE_DATA;

    if (m_length > PICO_PROTOCOL_DATA_MAX_LENGTH)
    {
		m_ec = make_system_error(EBADMSG);
		m_nextState = ERROR;
		return;
    }

    memset(m_rxBuffer, 0, PICO_PROTOCOL_BUFFER_SIZE);

    int received = spi_read_blocking(m_spi, 0, m_rxBuffer, m_length);

    if (received != m_length)
    {
        m_ec = make_system_error(EPROTO);
        m_nextState = ERROR;
        return;
    }
	uint8_t crc = calculate_crc(m_rxBuffer, m_length);
	if (crc != m_crc)
	{
		m_ec = make_system_error(EBADMSG);
		m_nextState = ERROR;
		return;
	}
    m_nextState = HANDLE_REQUEST;
}

void PicoProtocol::handle_request()
{
    m_currentState = HANDLE_REQUEST;

	const std::string request = reinterpret_cast<const char *>(m_rxBuffer);

    if (m_callback)
    {
        m_callback(request, m_answer, m_ec);
        if (m_ec.value())
        {
		    m_nextState = ERROR;
		    return;            
        }
    }
    else
        m_answer = request;

    m_nextState = MAKE_ANSWER;
}
	
void PicoProtocol::make_answer()
{
    m_currentState = MAKE_ANSWER;

	if (m_answer.length() > PICO_PROTOCOL_DATA_MAX_LENGTH)
	{
		m_ec = make_system_error(EMSGSIZE);
		m_nextState = ERROR;
		return;
	}

    memset(m_txBuffer, 0, PICO_PROTOCOL_BUFFER_SIZE);

	m_txBuffer[0] = m_signature[0];
	m_txBuffer[1] = m_signature[1];
	m_txBuffer[2] = m_answer.length();

 	memcpy(&m_txBuffer[4], reinterpret_cast<const uint8_t *>(m_answer.data()), m_answer.length());
    m_length = PICO_PROTOCOL_HEADER_LENGTH + m_answer.length();

	m_txBuffer[3] = calculate_crc(&m_txBuffer[4], m_answer.length());
	m_nextState = SEND_ANSWER;
}

void PicoProtocol::send_answer()
{
    m_currentState = SEND_ANSWER;
    printf("m_length = %d\n", m_length);
    printf("m_txBuffer = %s\n", m_txBuffer);
    int transmitted = spi_write_blocking (m_spi, static_cast<const uint8_t *>(m_txBuffer), m_length);
    printf("OK\n");
    if (transmitted != m_length)
    {
        m_ec = make_system_error(EPROTO);
        m_nextState = ERROR;
        return;     
    }
    m_nextState = COMPLETE;
}

void PicoProtocol::error()
{
   m_currentState = ERROR;
}

void PicoProtocol::complete()
{
   m_currentState = COMPLETE;
}
