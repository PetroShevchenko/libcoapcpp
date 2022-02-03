#ifndef _SENML_JSON_H
#define _SENML_JSON_H
#include "cJSON.h"
#include "error.h"
#include <string>
#include <vector>

namespace coap
{

struct SenmlJsonType
{
	std::string name;
	std::string unit;

	enum ValueType
	{
		NUMBER,
		STRING,
		BOOLEAN,
		DATA
	};
	
	struct Value
	{
		ValueType type;
		double asNumber;
		std::string asString;
		bool asBoolean;
		std::vector<uint8_t> asData;

		Value()
		: type{NUMBER}, asNumber{0}, asString{}, asBoolean{false}, asData{}
		{}

		Value(double _asNumber)
		: type{NUMBER}, asNumber{_asNumber}, asString{}, asBoolean{false}, asData{}
		{}

		Value(const char * _asString)
		: type{STRING}, asNumber{0}, asString{_asString}, asBoolean{false}, asData{}
		{}

		Value(bool _asBoolean)
		: type{BOOLEAN}, asNumber{0}, asString{}, asBoolean{_asBoolean}, asData{}
		{}

		Value(const std::vector<uint8_t> &_asData)
		: type{DATA}, asNumber{0}, asString{}, asBoolean{false}, asData{_asData}
		{}

		Value(const Value &other)
		{
			if (this != &other)
			{
				type = other.type;
				asNumber = other.asNumber;
				asString = other.asString;
				asBoolean = other.asBoolean;
				asData = other.asData;
			}
		}

		~Value()
		{}

		Value &operator=(const Value &other)
		{
			if (this == &other)
				return *this;

			type = other.type;
			asNumber = other.asNumber;
			asString = other.asString;
			asBoolean = other.asBoolean;
			asData = other.asData;

			return *this;
		}

		void clear()
		{
			type = NUMBER;
			asNumber = 0;
			asString.clear();
			asBoolean = false;
			asData.clear();
		}
	};

	Value value;
	double sum;
	double time;
	double updateTime;

	SenmlJsonType()
	: name{}, unit{}, value{}, sum{0}, time{0}, updateTime{0}
	{}
	SenmlJsonType(
		const char *_name, 
		const char *_unit, 
		const Value &_value,
		double _time
		)
	: name{_name}, unit{_unit}, value{_value}, sum{0}, time{_time}, updateTime{0}
	{}
	~SenmlJsonType()
	{}
	void clear()
	{
		name.clear();
		unit.clear();
		value.clear();
		sum = 0;
		time = 0;
		updateTime = 0;
	}
};

class SenmlJson
{
public:
	SenmlJson()
	: m_payload{0},
	  m_json{nullptr},
	  m_baseName{},
	  m_baseUnit{},
	  m_baseTime{0},
	  m_baseValue{0},
	  m_baseSum{0},
	  m_baseVersion{0}
	{}
	~SenmlJson()
	{ if (m_json) free(m_json);	}

	void clear_payload()
	{ m_payload.clear(); }

	void clear();

	void add_record(const SenmlJsonType &record)
	{ m_payload.push_back(record); }

	void create_json(std::error_code &ec);
	void parse_json(const char *json, std::error_code &ec);

	const char *json() const
	{
		return static_cast<const char *>(m_json);
	}

	const std::vector<SenmlJsonType> &payload() const
	{ return static_cast<const std::vector<SenmlJsonType> &>(m_payload); }

	const std::string &base_name() const
	{ return static_cast<const std::string &>(m_baseName); }

	const std::string &base_unit() const
	{ return static_cast<const std::string &>(m_baseUnit); }

	double base_time() const
	{ return m_baseTime; }

	double base_value() const
	{ return m_baseValue; }

	double base_sum() const
	{ return m_baseSum; }

	int base_version() const
	{ return m_baseVersion; }

private:
	void parse_record(cJSON *node, std::error_code &ec);

private:
	std::vector<SenmlJsonType> m_payload;
	char *m_json;
	std::string m_baseName;
	std::string m_baseUnit;
	double m_baseTime;
	double m_baseValue;
	double m_baseSum;
	int m_baseVersion;
};

}// namespace coap

#endif

