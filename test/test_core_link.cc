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
	for(vector<CoreLinkType>::const_iterator
				iter=parser.payload().begin(),
				end=parser.payload().end(); iter != end; ++iter)
	{
		info("{}", iter->uri.path().c_str());

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
