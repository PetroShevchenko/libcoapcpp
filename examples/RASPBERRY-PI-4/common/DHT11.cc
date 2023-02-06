#include <wiringPi.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "DHT11.h"
#include "trace.h"
#include "timestamp.h"

using namespace posix;
using namespace coap;
using namespace std;

namespace sensors
{

void Dht11::init(std::error_code &ec)
{
	ENTER_TRACE();
	TRACE("*****************************************\n");
	TRACE("* DHT11 Sensor initialization           *\n");
	TRACE("*****************************************\n");
 	if (wiringPiSetup()==-1)
 	{
 		ec = make_system_error(EFAULT);
 		EXIT_TRACE();
 		return;	
 	}
	EXIT_TRACE();
}

void Dht11::handler(
			const coap::UriPath &uriPath,
			std::vector<coap::SenmlJsonType> *in,
			std::vector<coap::SenmlJsonType> *out,
			std::error_code &ec
		)
{
	ENTER_TRACE();
	(void)in;
	TRACE("*****************************************\n");
	TRACE("*      DHT11 Sensor                     *\n");
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
		ec = make_error_code(CoapStatus::COAP_ERR_NOT_FOUND);
		EXIT_TRACE();
		return;
	}
	if (!read_value())
	{
		ec = make_system_error(EFAULT);
		EXIT_TRACE();
		return;
	}
	if (endpoint == "temp")
	{
		SenmlJsonType record(
							"temperature",
							"Cel",
							SenmlJsonType::Value((double)m_value[2]),
							get_timestamp()
						);
		out->push_back(move(record));
		TRACE("DHT11 Sensor : TEMPERATURE:", (double)m_value[2], " C\n");
	}
	else
	{
		SenmlJsonType record(
							"humidity",
							"%RH",
							SenmlJsonType::Value((double)m_value[0]),
							get_timestamp()
						);
		out->push_back(move(record));
		TRACE("DHT11 Sensor : HUMIDITY: ", (double)m_value[0], " %\n");
	}
	EXIT_TRACE();
}
#define MAX_TIME 85
bool Dht11::read_value()
{
	ENTER_TRACE();
    uint8_t lastState = HIGH;         //last state
    uint8_t counter=0;
    size_t j = 0, i;

	memset(m_value, 0, s_dataLen);

    //host send start signal    
    pinMode(m_dataPin, OUTPUT);      //set pin to output 
    digitalWrite(m_dataPin, LOW);    //set to low at least 18ms 
    delay(18);
    digitalWrite(m_dataPin, HIGH);   //set to high 20-40us
    delayMicroseconds(40);
     
    //start recieve dht response
    pinMode(m_dataPin, INPUT);       //set pin to input
    for(i = 0; i < MAX_TIME; i++)         
    {
        counter=0;
        while(digitalRead(m_dataPin) == lastState)
        {  //read pin state to see if dht responsed. if dht always high for 255 + 1 times, break this while circle
            counter++;
            delayMicroseconds(1);
            if(counter==255)
                break;
        }
        lastState = digitalRead(m_dataPin);   //read current state and store as last state. 
        if(counter==255)         	//if dht always high for 255 + 1 times, break this for circle
            break;
        // top 3 transistions are ignored, maybe aim to wait for dht finish response signal
        if((i >= 4) && ( i % 2 == 0))
        {
            m_value[j/8] <<= 1;		//write 1 bit to 0 by moving left (auto add 0)
            if(counter > 16)		//long mean 1
                m_value[j/8] |= 1;	//write 1 bit to 1 
            j++;
        }
    }
    // verify checksum and print the verified data
    if((j >= 40) && (m_value[4] == ((m_value[0] + m_value[1] + m_value[2] + m_value[3]) & 0xFF)))
    {
        TRACE("RH: ", m_value[0], " TEMP: ",  m_value[2], "\n");
        EXIT_TRACE();
        return true;
    }
    else
	{
		EXIT_TRACE();
	    return false;
	}
}

} // namespace sensors
