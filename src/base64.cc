#include "base64.h"

using namespace std;

bool Base64Encoder::s_decTableInited = false;

const size_t Base64Encoder::s_modTable[] = {0, 2, 1};

const size_t Base64Encoder::s_decTableSize;
const size_t Base64Encoder::s_encTableSize;

const char Base64Encoder::s_encodingTable[s_encTableSize] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

char Base64Encoder::s_decodingTable[s_decTableSize] = {0};

void Base64Encoder::decoding_table_init()
{
    for (size_t i = 0; i < s_encTableSize; i++)
    { s_decodingTable[(uint8_t) s_encodingTable[i]] = i; }
} 

Base64Encoder::Base64Encoder()
	: m_encodedSize{0},
	  m_decodedSize{0},
	  m_encodedData{nullptr},
	  m_decodedData{nullptr} 
{
	if (!s_decTableInited)
	{
		decoding_table_init();
		s_decTableInited = true;
	}
}

Base64Encoder::~Base64Encoder()
{
	if (m_encodedData)
		delete [] m_encodedData;
	if (m_decodedData)
		delete [] m_decodedData;
}

void Base64Encoder::encode(
				const uint8_t * data,
				size_t length,
				error_code &ec
			)
{
    m_encodedSize = 4 * ((length + 2) / 3);

    if (m_encodedData)
    	delete [] m_encodedData;

    m_encodedData = new char [m_encodedSize + sizeof(char)];

    if (m_encodedData == nullptr)
    {
    	ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
    	return;
    }

    for (size_t i = 0, j = 0; i < length;) {

        uint32_t octet_a = i < length ? (uint8_t)data[i++] : 0;
        uint32_t octet_b = i < length ? (uint8_t)data[i++] : 0;
        uint32_t octet_c = i < length ? (uint8_t)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        m_encodedData[j++] = s_encodingTable[(triple >> 3 * 6) & 0x3F];
        m_encodedData[j++] = s_encodingTable[(triple >> 2 * 6) & 0x3F];
        m_encodedData[j++] = s_encodingTable[(triple >> 1 * 6) & 0x3F];
        m_encodedData[j++] = s_encodingTable[(triple >> 0 * 6) & 0x3F];
    }

    for (size_t i = 0; i < s_modTable[length % 3]; i++)
        m_encodedData[m_encodedSize - 1 - i] = '=';
    m_encodedData[m_encodedSize] = '\0';
}

void Base64Encoder::decode(
				const char * data,
				size_t length,
				error_code &ec
			)
{
    if (length % 4 != 0)
    {
    	ec = make_system_error(EINVAL);
    	return;
    }

    m_decodedSize = length / 4 * 3;
    if (data[length - 1] == '=') m_decodedSize--;
    if (data[length - 2] == '=') m_decodedSize--;

    if (m_decodedData)
    	delete [] m_decodedData;

    m_decodedData = new uint8_t [m_decodedSize];

    if (m_decodedData == nullptr)
    {
    	ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
    	return;
    }

    for (size_t i = 0, j = 0; i < length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : s_decodingTable[(int)data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : s_decodingTable[(int)data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : s_decodingTable[(int)data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : s_decodingTable[(int)data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < m_decodedSize) m_decodedData[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < m_decodedSize) m_decodedData[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < m_decodedSize) m_decodedData[j++] = (triple >> 0 * 8) & 0xFF;
    }
}
