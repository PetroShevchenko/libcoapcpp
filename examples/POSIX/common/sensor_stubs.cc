#include "sensor_stubs.h"
#include "trace.h"

using namespace posix;

namespace sensors
{

void DHT11Simulator::init(std::error_code &ec)
{
	ENTER_TRACE();
	TRACE("*****************************************\n");
	TRACE("* DHT11 Sensor Stub initialization      *\n");
	TRACE("*****************************************\n");
	ec.clear();
	EXIT_TRACE();
}

void DHT11Simulator::handler(const void *in, void *out, std::error_code &ec)
{
	ENTER_TRACE();
	(void)in;
	TRACE("*****************************************\n");
	TRACE("*      DHT11 Sensor Stub                *\n");
	TRACE("* (temperature and humidity)            *\n");
	TRACE("*****************************************\n");	
	if (out == NULL)
	{
		ec = make_system_error(EFAULT);
		EXIT_TRACE();
		return;
	}
	Results *results = (Results *)out;
	TRACE("DHT11 Sensor Stub : TEMPERATURE: 25 C\n");
	results->temperature = 25;
	TRACE("DHT11 Sensor Stub : HUMIDITY: 40 %\n");
	results->humidity = 40;
	EXIT_TRACE();
} 

} //namespace sensors
