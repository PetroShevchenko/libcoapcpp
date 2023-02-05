#ifndef _SENSOR_STUBS_H
#define _SENSOR_STUBS_H
#include "sensor.h"

namespace sensors
{

class DHT11_Simulator : public Sensor
{
public:
	DHT11_Simulator()
	: Sensor("DHT11_Simulator", DHT11)
	{}
	~DHT11_Simulator() override
	{}

	//void init(SensorSet &collection, std::error_code &ec); 

private:
	void init(std::error_code &ec) override;	
	void handler(
			const coap::UriPath &uriPath,
			std::vector<coap::SenmlJsonType> *in,
			std::vector<coap::SenmlJsonType> *out,
			std::error_code &ec) override;
};

class RGB_LED_Simulator : public Sensor
{
public:
	RGB_LED_Simulator()
	: Sensor("RGB_LED_Simulator", RGB_LED),
	  m_redLight{0},
	  m_greenLight{0},
	  m_blueLight{0}
	{}
	~RGB_LED_Simulator() override
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
	uint8_t m_redLight;
	uint8_t m_greenLight;
	uint8_t m_blueLight;
};

} //namespace sensors 

#endif
