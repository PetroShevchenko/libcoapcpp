#ifndef _ENDPOINT_H
#define _ENDPOINT_H
#include <string>
#include <ctime>
#include "error.h"

namespace coap
{

class Endpoint {

public:
	Endpoint (const char *name)
	  : m_name{name}
	{}

	virtual ~Endpoint() = default;

	Endpoint(const Endpoint &) = delete;
	Endpoint & operator=(const Endpoint &) = delete;

public:
	virtual void registration_step(std::error_code &ec) = 0;
	virtual void transaction_step(std::error_code &ec) = 0;

	const std::string &name() const
	{ return m_name; }

protected:
	std::string 	m_name;
};

}// namespace coap
#endif
