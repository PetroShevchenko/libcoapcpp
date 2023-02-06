#include <wiringPi.h>  
#include <softPwm.h>  
#include "RGB_LED.h"
#include "trace.h"
#include "timestamp.h"

using namespace posix;
using namespace coap;
using namespace std;

namespace sensors
{

void RgbLed::init(std::error_code &ec)
{
	ENTER_TRACE();
	TRACE("*****************************************\n");
	TRACE("* RGB_LED Sensor initialization         *\n");
	TRACE("*****************************************\n");
	if (wiringPiSetup() == -1)
	{
		ec = make_system_error(EFAULT);
		EXIT_TRACE();
		return;
	}

    softPwmCreate(m_redLedPin,  100, 100);
    softPwmCreate(m_greenLedPin,100, 100);
    softPwmCreate(m_blueLedPin, 100, 100);
	EXIT_TRACE();
}

void RgbLed::handler(
			const coap::UriPath &uriPath,
			std::vector<coap::SenmlJsonType> *in,
			std::vector<coap::SenmlJsonType> *out,
			std::error_code &ec
		)
{
	ENTER_TRACE();
	TRACE("***********************************************\n");
	TRACE("*      RGB_LED Sensor                         *\n");
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
			softPwmWrite(m_redLedPin,   100 - m_redLight);
			TRACE("*** Red Light: ", (int)m_redLight, "\n");
		}
		else if (endpoint == "green")
		{
			m_greenLight = (*in)[0].value.asNumber;
			softPwmWrite(m_greenLedPin,   100 - m_greenLight);
			//digitalWrite(m_greenLedPin, LOW);
			TRACE("*** Green Light: ", (int)m_greenLight, "\n");
		}
		else if (endpoint == "blue")
		{
			m_blueLight = (*in)[0].value.asNumber;
			//digitalWrite(m_blueLedPin, LOW);
			softPwmWrite(m_blueLedPin,   100 - m_blueLight);
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

