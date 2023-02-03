#include "sensor_stubs.h"
#include "trace.h"
#include <ctime>

using namespace posix;
using namespace coap;
using namespace std;

namespace sensors
{

static double get_timestamp()
{
    time_t currentTime = time(nullptr);
    return static_cast<double>(mktime(localtime(&currentTime)));
}

void DHT11_Simulator::init(std::error_code &ec)
{
	ENTER_TRACE();
	TRACE("*****************************************\n");
	TRACE("* DHT11 Sensor Stub initialization      *\n");
	TRACE("*****************************************\n");
	ec.clear();
	EXIT_TRACE();
}

void DHT11_Simulator::handler(
			const coap::UriPath &uriPath,
			std::vector<coap::SenmlJsonType> *in,
			std::vector<coap::SenmlJsonType> *out,
			std::error_code &ec
		)
{
	ENTER_TRACE();
	(void)in;
	TRACE("*****************************************\n");
	TRACE("*      DHT11 Sensor Stub                *\n");
	TRACE("* (temperature and humidity)            *\n");
	TRACE("*****************************************\n");	
	if (out == nullptr)
	{
		ec = make_system_error(EFAULT);
		EXIT_TRACE();
		return;
	}
	TRACE("Uri-Path: ", uriPath.path(), "\n");
{
	SenmlJsonType record(
						"temperature",
						"Cel",
						SenmlJsonType::Value(25.5),
						get_timestamp()
					);
	out->push_back(move(record));
}
	TRACE("DHT11 Sensor Stub : TEMPERATURE: 25.5 C\n");
{
	SenmlJsonType record(
						"humidity",
						"%RH",
						SenmlJsonType::Value(40.0),
						get_timestamp()
					);
	out->push_back(move(record));
}
	TRACE("DHT11 Sensor Stub : HUMIDITY: 40.0 %\n");
	EXIT_TRACE();
} 

void RGB_LED_Simulator::init(std::error_code &ec)
{
	ENTER_TRACE();
	TRACE("*****************************************\n");
	TRACE("* RGB_LED Sensor Stub initialization    *\n");
	TRACE("*****************************************\n");
	ec.clear();
	EXIT_TRACE();
}

void RGB_LED_Simulator::handler(
			const coap::UriPath &uriPath,
			std::vector<coap::SenmlJsonType> *in,
			std::vector<coap::SenmlJsonType> *out,
			std::error_code &ec
		)
{
	ENTER_TRACE();
	TRACE("***********************************************\n");
	TRACE("*      RGB_LED Sensor Stub                    *\n");
	TRACE("* (three-color LED with brightness control)   *\n");
	TRACE("***********************************************\n");	

	TRACE("Uri-Path: ", uriPath.path(), "\n");

	// TODO implement brightness control simulation

	EXIT_TRACE();
}

} //namespace sensors
