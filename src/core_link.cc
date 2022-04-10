#include "core_link.h"
#include <string>
#include <cstring>
#include <iostream>

using namespace std;

namespace coap
{

static const char *root = "/.well-known/core";
static const char request = '?';
static const char *beginUri = "</";
static const char *endUri = ">";
static const char *recordSeparator = ",";
static const char *parameterSeparator = ";";
static const char *equal = "=";
static const char *quote = "\"";

CoreLinkType::CoreLinkType(CoreLinkType &&other)
{
	if (this != &other)
	{
		uri = std::move(other.uri);
		parameters = std::move(other.parameters);
	}
}

CoreLinkType &CoreLinkType::operator=(CoreLinkType &&other)
{
	if (this != &other)
	{
		uri = std::move(other.uri);
		parameters = std::move(other.parameters);
	}
	return *this;
}

void CoreLink::clear()
{
	m_payload.clear();
	m_coreLink.clear();
}

void CoreLink::create_core_link(std::error_code &ec)
{
	if (m_payload.empty())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_NO_PAYLOAD);
		return;
	}

	m_coreLink.clear();

	for(vector<CoreLinkType>::const_iterator
					iter = m_payload.begin(),
					end = m_payload.end(); iter != end; ++iter)
	{
		m_coreLink.append(beginUri + iter->uri.path() + endUri);

		for(vector<CoreLinkParameter>::const_iterator
						iter2 = iter->parameters.begin(),
						end2 = iter->parameters.end(); iter2 != end2; ++iter2)
		{
			string value = (iter2->value.type == CoreLinkParameter::NUMBER) ?
												to_string(iter2->value.asNumber) :
												quote + iter2->value.asString + quote;
			m_coreLink.append(parameterSeparator + iter2->name + equal + value);
		}

		if (iter + 1 != end) { m_coreLink += recordSeparator; }
	}
}

void CoreLink::parse_core_link(const char *coreLink, std::error_code &ec)
{
	if (coreLink == nullptr)
	{
		ec = make_system_error(EFAULT);
		return;
	}

	size_t length = strlen(coreLink);

	char *buffer = new char [length];
	if (buffer == nullptr)
	{
		ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
		return;
	}

	memcpy(buffer, coreLink, length);

	char *token = strtok(buffer, recordSeparator);
	while(token != NULL)
	{
		string record(token);

		parse_record(record, ec);

		if (ec.value()) break;

		token = strtok(NULL, recordSeparator);
	}

	delete [] buffer;
}

void CoreLink::parse_record(const string &line, std::error_code &ec)
{
	ec = make_error_code(CoapStatus::COAP_ERR_PARSE_CORE_LINK);

	string::size_type offset = line.find("<");

	if (offset == string::npos)	return;

	string::size_type closingBracket = line.find(">", offset + 1);

	if (closingBracket == string::npos)	return;

	string::size_type slash = line.find("</");

	CoreLinkType record;

	if (slash != string::npos)
	{
		record.uri.path(line.substr(slash + strlen("</"), closingBracket - slash - strlen("</")).c_str());
	}
	else
	{
		record.uri.path(line.substr(offset + strlen("<"), closingBracket - offset - strlen("<")).c_str());
	}

	offset = closingBracket;

	while(true)
	{
		offset = line.find(parameterSeparator, offset + 1);

		if (offset == string::npos) break;

		CoreLinkParameter::Value value;
		CoreLinkParameter param;

		string::size_type separator = line.find(equal, offset + 1);
		if (separator == string::npos) return;

		param.name = line.substr(offset + 1, separator - offset - 1);

		if ( strncmp(line.substr(separator + 1, 1).c_str(), quote, strlen(quote)) == 0)
		{
			value.type = CoreLinkParameter::STRING;
			string::size_type end = line.find("\"",separator + sizeof("\"") + 1);
			value.asString.assign(line.substr(separator + strlen("=\""), end - separator - strlen("\"") - 1));
		}
		else
		{
			value.type = CoreLinkParameter::NUMBER;
			string::size_type end = line.find(";",separator + 1);
			value.asNumber = atoi(line.substr(separator + 1, end - separator - 1).c_str());
		}
		param.value = move(value);
		record.parameters.push_back(move(param));
	}
	add_record(move(record));
	ec.clear();
}

}// namespace coap
