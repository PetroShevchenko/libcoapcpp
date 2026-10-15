#include "core_link.h"
#include "consts.h"
#include <cstdio>
#include <string>
#include <cstring>
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>

using namespace std;

namespace coap
{

static const char *root = "/.well-known/core";
static const char *beginUri = "</";
static const char *endUri = ">";
static const char *parameterSeparator = ";";
static const char *equal = "=";
static const char *quote = "\"";
static constexpr char recordSeparator = ',';

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

// remove spaces and symbols '\n', '\t', '\r'
static void clean_payload(std::string &payload)
{
	payload.erase(
		std::remove_if(payload.begin(), payload.end(),
			[](unsigned char c)
			{
				return c == '\n' || c == '\r';
			}
		),
		payload.end()
	);

	bool inQuote = false;
	size_t writeIndex = 0;

	for (size_t readIndex = 0; readIndex < payload.size(); ++readIndex)
	{
		const char c = payload[readIndex];
		if (inQuote)
		{
			// do not remove escaped symbols if they are inside quotes
			payload[writeIndex++] = c;
			if (c == '\\' && readIndex + 1 < payload.size())
				payload[writeIndex++] = payload[++readIndex]; // escaped character as-is
			else if (c == '"')
				inQuote = false; // closing quote
			continue;
		}
		if (c == ' ' || c == '\t')
			continue; // skip space or '\t' if it is not inside quotes
		if (c == '"')
			inQuote = true; // opening quote
		payload[writeIndex++] = c;
	}
	payload.resize(writeIndex);
}

static std::vector<std::string> split_records(const std::string &payload, std::error_code &ec)
{
	std::vector<std::string> out;
	bool inUri   = false;   // between '<' and '>'
	bool inQuote = false;   // between '"' and '"'
	size_t start  = 0;

	for (size_t i = 0; i < payload.size(); ++i)
	{
		const char c = payload[i];

		if (inQuote)
		{
			if (c == '\\' && i + 1 < payload.size()) ++i;	// skip escaped symbol
			else if (c == '"') inQuote = false;			// closing quote
		}
		else if (inUri)
		{
			if (c == '>') inUri = false; 	// closing uri
		}
		else if (c == '"') inQuote = true; // opening quote
		else if (c == '<') inUri = true; 	// opening uri
		else if (c == recordSeparator) 	// record separator
		{
			if (i > start) 			// skip empty records
				out.emplace_back(payload, start, i - start); // store record
			start = i + 1; 			// next record starts after separator
		}
	}
	if (start < payload.size())
		out.emplace_back(payload, start, payload.size() - start); // store the last record

	if (inQuote || inUri || out.empty())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_PARSE_CORE_LINK);
		return {};
	}
	return out; // return vector of records
}

void CoreLink::parse_core_link(const char *coreLink, std::error_code &ec)
{
	if (coreLink == nullptr)
	{
		ec = make_system_error(EFAULT);
		return;
	}
	std::string buffer(coreLink);

	clean_payload(buffer);

	std::vector<std::string> records = split_records(buffer, ec);
	if (ec) return;

	for (const auto &record: records)
	{
		parse_record(record, ec);
		if (ec) break;
	}
}

void CoreLink::parse_record(const string &line, std::error_code &ec)
{
	ec = make_error_code(CoapStatus::COAP_ERR_PARSE_CORE_LINK);

	if (line.empty() || line[0] != '<') return;

	string::size_type offset = 0;

	string::size_type closingBracket = line.find(">", offset + 1);
	if (closingBracket == string::npos) return;

	string::size_type start = offset + 1;
	if (start < closingBracket && line[start] == '/')
		++start;

	CoreLinkType record;
	record.uri.path(line.substr(start, closingBracket - start).c_str());

	string::size_type next = closingBracket + 1;
	if (next < line.size() && line[next] != ';') return;

	offset = next;

	while (offset < line.size()) // line[offset] == ';'
	{
		string::size_type nameStart = offset + 1;
		string::size_type nameEnd = line.find_first_of("=;", nameStart);

		// parameter without a value or an empty name
		if (nameEnd == string::npos || line[nameEnd] != '=' || nameEnd == nameStart)
			return;

		CoreLinkParameter param;
		param.name = line.substr(nameStart, nameEnd - nameStart);

		string::size_type valueStart = nameEnd + 1;
		string::size_type valueEnd; // position just after the value

		if (valueStart < line.size() && line[valueStart] == '"')
		{
			string::size_type closeQuote = line.find('"', valueStart + 1);
			if (closeQuote == string::npos) return; // unclosed quote

			param.value.type = CoreLinkParameter::STRING;
			param.value.asString = line.substr(valueStart + 1, closeQuote - valueStart - 1);
			valueEnd = closeQuote + 1;
		}
		else
		{
			valueEnd = line.find(';', valueStart);
			if (valueEnd == string::npos)	valueEnd = line.size();

			const char *first = line.data() + valueStart;
			const char *last  = line.data() + valueEnd;

			if (first == last || *first < '0' || *first > '9')
				return;

			char *end = nullptr;
			errno = 0;
			unsigned long number = strtoul(first, &end, 10);

			if (errno == ERANGE || end != last || number > UINT_MAX)
				return; // overflow or garbage after the number

			param.value.type = CoreLinkParameter::NUMBER;
			param.value.asNumber = static_cast<unsigned int>(number);
		}

		// after the value — only ';' or the end
		if (valueEnd < line.size() && line[valueEnd] != ';') return;

		record.parameters.push_back(std::move(param));
		offset = valueEnd; // next search — after the value
	}
	add_record(std::move(record));
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
    if (ec)
        return false;

    const auto &haystack = in.uri.uri().asString();
    const auto &needle   = comparedPath.uri().asString();

    if (needle.empty())
        return false;

    auto it = std::search(haystack.begin(), haystack.end(),
                          needle.begin(), needle.end());
    if (it == haystack.end())
        return false;

    out.uri.uri().asString().assign(it, haystack.end());
    out.uri.uri_to_path();
    out.parameters.insert(out.parameters.end(),
                          in.parameters.begin(), in.parameters.end());
    return true;
}

}//namespace core_link

}// namespace coap
