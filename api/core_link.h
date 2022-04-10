#ifndef _CORE_LINK_H
#define _CORE_LINK_H
#include "uri.h"
#include "error.h"
#include <string>
#include <vector>

namespace coap
{

struct CoreLinkParameter
{
	std::string name;

	enum ValueType
	{
		NUMBER,
		STRING,
	};

	struct Value
	{
		ValueType type;
		unsigned int asNumber;
		std::string asString;

		Value()
		: type{NUMBER}, asNumber{0}, asString{}
		{}

		Value(unsigned int _asNumber)
		: type{NUMBER}, asNumber{_asNumber}, asString{}
		{}

		Value(const char *_asString)
		: type{STRING}, asNumber{0}, asString{_asString}
		{}

		Value(Value &&other)
		{
			if (this != &other)
			{
				std::swap(type, other.type);
				std::swap(asNumber, other.asNumber);
				asString = std::move(other.asString);
			}
		}

		~Value()
		{}

		Value &operator=(Value &&other)
		{
			if (this == &other)
				return *this;

			std::swap(type, other.type);
			std::swap(asNumber, other.asNumber);
			asString = std::move(other.asString);

			return *this;
		}

		Value &operator=(const Value &other) = delete;

		void clear()
		{
			type = NUMBER;
			asNumber = 0;
			asString.clear();
		}
	};
	Value value;

	CoreLinkParameter()
	: name{}, value{}
	{}

	CoreLinkParameter(const char *_name, Value &&_value)
	: name{_name}, value{std::move(_value)}
	{}

	CoreLinkParameter &operator=(CoreLinkParameter &&other)
	{
		if (this != &other)
		{
			name = std::move(other.name);
			value = std::move(other.value);
		}
		return *this;
	}

	CoreLinkParameter(CoreLinkParameter &&other)
	{ operator=(std::move(other)); }

	~CoreLinkParameter()
	{}

	void clear()
	{
		name.clear();
		value.clear();
	}
};

struct CoreLinkType
{
	UriPath uri;
	std::vector<CoreLinkParameter> parameters;

	CoreLinkType()
	: uri{}, parameters{0}
	{}

	CoreLinkType(const char *_uri, std::error_code &ec)
	: uri{_uri, ec}, parameters{0}
	{}

	CoreLinkType(CoreLinkType &&other);

	CoreLinkType &operator=(CoreLinkType &&other);

	~CoreLinkType()
	{}
};

class CoreLink
{
public:
	CoreLink()
	: m_payload{0}, m_coreLink{}
	{}

	CoreLink(const char *coreLink, std::error_code &ec)
	: m_payload{0}, m_coreLink{coreLink}
	{ parse_core_link(coreLink, ec); }

	~CoreLink()
	{}

	void clear_payload()
	{ m_payload.clear(); }

	void clear();

	void add_record(CoreLinkType &&record)
	{ m_payload.push_back(std::move(record)); }

	void create_core_link(std::error_code &ec);
	void parse_core_link(const char *coreLink, std::error_code &ec);

	const std::string &core_link() const
	{
		return static_cast<const std::string &>(m_coreLink);
	}

	const std::vector<CoreLinkType> &payload() const
	{ return static_cast<const std::vector<CoreLinkType> &>(m_payload); }

private:
	void parse_record(const std::string &line, std::error_code &ec);

private:
	std::vector<CoreLinkType> m_payload;
	std::string m_coreLink;
};

/*
TODO use Uri class to convert data from Uri to CoRE-Link format 
and vise versa.
Implement Discovery procedure 

*/

}// namespace coap

#endif
