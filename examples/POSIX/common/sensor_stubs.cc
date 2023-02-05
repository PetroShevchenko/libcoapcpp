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
	size_t size = uriPath.uri().asString().size();
	if (size == 0)
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
		EXIT_TRACE();
		return;
	}
	std::string endpoint = uriPath.uri().asString()[size - 1];

	if (endpoint != "temp"
		&& endpoint != "hum")
	{
		ec = make_system_error(CoapStatus::COAP_ERR_NOT_FOUND);
		EXIT_TRACE();
		return;
	}
	if (endpoint == "temp")
	{
		SenmlJsonType record(
							"temperature",
							"Cel",
							SenmlJsonType::Value(25.5),
							get_timestamp()
						);
		out->push_back(move(record));
		TRACE("DHT11 Sensor Stub : TEMPERATURE: 25.5 C\n");
	}
	else
	{
		SenmlJsonType record(
							"humidity",
							"%RH",
							SenmlJsonType::Value(40.0),
							get_timestamp()
						);
		out->push_back(move(record));
		TRACE("DHT11 Sensor Stub : HUMIDITY: 40.0 %\n");
	}
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
	size_t size = uriPath.uri().asString().size();
	if (size == 0)
	{
		ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
		EXIT_TRACE();
		return;
	}
	std::string endpoint = uriPath.uri().asString()[size - 1];

	if (endpoint != "red"
		&& endpoint != "green"
		&& endpoint != "blue")
	{
		ec = make_system_error(CoapStatus::COAP_ERR_NOT_FOUND);
		EXIT_TRACE();
		return;
	}

	if (in) //PUT or POST
	{
		if (in->size() != 1
			|| (*in)[0].value.type != SenmlJsonType::NUMBER
			|| (*in)[0].name != "light"
			|| (*in)[0].value.asNumber > 100)
		{
			ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
			EXIT_TRACE();
			return;
		}
		if (endpoint == "red")
		{
			m_redLight = (*in)[0].value.asNumber;
			TRACE("*** Red Light: ", (int)m_redLight, "\n");
		}
		else if (endpoint == "green")
		{
			m_greenLight = (*in)[0].value.asNumber;
			TRACE("*** Green Light: ", (int)m_greenLight, "\n");
		}
		else if (endpoint == "blue")
		{
			m_blueLight = (*in)[0].value.asNumber;
			TRACE("*** Blue Light: ", (int)m_blueLight, "\n");
		}
	}
	else if (out) //GET
	{
		coap::SenmlJsonType record;
		record.name = "light";
		record.unit = "%";
		record.time = get_timestamp();
		record.value.type = SenmlJsonType::NUMBER;

		if (endpoint == "red")
		{
			record.value.asNumber = m_redLight;
			TRACE("*** Red Light: ", (int)m_redLight, "\n");
		}
		else if (endpoint == "green")
		{
			record.value.asNumber = m_greenLight;
			TRACE("*** Green Light: ", (int)m_greenLight, "\n");
		}
		else if (endpoint == "blue")
		{
			record.value.asNumber = m_blueLight;
			TRACE("*** Blue Light: ", (int)m_blueLight, "\n");
		}
		out->push_back(record);
	}
	EXIT_TRACE();
}

} //namespace sensors
