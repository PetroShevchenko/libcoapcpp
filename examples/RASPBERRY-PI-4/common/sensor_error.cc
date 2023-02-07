#include "sensor_error.h"
#include <string>

static const char *sensor_status_to_string(SensorStatus status)
{
    switch(status)
    {
    case SENSOR_OK:
        return "SENSOR_OK";
    case SENSOR_ERR_DHT11_START_CONDITION:
        return "SENSOR_ERR_DHT11_START_CONDITION";
    case SENSOR_ERR_DHT11_READ_ACK:
        return "SENSOR_ERR_DHT11_READ_ACK";
    case SENSOR_ERR_DHT11_READ_DATA:
        return "SENSOR_ERR_DHT11_READ_DATA";
    case SENSOR_ERR_DHT11_CRC:
        return "SENSOR_ERR_DHT11_CRC";
    case SENSOR_ERR_WIRING_PI_SETUP:
        return "SENSOR_ERR_WIRING_PI_SETUP";
    default:
        return "UNKNOWN";
    }
}

namespace
{

struct SensorErrorCategory : public std::error_category
{
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* SensorErrorCategory::name() const noexcept
{ return "sensor"; }

std::string SensorErrorCategory::message(int ev) const
{
    return sensor_status_to_string((SensorStatus)ev);
}

const SensorErrorCategory theSensorErrorCategory {};

} // namespace

std::error_code make_error_code (SensorStatus e)
{
    return {static_cast<int>(e), theSensorErrorCategory};
}