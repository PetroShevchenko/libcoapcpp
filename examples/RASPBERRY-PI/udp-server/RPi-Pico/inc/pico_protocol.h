#ifndef PICO_PROTOCOL_H
#define PICO_PROTOCOL_H
#include <cstdint>
#include <string>
#include <memory>
#include "pico/stdlib.h"
#include "hardware/spi.h"
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
#define PICO_PROTOCOL_BUFFER_SIZE 255U

typedef void (*RequestHandlerCallback)(const std::string &inBuffer, std::string &outBuffer, std::error_code &ec);

class PicoProtocol
{

public:
	enum State {
		IDLE = 0,
		RECEIVE_HEADER,
		RECEIVE_DATA,
        HANDLE_REQUEST,
        MAKE_ANSWER,
        SEND_ANSWER,
		ERROR,
		COMPLETE,
	};

public:
	PicoProtocol(spi_inst_t * spi, RequestHandlerCallback callback)
	: m_spi{spi},
      m_callback{callback},
	  m_currentState{IDLE},
	  m_nextState{IDLE},
	  m_length{0},
	  m_crc{0},
	  m_answer{},
	  m_ec{},
	  m_txBuffer{},
      m_rxBuffer{}
	{}

	~PicoProtocol() = default;

public:
    void process(std::error_code &ec);

public:
	const std::error_code & ec() const
	{ return static_cast<const std::error_code &>(m_ec); }

private:
	static uint8_t calculate_crc(uint8_t * data, size_t len);

	void receive_header();	
	void receive_data();
	void handle_request();
	void make_answer();
    void send_answer();
	void error();
	void complete();

private:
	spi_inst_t *m_spi;
    RequestHandlerCallback m_callback;
	State m_currentState;
	State m_nextState;
	const uint8_t m_signature[2] = {0xA5, 0x5A};
	uint8_t m_length;
	uint8_t m_crc;
	std::string m_answer;
	std::error_code m_ec;
	uint8_t m_txBuffer[255];
    uint8_t m_rxBuffer[255];
};

#endif // PICO_PROTOCOL_H
