#ifndef _ENDPOINT_H
#define _ENDPOINT_H
#include "sensor.h"
#include "core_link.h"
#include <string>
#include <cstring>

namespace sensors
{

struct EndpointType
{
    std::string path;
    SensorType type;
    const SensorSet *sensorSet;    
};

class Endpoint
{
public:
	Endpoint(
			const char *path,
			SensorType type,
			const SensorSet &ss
		)
	: m_path{path},
	  m_type{type},
	  m_sensorSet{&ss}
	{}
    Endpoint(
            const EndpointType &ep
        )
    : m_path{ep.path},
      m_type{ep.type},
      m_sensorSet{ep.sensorSet}
    {}
	~Endpoint()
	{}

	Endpoint &operator=(const Endpoint &other);
	Endpoint &operator=(Endpoint &&other);

	Endpoint(const Endpoint &other)
	{ operator=(other); }

	Endpoint(Endpoint &&other)
	{ operator=(other); }

public:
	bool is_path_matched(const char *path) const
	{ return !strcmp(path, m_path.c_str()); }

	bool is_path_matched(const std::string &path) const
	{ return (path == m_path); }

	SensorType type() const
	{ return m_type; }

	const SensorSet *sensor_set() const
	{ return m_sensorSet; }

private:
	std::string 	m_path;
	SensorType 		m_type;
	const SensorSet *m_sensorSet;
};

class EndpointPool
{
public:
	EndpointPool()
	: m_endpoints{}
	{}
	EndpointPool(const std::vector<EndpointType> &endpoints);
	~EndpointPool()
	{}

public:
	std::vector<Endpoint> &endpoints()
	{ return m_endpoints; } 

	void clear()
	{ m_endpoints.clear(); }

	void add_endpoint(const EndpointType &endpoint, std::error_code &ec);
	void compare_endpoints(const coap::CoreLink &parser, std::error_code &ec);

private:
	std::vector<Endpoint> m_endpoints;
};

}// namespace sensors

#endif
