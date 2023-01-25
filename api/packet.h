#ifndef _PACKET_H
#define _PACKET_H
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>
#include <array>
#include "consts.h"
#include "error.h"
#include "core_link.h"
#include "senml_json.h"

namespace coap
{

struct Option
{
    union Header
    {
        std::uint8_t asByte;
#pragma pack(push,1)
        struct
        {
            std::uint8_t length : 4;
            std::uint8_t delta : 4;
        } asBitField;
#pragma pack(pop)
    };

    Option()
        : m_header{0}, m_number{0}, m_value{}
    {}
    ~Option()
    {}

    std::uint8_t length() const
    { return m_header.asBitField.length; }

    std::uint8_t delta() const
    { return m_header.asBitField.delta; }

    std::uint8_t header_as_byte() const
    { return m_header.asByte; }

    void header_as_byte(std::uint8_t value)
    { m_header.asByte = value; }

    std::uint16_t number() const
    { return m_number; }

    void number(std::uint16_t value)
    { m_number = value; }

    const std::vector<std::uint8_t> &value() const
    { return static_cast<const std::vector<std::uint8_t> &>(m_value); }

    std::vector<std::uint8_t> &value()
    { return m_value; }

    void clear()
    {
        m_header.asByte = 0;
        m_number = 0;
        m_value.clear();
    }

    bool operator<(const Option &other)
    {
        return m_number < other.m_number;
    }

private:
    Header                      m_header;
    std::uint16_t               m_number;
    std::vector<std::uint8_t>   m_value;
};

using OptionList = std::vector<Option>;

using PayloadType = std::vector<std::uint8_t>;

using TokenType = std::array<std::uint8_t,TOKEN_MAX_LENGTH>;

struct Message
{
    union Header {
        std::uint8_t asByte;
#pragma pack(push,1)
        struct {
            std::uint8_t tokenLength: 4;
            std::uint8_t type: 2;
            std::uint8_t version: 2;
        } asBitField;
#pragma pack(pop)
    };

    union Code {
        std::uint8_t asByte;
#pragma pack(push,1)
        struct {
            std::uint8_t codeDetail: 5;
            std::uint8_t codeClass: 3;
        } asBitField;
#pragma pack(pop)
    };

    Message()
        : m_header{0}, m_code{0}, m_identity{0}, m_token{0}, m_options{}, m_payloadOffset{0}, m_payload{}
    {}
    virtual ~Message()
    {}

    OptionList &options()
    { return m_options; }

    const OptionList &options() const
    { return static_cast<const OptionList &>(m_options); }

    void header_as_byte(std::uint8_t value)
    { m_header.asByte = value; }

    std::uint8_t header_as_byte() const
    { return m_header.asByte; }

    std::uint8_t type() const
    { return m_header.asBitField.type; }

    void type(std::uint8_t value)
    { m_header.asBitField.type = value; }

    std::uint8_t version() const
    { return m_header.asBitField.version; }

    void version(std::uint8_t value)
    { m_header.asBitField.version = value; }

    void code_as_byte(std::uint8_t value)
    { m_code.asByte = value; }

    std::uint8_t code_as_byte() const
    { return m_code.asByte; }

    std::uint8_t code_detail() const
    { return m_code.asBitField.codeDetail; }

    std::uint8_t code_class() const
    { return m_code.asBitField.codeClass; }

    void identity(std::uint16_t value)
    { m_identity = value; }

    std::uint16_t identity() const
    { return m_identity; }

    void token_length(const std::size_t len)
    { m_header.asBitField.tokenLength = static_cast<std::uint8_t>(len); }

    std::size_t token_length() const
    { return static_cast<std::size_t>(m_header.asBitField.tokenLength); }

    TokenType & token()
    { return m_token; }

    const TokenType & token() const
    { return static_cast<const TokenType &>(m_token); }

    void payload_offset(std::size_t value)
    { m_payloadOffset = value; }

    std::size_t payload_offset() const
    { return m_payloadOffset; }

    PayloadType &payload()
    { return m_payload; }

    const PayloadType &payload() const
    { return static_cast<const PayloadType &>(m_payload); }

    void sort_options()
    { std::sort(m_options.begin(), m_options.end()); }

    bool generate_token(const std::size_t len = TOKEN_MAX_LENGTH);

    void clear();

private:
    Header          m_header;
    Code            m_code;
    std::uint16_t   m_identity;
    TokenType       m_token;
    OptionList      m_options;
    std::size_t     m_payloadOffset;
    PayloadType     m_payload;
};

bool is_little_endian_byte_order();

class Packet : public Message
{
public:
    Packet()
        : m_littleEndian{is_little_endian_byte_order()}
    {}
    ~Packet()
    {}

    Packet(const Packet &) = delete;
    Packet(Packet &&) = delete;
    Packet & operator=(const Packet &) = delete;
    Packet & operator=(Packet &&) = delete;

private:
    void parse_header(const void * buffer, std::size_t size, std::error_code &ec);
    void parse_token(const void * buffer, std::size_t size, std::error_code &ec);
    void parse_options(const void * buffer, std::size_t size, std::error_code &ec);
    void parse_payload(const void * buffer, std::size_t size, std::error_code &ec);

    std::size_t get_option_nibble(std::size_t value);

public:
    void add_option(
            OptionNumber number,    // option number
            const void * value,     // option value
            size_t length,          // option length
            std::error_code &ec
        );

    std::size_t
        find_option(
            const std::uint16_t number,
            std::vector<Option *> &rOptions
        );

    void parse(
            const void * buffer,
            std::size_t size,
            std::error_code &ec
        );

    void serialize(
            std::error_code &ec,
            void * buffer,
            std::size_t &size,
            bool checkBufferSizeOnly = false
        );

    void make_request(
            std::error_code &ec,
            MessageType type,   // message type
            MessageCode code,   // response code
            std::uint16_t id,   // message identity
            const void * payload,
            std::size_t payloadSize,
            std::size_t tokenLength = TOKEN_MAX_LENGTH
        );

    void prepare_answer(
            std::error_code &ec,
            MessageType type,   // message type
            MessageCode code,   // response code
            std::uint16_t id,   // message identity
            const void * payload,
            std::size_t payloadSize
        );

    bool little_endian() const
    { return m_littleEndian; }

private:
    const bool  m_littleEndian;
};

std::uint16_t generate_identity();

struct DataType
{
    enum ValueType
    {
        TYPE_STRING,
        TYPE_HEX_ARRAY,
        TYPE_SENML_JSON,
        TYPE_CORE_LINK,
    };

    struct Value
    {
        ValueType type;
        std::string asString;
        std::vector<uint8_t> asHexArray;
        std::vector<SenmlJsonType> asSenmlJson;
        std::vector<CoreLinkType> asCoreLink;

        Value(const char *str)
        : type{TYPE_STRING}, asString{str}, asHexArray{0}, asSenmlJson{0}, asCoreLink{0}
        {}

        Value(const uint8_t *hex, size_t size)
        : type{TYPE_HEX_ARRAY}, asString{}, asSenmlJson{0}, asCoreLink{0}
        {
            for (size_t i = 0; i < size; ++i)
                asHexArray[i] = hex[i];
        }

        Value(const std::vector<SenmlJsonType> &senmlJson)
        : type{TYPE_SENML_JSON}, asString{}, asHexArray{0}, asSenmlJson{senmlJson}, asCoreLink{0}
        {}

        Value(std::vector<CoreLinkType> &&coreLink)
        : type{TYPE_CORE_LINK}, asString{}, asHexArray{0}, asSenmlJson{0}, asCoreLink{std::move(coreLink)}
        {}

        Value(const Value &other)
        {
            if (this != &other)
            {
                type = other.type;
                asString = other.asString;
                asHexArray = other.asHexArray;
                asSenmlJson = other.asSenmlJson;
                //asCoreLink = other.asCoreLink; // TODO Uncomment after CoreLinkType implementation
            }
        }

        ~Value()
        {}
    };

    Value       value;

    DataType(const char *str)
    : value{str}
    {}

    DataType(const uint8_t *hex, size_t size)
    : value{hex, size}
    {}

    DataType(const std::vector<SenmlJsonType> &senmlJson)
    : value{senmlJson}
    {}

    DataType(std::vector<CoreLinkType> &&coreLink)
    : value{std::move(coreLink)}
    {}

    ~DataType()
    {}
};

} // namespace coap

#endif
