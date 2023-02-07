#ifndef _DHT11_H
#define _DHT11_h
#include "sensor.h"
#include "sensor_error.h"

namespace sensors
{

class Dht11 : public Sensor
{
public:
	Dht11(int dataPin)
	: Sensor("DHT11 Temperature/Humidity Sensor", DHT11),
	  m_dataPin{dataPin},
	  m_data{0,}
	{}
	~Dht11() override
	{}

	//void init(SensorSet &collection, std::error_code &ec); 

private:
	void init(std::error_code &ec) override;	
	void handler(
			const coap::UriPath &uriPath,
			std::vector<coap::SenmlJsonType> *in,
			std::vector<coap::SenmlJsonType> *out,
			std::error_code &ec) override;

private:
	inline bool is_data_correct();
	inline bool wait_while_status(size_t usTimeout, bool initStatus);
	inline bool start_condition();
	inline bool read_acknowledge();
	inline bool read_data_byte(uint8_t &byte);


private:
	SensorStatus read_data();

private:
	int m_dataPin;
	static const size_t s_dataLen = 5;
	uint8_t m_data[s_dataLen];
};

} //namespace sensors

#endif
