#ifndef _SENSOR_H
#define _SENSOR_H
#include "error.h"
#include <vector>
#include <string>

namespace sensors
{

enum SensorType
{
	TEMPERATURE,
	HUMIDITY,
	DOUBLE_TEMP_HUM,
	AXELEROMETER,
	RGB_LED,
	SENSOR_QTY
};
#define SENSOR_TYPE_MIN TEMPERATURE
#define is_sensor_type_correct(type) (type < SENSOR_QTY && type >= SENSOR_TYPE_MIN)

const char *sensor_type_to_string(SensorType type);

class SensorSet;

class Sensor
{
public:
	Sensor(const char *name, SensorType type)
	: m_name{name},
	  m_type{type}
	{}
	virtual ~Sensor()
	{}

public:
	void bind(SensorSet &collection, std::error_code &ec);

public:
	const char *name() const
	{ return m_name.c_str(); }

	SensorType type() const
	{ return m_type; }

protected:
	virtual void init(std::error_code &ec) = 0;
	virtual void handler(const void *in, void *out, std::error_code &ec) = 0;

private:
	static void handler(void *object, const void *in, void *out, std::error_code &ec);

private:
	std::string m_name;
	SensorType 	m_type;
};

typedef void (*sensor_handler_ptr)(void *object, const void *in, void *out, std::error_code &ec);

class SensorSet
{
public:
	SensorSet();
	~SensorSet()
	{}

public:
	void register_callback(void *object, SensorType type, sensor_handler_ptr clbk, std::error_code &ec);
	void process(SensorType type, const void *in, void *out, std::error_code &ec);

private:
	std::vector<void *> 			m_objects;
	std::vector<sensor_handler_ptr> m_callbacks;
	const size_t c_sensors = SENSOR_QTY; 
};

}// namespace sensors

#endif
