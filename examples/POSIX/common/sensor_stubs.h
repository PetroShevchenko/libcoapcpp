#ifndef _SENSOR_STUBS_H
#define _SENSOR_STUBS_H
#include "sensor.h"

namespace sensors
{

class DHT11Simulator : public Sensor
{
public:
	DHT11Simulator()
	: Sensor("DHT11_Simulator", DOUBLE_TEMP_HUM)
	{}
	~DHT11Simulator() override
	{}

	//void init(SensorSet &collection, std::error_code &ec); 

public:
	struct Results
	{
		uint8_t temperature;
		uint8_t humidity;
	};

private:
	void init(std::error_code &ec) override;	
	void handler(const void *in, void *out, std::error_code &ec) override;
};

} //namespace sensors 

#endif
