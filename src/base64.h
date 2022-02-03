#ifndef _BASE64_H
#define _BASE64_H
#include <cstdint>
#include "error.h"

class Base64Encoder
{
public:
	Base64Encoder();
	~Base64Encoder();

public:
	void encode(
				const uint8_t * data,
				size_t length,
				std::error_code &ec
			);
	void decode(
				const char * data,
				size_t length,
				std::error_code &ec
			);

private:
	static void decoding_table_init();

private:
	size_t m_encodedSize;
	size_t m_decodedSize;
	char * m_encodedData;
	uint8_t * m_decodedData;

public:
	size_t encoded_size() const
	{ return m_encodedSize; }

	size_t decoded_size() const
	{ return m_decodedSize; }

	const char * encoded_data() const
	{ return static_cast<const char *>(m_encodedData); }

	const uint8_t * decoded_data() const
	{ return static_cast<const uint8_t *>(m_decodedData); }

private:
	static bool s_decTableInited;
	static const size_t s_modTable[];
	static const size_t s_encTableSize = 64;
	static const size_t s_decTableSize = 256;
	static const char s_encodingTable[s_encTableSize];
	static char s_decodingTable[s_decTableSize];
};

#endif
