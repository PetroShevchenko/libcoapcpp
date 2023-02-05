#include "senml_json.h"
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

static double get_timestamp()
{
    time_t currentTime = time(nullptr);
    return static_cast<double>(mktime(localtime(&currentTime)));
}

TEST(testSenmlJson, createJson)
{
    error_code ec;
    SenmlJson parser;
{
	SenmlJsonType record("temperature", "Cel", SenmlJsonType::Value(25.5), get_timestamp());
    parser.add_record(record);
}

{
	SenmlJsonType record("humidity", "%RH", SenmlJsonType::Value(30.0), get_timestamp());
    parser.add_record(record);
}

	parser.create_json(ec);
	EXPECT_EQ(ec.value(), 0);

#ifdef PRINT_TESTED_VALUES
	info("{}", parser.json());
#endif

	parser.clear_payload();

{
	SenmlJsonType record("light", "%", SenmlJsonType::Value(100.0), get_timestamp());
    parser.add_record(record);
}

{
	SenmlJsonType record("test_str", "chars", SenmlJsonType::Value("text1"), get_timestamp());
    parser.add_record(record);
}

{
	SenmlJsonType record("test_bool", "boolean", SenmlJsonType::Value(true), get_timestamp());
    parser.add_record(record);
}

{
	const std::vector<uint8_t> text2 = {'A','B','C','D','E','F'};
	SenmlJsonType record("test_data", "Base64", SenmlJsonType::Value(text2), get_timestamp());
    parser.add_record(record);
}

	parser.create_json(ec);
	EXPECT_EQ(ec.value(), 0);

#ifdef PRINT_TESTED_VALUES
	if (ec.value())
		info("Error message: {}",ec.message());
	else
		info("{}", parser.json());
#endif
}

TEST(testSenmlJson, parseJson)
{
    error_code ec;
    SenmlJson parser;

	const char *json = 
	"["\
		"{\"bn\":\"urn:dev:ow:10e2073a0108006:\",\"bt\":1.276020076001e+09,"\
		"\"bu\":\"A\",\"bver\":5,"\
		"\"n\":\"voltage\",\"u\":\"V\",\"v\":120.1},"\
		"{\"n\":\"current\",\"t\":-5,\"v\":1.2},"\
		"{\"n\":\"current\",\"t\":-4,\"v\":1.3},"\
		"{\"n\":\"current\",\"t\":-3,\"v\":1.4},"\
		"{\"n\":\"current\",\"t\":-2,\"v\":1.5},"\
		"{\"n\":\"current\",\"t\":-1,\"v\":1.6},"\
		"{\"n\":\"current\",\"v\":1.7}"\
	"]";

	parser.parse_json(json, ec);

#ifdef PRINT_TESTED_VALUES
	if (ec.value())
		info("Error message: {}",ec.message());
	else
	{
		info("=============");
		info("Parsed JSON :");
		info("=============");
		info("base name: {}",parser.base_name().c_str());
		info("base unit: {}",parser.base_unit().c_str());
		info("base time: {0:f}",parser.base_time());
		info("base value: {0:f}",parser.base_value());
		info("base sum: {0:f}",parser.base_sum());
		info("base version: {0:d}",parser.base_version());

		size_t i = 0;
		for(auto record : parser.payload())
		{
			info("=============");
			info("Record {0:d}:", ++i);
			info("=============");
			info("name: {}", record.name.c_str());
			info("unit: {}", record.unit.c_str());
			info("value.type: {0:d}", record.value.type);
			info("value.asNumber: {0:f}", record.value.asNumber);
			info("value.asString: {}", record.value.asString.c_str());
			info("value.asData: {}", to_hex(record.value.asData.data(), record.value.asData.data() + record.value.asData.size()));
			info("value.asBoolean: {0:b}", record.value.asBoolean);
			info("sum: {0:f}", record.sum);
			info("time: {0:f}", record.time);
			info("updateTime: {0:f}", record.updateTime);
		}
	}
#endif

	EXPECT_EQ(ec.value(), 0);
}

