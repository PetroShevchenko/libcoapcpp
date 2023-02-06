#ifndef _DHT11_H
#define _DHT11_h
#include "sensor.h"

namespace sensors
{

class Dht11 : public Sensor
{
public:
	Dht11(int dataPin)
	: Sensor("DHT11 Temperature/Humidity Sensor", DHT11),
	  m_dataPin{dataPin},
	  m_value{0,}
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
	bool read_value();

private:
	int m_dataPin;
	const size_t dataLen = 5; 
	uint8_t m_value[dataLen];
};

} //namespace sensors

#endif
