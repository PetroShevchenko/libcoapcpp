#ifndef _RGB_LED_H
#define _RGB_LED_H
#include "sensor.h"

namespace sensors
{

class RgbLed : public Sensor
{
public:
	RgbLed(int redLedPin, int greenLedPin, int blueLedPin)
	: Sensor("RGB_LED Sensor", RGB_LED),
	  m_redLedPin{redLedPin},
	  m_greenLedPin{greenLedPin},
	  m_blueLedPin{blueLedPin},
	  m_redLight{0},
	  m_greenLight{0},
	  m_blueLight{0}
	{}
	~RgbLed() override
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

private:
	int m_redLedPin;
	int m_greenLedPin;
	int m_blueLedPin;
	uint8_t m_redLight;
	uint8_t m_greenLight;
	uint8_t m_blueLight;
};

} //namespace sensors

#endif
