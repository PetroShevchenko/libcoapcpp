#include <wiringPi.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "DHT11.h"
#include "trace.h"
#include "timestamp.h"

using namespace posix;
using namespace coap;
using namespace std;

enum DhtDelay {
	DHT11_DELAY_STEP_USEC = 1,
	DHT11_START_DELAY_MSEC = 20,
	DHT11_START_WAIT_RESP_MAX_USEC = 40,
	DHT11_ACK_LOW_STATE_USEC = 80,
	DHT11_ACK_HIGHT_STATE_USEC = 80,
	DHT11_DATA_START_USEC = 50,
	DHT11_DATA_READ_LOW_MAX_USEC = 28,
	DHT11_DATA_READ_HIGHT_USEC = 70,
};

namespace sensors
{

void Dht11::init(std::error_code &ec)
{
	ENTER_TRACE();
	TRACE("*****************************************\n");
	TRACE("* DHT11 Sensor initialization           *\n");
	TRACE("*****************************************\n");
	if (wiringPiSetup()==-1)
	{
		ec = make_error_code(SensorStatus::SENSOR_ERR_WIRING_PI_SETUP);
		EXIT_TRACE();
		return;	
	}
	pinMode(m_dataPin, OUTPUT);
	pullUpDnControl(m_dataPin, PUD_DOWN);
	digitalWrite(m_dataPin, HIGH);

	EXIT_TRACE();
}

void Dht11::handler(
			const coap::UriPath &uriPath,
			std::vector<coap::SenmlJsonType> *in,
			std::vector<coap::SenmlJsonType> *out,
			std::error_code &ec
		)
{
	ENTER_TRACE();
	(void)in;
	TRACE("*****************************************\n");
	TRACE("*      DHT11 Sensor                     *\n");
	TRACE("* (temperature and humidity)            *\n");
	TRACE("*****************************************\n");	
	if (out == nullptr)
	{
		ec = make_system_error(EFAULT);
		EXIT_TRACE();
		return;
	}
	TRACE("Uri-Path: ", uriPath.path(), "\n");
	size_t size = uriPath.uri().asString().size();
	if (size == 0)
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
		EXIT_TRACE();
		return;
	}
	std::string endpoint = uriPath.uri().asString()[size - 1];

	if (endpoint != "temp"
		&& endpoint != "hum")
	{
		ec = make_error_code(CoapStatus::COAP_ERR_NOT_FOUND);
		EXIT_TRACE();
		return;
	}
	SensorStatus status = read_data();
	if (status != SENSOR_OK)
	{
		ec = make_error_code(status);
		EXIT_TRACE();
		return;
	}
	if (endpoint == "temp")
	{
		SenmlJsonType record(
							"temperature",
							"Cel",
							SenmlJsonType::Value((double)m_data[2]),
							get_timestamp()
						);
		out->push_back(move(record));
		TRACE("DHT11 Sensor : TEMPERATURE:", (double)m_data[2], " C\n");
	}
	else
	{
		SenmlJsonType record(
							"humidity",
							"%RH",
							SenmlJsonType::Value((double)m_data[0]),
							get_timestamp()
						);
		out->push_back(move(record));
		TRACE("DHT11 Sensor : HUMIDITY: ", (double)m_data[0], " %\n");
	}
	EXIT_TRACE();
}

inline bool Dht11::is_data_correct()
{
	uint8_t crc = 0;
	const size_t crcIndex = s_dataLen - 1;
	for(size_t i = 0; i < crcIndex; i++)
		crc += m_data[i];
	return (crc == m_data[crcIndex]);
}

inline bool Dht11::wait_while_status(size_t usTimeout, bool initStatus)
{
	size_t counter = usTimeout / DHT11_DELAY_STEP_USEC;
	bool status = initStatus;
	do {
		delayMicroseconds(DHT11_DELAY_STEP_USEC);
		status = (digitalRead(m_dataPin) == HIGH);
	} while((status == initStatus) && --counter);
	return status;
}

/* The first DHT11's state is START CONDITION */
inline bool Dht11::start_condition()
{
	pinMode(m_dataPin, OUTPUT);
	pullUpDnControl(m_dataPin, PUD_DOWN);
	digitalWrite(m_dataPin, LOW);
	delay(DHT11_START_DELAY_MSEC);
	digitalWrite(m_dataPin, HIGH);
	pinMode(m_dataPin, INPUT);
	return (wait_while_status(DHT11_START_WAIT_RESP_MAX_USEC, true) == false);
}

/* The second DHT11's state is READ ACKNOWLEDGE */
inline bool Dht11::read_acknowledge()
{
	wait_while_status(DHT11_ACK_LOW_STATE_USEC, false);
	return (wait_while_status(DHT11_ACK_HIGHT_STATE_USEC, true) == false);
}

/* The third DHT11's state is READ DATA BYTES */
inline bool Dht11::read_data_byte(uint8_t &byte)
{
	byte = 0;
	for(int i = 7 ; i >= 0; i--)
	{
		wait_while_status(DHT11_DATA_START_USEC, false);
		if (wait_while_status(DHT11_DATA_READ_LOW_MAX_USEC, true) == false)
			continue;
		if (wait_while_status(DHT11_DATA_READ_HIGHT_USEC, true) == false)
			byte |= (1 << i);
		else
			return false;
	}
	return true;
}

inline SensorStatus Dht11::read_data()
{
	if (!start_condition())
		return SENSOR_ERR_DHT11_START_CONDITION;
	if (!read_acknowledge())
		return SENSOR_ERR_DHT11_READ_ACK;
	for (size_t i = 0; i < s_dataLen; i++)
	{
		if (!read_data_byte (m_data[i]))
			return SENSOR_ERR_DHT11_READ_DATA;
		delayMicroseconds(DHT11_DELAY_STEP_USEC);
	}
	if (!is_data_correct())
		return SENSOR_ERR_DHT11_CRC;
	return SENSOR_OK;
}

} // namespace sensors
