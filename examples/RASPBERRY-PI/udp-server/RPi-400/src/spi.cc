#include "spi.h"
#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

/*
SPI mode:
	SPI_LOOP
	SPI_CPHA
	SPI_CPOL
	SPI_LSB_FIRST
	SPI_CS_HIGH
	SPI_3WIRE
	SPI_NO_CS
	SPI_READY
*/
static const char *spi_device_default = "/dev/spidev0.0"; 
static const uint32_t spi_speed_default = 1000000;// 1 MHz
static const uint8_t spi_mode_default = (SPI_CPHA | SPI_CPOL); // CPHA=1, CPOL=1
static const uint8_t spi_bits_default = 8; // 8 bit data 

Spi::Spi(
		const char *device,
		uint32_t speed,
		uint8_t mode,
		uint8_t bits,
		std::error_code &ec
	)
	: m_device{device},
	  m_speed{speed},
	  m_mode{mode},
	  m_bits{bits},
	  m_fd{-1},
	  m_txBuf{RPI400_SPI_TX_BUF_SIZE},
	  m_rxBuf{RPI400_SPI_RX_BUF_SIZE}
{
	m_fd = open(device, O_RDWR);
	if (m_fd < 0)
	{
		ec = make_system_error(errno);
	}
}

Spi::Spi(std::error_code &ec)
: Spi::Spi(
			spi_device_default,
			spi_speed_default,
			spi_mode_default,
			spi_bits_default,
			ec 
		)
{	
}

Spi::~Spi()
{
	close(m_fd);
}

void Spi::init(std::error_code &ec)
{
	int status;
	status = ioctl(m_fd, SPI_IOC_WR_MODE, &m_mode);
	if (status == -1) goto error_exit;

	status = ioctl(m_fd, SPI_IOC_RD_MODE, &m_mode);
	if (status == -1) goto error_exit;

	status = ioctl(m_fd, SPI_IOC_WR_BITS_PER_WORD, &m_bits);
	if (status == -1) goto error_exit;

	status = ioctl(m_fd, SPI_IOC_RD_BITS_PER_WORD, &m_bits);
	if (status == -1) goto error_exit;

	status = ioctl(m_fd, SPI_IOC_WR_MAX_SPEED_HZ, &m_speed);
	if (status == -1) goto error_exit;

	status = ioctl(m_fd, SPI_IOC_RD_MAX_SPEED_HZ, &m_speed);
	if (status == -1) goto error_exit;

	return;

error_exit:
	ec = make_system_error(errno);
}

void Spi::write_read_blocking(std::error_code &ec)
{
	struct spi_ioc_transfer transfer = {};

	m_rxBuf.clear();

	transfer.tx_buf = (unsigned long)m_txBuf.data();
	transfer.rx_buf = (unsigned long)m_rxBuf.data();
	transfer.len = m_txBuf.offset();
	transfer.speed_hz = m_speed;
	transfer.delay_usecs = 0;
	transfer.bits_per_word = m_bits;

	if (ioctl(m_fd, SPI_IOC_MESSAGE(1), &transfer) < 0)
	{
		ec = make_system_error(errno);
		return;
	}
	m_rxBuf.offset(m_txBuf.offset());
}

Buffer& Spi::write_read_blocking(const Buffer &src, std::error_code &ec)
{
	m_txBuf = std::move(src);
	write_read_blocking(ec);
	return m_rxBuf;
}

void Spi::write_read_blocking(
				const uint8_t *src,
				uint8_t *dst,
				size_t len,
				std::error_code &ec
			)
{
	if (src == nullptr)
	{
		ec = make_system_error(EFAULT);
		return;
	}
	if (len == 0 || len > m_txBuf.length() || len > m_rxBuf.length())
	{
		ec = make_system_error(EINVAL);
		return;
	}

	m_txBuf.clear();
	m_rxBuf.clear();

	memcpy(m_txBuf.data(), src, len);
	m_txBuf.offset(len);

	write_read_blocking(ec);

	if (ec.value()) return;

	if (dst)
	{
		memcpy(dst, m_rxBuf.data(), len);
	}
}

Buffer& Spi::read_blocking(uint8_t repeated_tx_data, size_t len, std::error_code &ec)
{
	if (len == 0 || len > m_txBuf.length() || len > m_rxBuf.length())
	{
		ec = make_system_error(EINVAL);
		return m_rxBuf;
	}

	memset(m_txBuf.data(), repeated_tx_data, len);
	m_txBuf.offset(len);

	write_read_blocking(ec);
	return m_rxBuf;
}

void Spi::read_blocking(uint8_t repeated_tx_data, uint8_t *dst, size_t len, std::error_code &ec)
{
	if (dst == nullptr)
	{
		ec = make_system_error(EFAULT);
		return;
	}
	if (len == 0 || len > m_txBuf.length() || len > m_rxBuf.length())
	{
		ec = make_system_error(EINVAL);
		return;
	}
	{
		uint8_t *tx_buf = new (std::nothrow) uint8_t [len];

		if (tx_buf == nullptr)
		{
			ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
			return;
		}

		memset(tx_buf, repeated_tx_data, len);

		write_read_blocking(
					(const uint8_t *)tx_buf,
					dst,
					len,
					ec
				);
		delete [] tx_buf;
	}
}

void Spi::write_blocking(const Buffer &src, std::error_code &ec)
{
	m_txBuf = src;
	write_read_blocking(ec);	
}

void Spi::write_blocking(const uint8_t *src, size_t len, std::error_code &ec)
{
	write_read_blocking(
				src,
				nullptr,
				len,
				ec
			);	
}
