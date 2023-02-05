#include "endpoint.h"
#include <algorithm>
#include <vector>
#include <iterator>

using namespace std;
using namespace coap;

namespace sensors
{

Endpoint &Endpoint::operator=(const Endpoint &other)
{
	if (this != &other)
	{
		m_path = other.m_path;
		m_type = other.m_type;
		m_sensorSet = other.m_sensorSet;
	}
	return *this;
}

Endpoint &Endpoint::operator=(Endpoint &&other)
{
	if (this != &other)
	{
		m_path = std::move(other.m_path);
		std::swap(m_type, other.m_type);
		std::swap(m_sensorSet, other.m_sensorSet);
	}
	return *this;		
}

EndpointPool::EndpointPool(const std::vector<EndpointType> &endpoints)
{
	for (std::vector<EndpointType>::const_iterator iter = endpoints.begin(), end = endpoints.end(); iter != end; ++iter )
		m_endpoints.push_back(std::move(Endpoint(*iter)));
}

void EndpointPool::add_endpoint(const EndpointType &endpoint, std::error_code &ec)
{
	if (endpoint.sensorSet == nullptr)
	{
		ec = make_system_error(EFAULT);
		return;	
	}
	if (!is_sensor_type_correct(endpoint.type)
		|| endpoint.path.empty())
	{
		ec = make_system_error(EINVAL);
		return;
	}
	m_endpoints.push_back(std::move(Endpoint(endpoint)));
}

void EndpointPool::compare_endpoints(const coap::CoreLink &parser, std::error_code &ec)
{
    // Check if the URIs from the Core-Link file match the ones from the served endpoints
    for (vector<CoreLinkType>::const_iterator corelink_iter = parser.payload().begin(),
            corelink_end = parser.payload().end(); corelink_iter != corelink_end; ++corelink_iter)
    {
        std::vector<CoreLinkParameter>::const_iterator attribute_iterator;
        attribute_iterator = core_link::find_attribute("if", *corelink_iter);

        if (attribute_iterator != corelink_iter->parameters.end()
            && core_link::is_attribute_matched("if", "sensor", *attribute_iterator))
        {
            std::vector<Endpoint>::const_iterator endpoint_iter, endpoint_end;
            for(endpoint_iter = endpoints().begin(),
                endpoint_end = endpoints().end(); endpoint_iter != endpoint_end; ++endpoint_iter)
            {
                if (endpoint_iter->is_path_matched(corelink_iter->uri.path()))
                    break;
            }
            if (endpoint_iter == endpoint_end)
            {
                // there is no the appropriate endpoint for the Core-Link record
                ec = make_error_code(CoapStatus::COAP_ERR_NO_ENDPOINT);
                return;
            }
        }
    }
    ec.clear(); // the Core-Link records correspond to the endpoints
}

} //namespace sensors 
