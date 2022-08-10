#ifndef RPI400_SPI_H 
#define RPI400_SPI_H
#include <cstdint>
#include <string>
#include "error.h"
#include "buffer.h"

#define RPI400_SPI_TX_BUF_SIZE 256U
#define RPI400_SPI_RX_BUF_SIZE RPI400_SPI_TX_BUF_SIZE

class Spi
{
public:
	Spi(const char *device, uint32_t speed, uint8_t mode, uint8_t bits, std::error_code &ec);
	Spi(std::error_code &ec);
	~Spi();

public:
	uint32_t speed() const
	{ return m_speed; }

	void speed(uint32_t value)
	{ m_speed = value; }

	uint8_t mode() const
	{ return m_mode; }

	void mode(uint8_t value)
	{ m_mode = value; }

	uint8_t bits() const
	{ return m_bits; }

	void bits(uint8_t value)
	{ m_bits = value; }

public:
	void init(std::error_code &ec);

	Buffer& write_read_blocking(const Buffer &src, std::error_code &ec);
	void write_read_blocking(const uint8_t *src, uint8_t *dst, size_t len, std::error_code &ec);

	Buffer& read_blocking(uint8_t repeated_tx_data, size_t len, std::error_code &ec);
	void read_blocking(uint8_t repeated_tx_data, uint8_t *dst, size_t len, std::error_code &ec);

	void write_blocking(const Buffer &src, std::error_code &ec);
	void write_blocking(const uint8_t *src, size_t len, std::error_code &ec);

private:
	void write_read_blocking(std::error_code &ec);

private:
	std::string m_device;
	uint32_t m_speed;
	uint8_t m_mode;
	uint8_t m_bits;
	int m_fd;
	Buffer m_txBuf;
	Buffer m_rxBuf;
};


#endif // RPI400_SPI_H
