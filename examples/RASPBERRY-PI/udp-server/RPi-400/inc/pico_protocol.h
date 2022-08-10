#ifndef PICO_PROOCOL
#define PICO_PROTOCOL
#include <cstdint>
#include <string>
#include "spi.h"
#include "error.h"

/****************************************************************
 *
 * Protocol package format:
 *
 *  ----------- Header ------------ ---------- Data --------
 * |           4 bytes             |       1...251 bytes    |
 * |_______________________________|___________   __________|
 * |      |      |          |      |          /  /          | 
 * |      |      |  length  |      |         /  /           |
 * | 0xA5 | 0x5A |(up to 251| CRC  | 1 byte /  /  251 byte  |
 * |	  |	     |	 bytes) |      |        \  \            |
 * |______|______|__________|______|_________\  \___________| 
 *
 ***************************************************************/
#define PICO_PROTOCOL_HEADER_LENGTH 4U
#define PICO_PROTOCOL_DATA_MAX_LENGTH (0xFF - PICO_PROTOCOL_HEADER_LENGTH)

class PicoProtocol
{

public:
	enum State {
		IDLE = 0,
		MAKE_REQUEST,
		SEND_REQUEST,
		RECEIVE_HEADER,
		RECEIVE_DATA,
		ERROR,
		COMPLETE,
	};

public:
	PicoProtocol(std::error_code &ec)
	: m_spi{ec},
	  m_constructed{ec.value() == 0},
	  m_currentState{IDLE},
	  m_nextState{IDLE},
	  m_length{0},
	  m_crc{0},
	  m_request{},
	  m_answer{},
	  m_ec{ec},
	  m_buffer{}
	{}

	~PicoProtocol() = default;

public:
	static void transfer(void *data, const std::string &request, std::string &answer, std::error_code &ec);

	bool constructed() const
	{ return m_constructed; }

	const std::error_code & ec() const
	{ return static_cast<const std::error_code &>(m_ec); }

private:
	static uint8_t calculate_crc(uint8_t * data, size_t len);

	void make_request();	
	void send_request();
	void receive_header();
	void receive_data();
	void error();
	void complete();

private:
	Spi m_spi;
	bool m_constructed;
	State m_currentState;
	State m_nextState;
	const uint8_t m_signature[2] = {0xA5, 0x5A};
	uint8_t m_length;
	uint8_t m_crc;
	std::string m_request;
	std::string m_answer;
	std::error_code m_ec;
	std::shared_ptr<Buffer> m_buffer;
};

namespace Pico
{
	void transfer(const std::string &request, std::string &answer, std::error_code &ec);
}

#endif // PICO_PROTOCOL

