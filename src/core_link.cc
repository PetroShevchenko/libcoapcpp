#include "core_link.h"
#include "consts.h"
#include <cstdio>
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

CoreLinkType::CoreLinkType(const CoreLinkType &other)
{
	if (this != &other)
	{
		uri = other.uri;
		std::copy(other.parameters.begin(), other.parameters.end(),
			std::back_inserter(parameters));
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

	size_t length = strlen(coreLink) + sizeof('\0');

	char *buffer = new char [length];
	if (buffer == nullptr)
	{
		ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
		return;
	}
	memset(buffer, 0, length);
	strncpy(buffer, coreLink, length);

	std::vector<std::string> records;
	char *token = strtok(buffer, recordSeparator);
	while(token != NULL)
	{

		records.push_back(token);
		token = strtok(NULL, recordSeparator);
	}
	for (auto record: records)
	{
		parse_record(record, ec);
		if (ec.value()) break;
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

namespace core_link
{

bool is_root(const char *line)
{ return !strcmp(root, line); }

bool is_root(std::string &line)
{ return is_root(line.c_str()); }

bool is_record_matched(std::string &line, const CoreLinkType &record)
{ return !strcmp(line.c_str(), record.uri.path().c_str()); }

bool is_attribute_matched(const char *name, const char *value, const CoreLinkParameter &a)
{
	if (a.value.type == CoreLinkParameter::STRING
		&& !strcmp(name, a.name.c_str())
		&& !strcmp(value, a.value.asString.c_str()))
		return true;
	return false;
}

bool is_attribute_matched(const char *name, unsigned long value, const CoreLinkParameter &a)
{
	if (a.value.type == CoreLinkParameter::NUMBER
		&& !strcmp(name, a.name.c_str())
		&& value == a.value.asNumber)
		return true;
	return false;
}

std::vector<CoreLinkParameter>::const_iterator
find_attribute(const char *name, const CoreLinkType &record)
{
	vector<CoreLinkParameter>::const_iterator iter, end;
    for (iter = record.parameters.begin(),
         end = record.parameters.end(); iter != end; ++iter)
    	if (!strcmp(name, iter->name.c_str()))
    		return iter;
    return end;
}

bool create_record_from_path_if_contains(
			const char *path,
			const CoreLinkType &in,
			CoreLinkType &out,
			std::error_code &ec
		)
{
	if (path == nullptr)
	{
		ec = make_system_error(EFAULT);
		return false;
	}

	UriPath comparedPath(path, ec);
	if (ec.value())
		return false;

	for (size_t i = 0, j = 0; i < in.uri.uri().asString().size(); i++)
	{
		if (comparedPath.uri().asString()[j] == in.uri.uri().asString()[i])
		{
			for(size_t k = i + 1; k < in.uri.uri().asString().size(); ++k)
			{
				out.uri.uri().asString().push_back(comparedPath.uri().asString()[j++]);
				if (j == comparedPath.uri().asString().size())
				{
					for(size_t l = k; l < in.uri.uri().asString().size(); ++l)
						out.uri.uri().asString().push_back(in.uri.uri().asString()[l]);

					out.uri.uri_to_path();
					std::copy(in.parameters.begin(), in.parameters.end(),
						std::back_inserter(out.parameters));
					return true;
				}

				if (comparedPath.uri().asString()[j] != in.uri.uri().asString()[k])
				{
					j = 0;
					i++;
					out.uri.uri().asString().clear();
					break;
				}
			}
		}
	}
	return false;
}

}//namespace core_link

}// namespace coap
