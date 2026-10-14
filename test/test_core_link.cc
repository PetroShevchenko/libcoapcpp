#include "core_link.h"
#include "test_common.h"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <cstdint>
#include <cstring>
#include <ctime>

using namespace std;
using namespace coap;
using namespace spdlog;


TEST(testCoreLink, createCoreLink)
{
    error_code ec;
    CoreLink parser;
{
	CoreLinkType record("sensors/temp", ec);
	EXPECT_EQ(ec.value(), 0);
	{
		CoreLinkParameter::Value value("temperature-c");
		CoreLinkParameter param("rt", move(value));
		record.parameters.push_back(move(param));
	}
	{
		CoreLinkParameter::Value value("sensor");
		CoreLinkParameter param("if", move(value));
		record.parameters.push_back(move(param));
	}
    parser.add_record(move(record));
}
{
	CoreLinkType record("sensors/light", ec);
	EXPECT_EQ(ec.value(), 0);
	{
		CoreLinkParameter::Value value("light-lux");
		CoreLinkParameter param("rt", move(value));
		record.parameters.push_back(move(param));
	}
	{
		CoreLinkParameter::Value value("sensor");
		CoreLinkParameter param("if", move(value));
		record.parameters.push_back(move(param));
	}
    parser.add_record(move(record));
}

#ifdef PRINT_TESTED_VALUES
	for(vector<CoreLinkType>::const_iterator
				iter=parser.payload().begin(),
				end=parser.payload().end(); iter != end; ++iter)
	{
		info("{}", iter->uri.path().c_str());
	}
#endif

	parser.create_core_link(ec);
	EXPECT_EQ(ec.value(), 0);

#ifdef PRINT_TESTED_VALUES
	info("{}", parser.core_link().c_str());
#endif

}

TEST(testCoreLink, parseCoreLink)
{
    error_code ec;
    CoreLink parser;

    const char *coreLink =
    "</sensors>;ct=40;title=\"Sensor Index\","\
    "</sensors/temp>;rt=\"temperature-c\";if=\"sensor\","\
    "</sensors/light>;rt=\"light-lux\";if=\"sensor\","\
	"<coap://192.168.0.104/sensors/DHT11/t>;anchor=\"/sensors/temp\";rel=\"describedby\","\
    "</t>;anchor=\"/sensors/temp\";rel=\"alternate\"";

	parser.parse_core_link(coreLink, ec);
	EXPECT_EQ(ec.value(), 0);

#ifdef PRINT_TESTED_VALUES
	info("Records: {0:d}", parser.payload().size());
	for(vector<CoreLinkType>::const_iterator
				iter=parser.payload().begin(),
				end=parser.payload().end(); iter != end; ++iter)
	{
		info("Record: {}", iter->uri.path().c_str());

		for(vector<CoreLinkParameter>::const_iterator
				iter2=iter->parameters.begin(),
				end2=iter->parameters.end(); iter2 != end2; ++iter2)
		{
			info("name: {}", iter2->name.c_str());

			if (iter2->value.type == CoreLinkParameter::NUMBER)
			{
				info("type: NUMBER");
				info("value: {0:d}", iter2->value.asNumber);
			}
			else
			{
				info("type: STRING");
				info("value: {}", iter2->value.asString.c_str());
			}
		}
	}
#endif

}

static vector<uint8_t> to_bytes(const char *text)
{ return vector<uint8_t>(text, text + strlen(text)); }

TEST(testCoreLink, rejectUnterminatedString)
{
    /*
        There are 3 payloads for testing

        Every payload has a quoted string that is not closed, so the parser must not
        split the payload into records or accept the string:
        </a>;title="x           The string runs to the end of the payload
        </a>;title="x,</b>      The record separator is inside the string, not between two records
        </a>;title="x\"         The last quote is escaped, so it does not close the string
    */
    const vector<TestCase> cases = {
        {"string to the end of payload", to_bytes("</a>;title=\"x"),       CoapStatus::COAP_ERR_PARSE_CORE_LINK},
        {"separator inside string",      to_bytes("</a>;title=\"x,</b>"),  CoapStatus::COAP_ERR_PARSE_CORE_LINK},
        {"escaped closing quote",        to_bytes("</a>;title=\"x\\\""),   CoapStatus::COAP_ERR_PARSE_CORE_LINK},
    };

    for (const auto & tc : cases)
    {
        error_code ec;
        CoreLink parser;

        // parse_core_link() takes a terminated C string
        const string text(tc.message.begin(), tc.message.end());

        parser.parse_core_link(text.c_str(), ec);

#ifdef PRINT_TESTED_VALUES
        print_error(tc.name, ec);
#endif

        const error_code expected = make_error_code(tc.expected);

        EXPECT_EQ(ec, expected)
            << "  TC name:  " << tc.name << "\n"
            << "  actual:   " << ec.message() << "\n"
            << "  expected: " << expected.message();
    }
}
