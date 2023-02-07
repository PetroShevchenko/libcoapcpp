#ifndef _SENSOR_ERROR_H
#define _SENSOR_ERROR_H
#include <system_error>

//typedef int SensorStatus;
enum SensorStatus
{
	SENSOR_OK,
	SENSOR_ERR_DHT11_START_CONDITION,
	SENSOR_ERR_DHT11_READ_ACK,
	SENSOR_ERR_DHT11_READ_DATA,
	SENSOR_ERR_DHT11_CRC,
	SENSOR_ERR_WIRING_PI_SETUP,
};

namespace std
{
template<> struct is_error_condition_enum<SensorStatus> : public true_type {};
}

std::error_code make_error_code (SensorStatus e);

#endif
