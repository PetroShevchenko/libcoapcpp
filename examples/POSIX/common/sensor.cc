#include "sensor.h"
#include "trace.h"

using namespace posix;

namespace sensors
{

const char *sensor_type_to_string(SensorType type)
{
	switch(type)
	{
	case TEMPERATURE:
		return "TEMPERATURE";
	case HUMIDITY:
		return "HUMIDITY";
	case DOUBLE_TEMP_HUM:
		return "DOUBLE_TEMP_HUM";
	case AXELEROMETER:
		return "AXELEROMETER";
	case RGB_LED:
		return "RGB_LED";
	default:
		return "UNKNOWN";
	}
}

void Sensor::bind(SensorSet &collection, std::error_code &ec)
{
	ENTER_TRACE();
	collection.register_callback(this, m_type, handler, ec);
	if (ec.value())
	{
		EXIT_TRACE();
		return;
	}
	init(ec);
	EXIT_TRACE();
}

void Sensor::handler(
					Sensor *object,
					const coap::UriPath &uriPath,
					std::vector<coap::SenmlJsonType> *in,
					std::vector<coap::SenmlJsonType> *out,
					std::error_code &ec
				)
{
	ENTER_TRACE();
	object->handler(uriPath, in, out, ec);
	EXIT_TRACE();
}

SensorSet::SensorSet()
		: m_objects{},
	  	  m_callbacks{}
{
	for (size_t i = 0; i < c_sensors; ++i)
	{
		m_objects.push_back(nullptr);
		m_callbacks.push_back(nullptr);
	}
}

void SensorSet::register_callback(
						Sensor *object,
						SensorType type,
						sensor_handler_ptr clbk,
						std::error_code &ec
					)
{
	ENTER_TRACE();
	if (!is_sensor_type_correct(type))
	{
		ec = make_system_error(EINVAL);
		EXIT_TRACE();
		return;			
	}
	if (object == nullptr || clbk == nullptr)
	{
		ec = make_system_error(EFAULT);
		EXIT_TRACE();
		return;
	}
	if (m_objects[type])
		TRACE("WARNING!!! Re-initialization by another object\n");
	if (m_callbacks[type])
		TRACE("WARNING!!! Re-initialization by another callback\n");

	m_objects[type] = object;
	m_callbacks[type] = clbk;
	EXIT_TRACE();
}

void SensorSet::process(
					SensorType type,
					const coap::UriPath &uriPath,
					std::vector<coap::SenmlJsonType> *in,
					std::vector<coap::SenmlJsonType> *out,
					std::error_code &ec
				)
{
	ENTER_TRACE();
	if (!is_sensor_type_correct(type))
	{
		ec = make_system_error(EINVAL);
		EXIT_TRACE();
		return;			
	}
	TRACE("m_callbacks[", sensor_type_to_string(type), "]: ", (void *)m_callbacks[type], "\n");
	TRACE("m_objects[", sensor_type_to_string(type), "]: ", m_objects[type], "\n");	
	if (m_callbacks[type] && m_objects[type])
		m_callbacks[type](m_objects[type], uriPath, in, out, ec);
	EXIT_TRACE();
}

}// namespace sensors
