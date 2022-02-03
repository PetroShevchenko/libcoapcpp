#include "senml_json.h"
#include "base64.h"
#include <cstring>
#include <iostream>

using namespace std;

namespace coap
{

void SenmlJson::clear()
{
	clear_payload();
	if (m_json)
	{
		free(m_json);
		m_json = nullptr;
	}
	m_baseName.clear();
	m_baseUnit.clear();
	m_baseTime = 0;
	m_baseValue = 0;
	m_baseSum = 0;
	m_baseVersion = 0;
}

void SenmlJson::create_json(std::error_code &ec)
{
	if (m_payload.empty())
	{
		ec = make_error_code(CoapStatus::COAP_ERR_NO_PAYLOAD);
		return;
	}

	ec = make_error_code(CoapStatus::COAP_ERR_CREATE_JSON);

	cJSON *records = NULL;
	cJSON *record = NULL;
	cJSON *name = NULL;
	cJSON *unit = NULL;
	cJSON *time = NULL;
	cJSON *value = NULL;

	records = cJSON_CreateArray();
	if (records == NULL) goto forced_exit;

	for (std::vector<SenmlJsonType>::const_iterator
						iter = m_payload.begin(),
						end = m_payload.end(); iter != end; ++iter)
	{
		record = cJSON_CreateObject();
		if (record == NULL) goto forced_exit;

		cJSON_AddItemToArray(records, record);

		name = cJSON_CreateString(iter->name.c_str());
		if (name == NULL) goto forced_exit;

		cJSON_AddItemToObject(record, "n", name);

		unit = cJSON_CreateString(iter->unit.c_str());
		if (unit == NULL) goto forced_exit;

		cJSON_AddItemToObject(record, "u", unit);

		time = cJSON_CreateNumber(iter->time);
		if (time == NULL) goto forced_exit;

		cJSON_AddItemToObject(record, "t", time);

		value = NULL;

		switch(iter->value.type)
		{
			case SenmlJsonType::NUMBER:
			{
				value = cJSON_CreateNumber(iter->value.asNumber);
				if (value == NULL) goto forced_exit;
				cJSON_AddItemToObject(record, "v", value);
			}
			break;

			case SenmlJsonType::STRING:
			{
				value = cJSON_CreateString(iter->value.asString.c_str());
				if (value == NULL) goto forced_exit;
				cJSON_AddItemToObject(record, "vs", value);
			}
			break;

			case SenmlJsonType::BOOLEAN:
			{
				value = cJSON_CreateBool((cJSON_bool)iter->value.asBoolean);
				if (value == NULL) goto forced_exit;
				cJSON_AddItemToObject(record, "vb", value);
			}
			break;

			case SenmlJsonType::DATA:
			{
				Base64Encoder encoder;
				ec.clear();
				encoder.encode(iter->value.asData.data(), iter->value.asData.size(), ec);
				if (ec.value())	goto forced_exit;

				value = cJSON_CreateString(encoder.encoded_data());
				if (value == NULL) goto forced_exit;
				cJSON_AddItemToObject(record, "vd", value);
			}
			break;
		}
	}
	ec.clear();
	if (m_json)	free(m_json);

	m_json = cJSON_Print(records);

forced_exit:
	cJSON_Delete(records);
}

static const char *labels[] = {
	"bn",
	"bt",
	"bu",
	"bv",
	"bs",
	"bver",
	"n",
	"u",
	"v",
	"vs",
	"vb",
	"vd",
	"s",
	"t",
	"ut"
};

void SenmlJson::parse_record(cJSON *node, std::error_code &ec)
{
	if (node == nullptr)
	{
		ec = make_system_error(EFAULT);
		return;
	}

	ec = make_error_code(CoapStatus::COAP_ERR_PARSE_JSON);

	SenmlJsonType record;

	for(auto label : labels)
	{
		const cJSON *item = cJSON_GetObjectItemCaseSensitive(node, label);
		if (item == NULL) continue;

		if (cJSON_IsString(item))
		{
			const char *value = cJSON_GetStringValue(item);
			if (value == NULL) return;

			if (strncmp(label, "bn", strlen(label)) == 0)
			{
				m_baseName = value;
			}
			else if (strncmp(label, "bu", strlen(label)) == 0)
			{
				m_baseUnit = value;
			}
			else if (strncmp(label, "n", strlen(label)) == 0)
			{
				record.name = value;
			}
			else if (strncmp(label, "u", strlen(label)) == 0)
			{
				record.unit = value;
			}
			else if (strncmp(label, "vs", strlen(label)) == 0)
			{
				record.value.type = SenmlJsonType::STRING;
				record.value.asString = string(value);
			}
			else if (strncmp(label, "vd", strlen(label)) == 0)
			{
				record.value.type = SenmlJsonType::DATA;
				copy(value, value + strlen(value), record.value.asData.begin());
			}
			else return;
			continue;
		}
		else if (cJSON_IsNumber(item))
		{
			double value = cJSON_GetNumberValue(item);

			if (strncmp(label, "bt", strlen(label)) == 0)
			{
				m_baseTime = value;
			}
			else if (strncmp(label, "bv", strlen(label)) == 0)
			{
				m_baseValue = value;
			}
			else if (strncmp(label, "bs", strlen(label)) == 0)
			{
				m_baseSum = value;
			}
			else if (strncmp(label, "bver", strlen(label)) == 0)
			{
				m_baseVersion = value;
			}
			else if (strncmp(label, "v", strlen(label)) == 0)
			{
				record.value.type = SenmlJsonType::NUMBER;
				record.value.asNumber = value;
			}
			else if (strncmp(label, "s", strlen(label)) == 0)
			{
				record.sum = value;
			}
			else if (strncmp(label, "t", strlen(label)) == 0)
			{
				record.time = value;
			}
			else if (strncmp(label, "ut", strlen(label)) == 0)
			{
				record.updateTime = value;
			}
			else return;
			continue;
		}
		else if (cJSON_IsBool(item) && strncmp(label, "vb", strlen(label)) == 0)
		{
			record.value.type = SenmlJsonType::BOOLEAN;
			record.value.asBoolean = cJSON_IsTrue(item);
		}
	}
	m_payload.push_back(record);
	ec.clear();
}

void SenmlJson::parse_json(const char *json, std::error_code &ec)
{
	if (json == nullptr)
	{
		ec = make_system_error(EFAULT);
		return;
	}

	cJSON *root = NULL;
	cJSON *element = NULL;
	int size;

	clear();
	ec = make_error_code(CoapStatus::COAP_ERR_PARSE_JSON);

	root = cJSON_Parse(json);
	if (root == NULL) goto forced_exit;

	size = cJSON_GetArraySize(root);
	if (size <= 0) goto forced_exit;

	for (int i = 0; i < size; ++i)
	{
		element = cJSON_GetArrayItem(root, i);
		if (element == NULL) goto forced_exit;
		parse_record(element, ec);
		if (ec.value()) goto forced_exit;
	}

	ec.clear();

forced_exit:
	cJSON_Delete(root);
}

}// namespace coap
