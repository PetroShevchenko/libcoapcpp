#include "packet.h"
#include "spdlog/spdlog.h"
#include <cstdlib>
#include <climits>
#include <cassert>
#include <cstring>

using namespace std;
using namespace spdlog;

namespace coap
{

bool Message::generate_token(const std::size_t len)
{
    if (len > TOKEN_MAX_LENGTH)
        return false;

    static bool initialized = false;

    if (!initialized )
    {
        std::srand(time(nullptr));
        initialized  = true;
    }

    for (std::size_t i = 0; i < len; i++)
    {
        m_token[i] = static_cast<uint8_t>(rand() % (UCHAR_MAX + 1));
    }

    token_length(len);

    return true;
}

void Packet::parse_header(const void * buffer, size_t size, error_code &ec)
{
    assert(buffer != nullptr);
    assert(size >= PACKET_MIN_LENGTH);

    ec.clear();

    if (buffer == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    if (size < PACKET_MIN_LENGTH)
    {
        ec = make_system_error(EINVAL);
        return;
    }

    const uint8_t * buf = static_cast<const uint8_t *>(buffer);

    header_as_byte(buf[0]);

    if (COAP_VERSION != version())
    {
        ec = make_error_code(CoapStatus::COAP_ERR_PROTOCOL_VERSION);
        return;
    }

    code_as_byte(buf[1]);

    if (m_littleEndian)
        identity(static_cast<std::uint16_t>(buf[3] | (buf[2] << 8)));
    else
        identity(static_cast<std::uint16_t>(buf[2] | (buf[3] << 8)));
}

void Packet::parse_token(const void * buffer, size_t size, std::error_code &ec)
{
    assert(buffer != nullptr);

    ec.clear();

    if (buffer == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    if (token_length() > TOKEN_MAX_LENGTH)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_TOKEN_LENGTH);
        return;
    }

    if (size < PACKET_HEADER_SIZE + token_length())
    {
        ec = make_system_error(EINVAL);
        return;
    }

    const uint8_t * buf = static_cast<const uint8_t *>(buffer);

    memcpy(token().data(), &buf[PACKET_HEADER_SIZE], token_length());
}

static bool parse_option(
        const uint8_t * buffer,
        size_t size,
        uint8_t parsing,
        uint16_t &modifying,
        size_t &offset,
        bool littleEndian
    )
{
    assert(buffer != nullptr);

    if (buffer == nullptr)
        return false;

    if (parsing == MINUS_THIRTEEN)
    {
        if (offset + 1 > size)
            return false;
        modifying = buffer[offset + 1] + MINUS_THIRTEEN_OPT_VALUE;
        offset += sizeof(uint8_t);
    }
    else if (parsing == MINUS_TWO_HUNDRED_SIXTY_NINE)
    {
        if (offset + 2 > size)
            return false;
        if (littleEndian)
            modifying = buffer[offset + 1] | (buffer[offset + 2] << 8);
        else
            modifying = buffer[offset + 2] | (buffer[offset + 1] << 8);
        modifying += MINUS_TWO_HUNDRED_SIXTY_NINE_OPT_VALUE;
        offset += sizeof(uint16_t);
    }
    else if (parsing == RESERVED_FOR_FUTURE)
        return false;

    return true;
}

void Packet::parse_options(const void * buffer, size_t size, std::error_code &ec)
{
    assert(buffer != nullptr);

    ec.clear();

    if (buffer == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    const uint8_t * buf = static_cast<const uint8_t *>(buffer);
    const size_t start_offset = PACKET_HEADER_SIZE + token_length();

    options().clear();

    uint16_t optDelta = 0;
    uint16_t optLength = 0;
    uint16_t optNumber = 0;
    Option opt;

    size_t offset = start_offset;
    for (; buf[offset] !=  PAYLOAD_MARKER && offset < size; offset += optLength)
    {
        opt.header_as_byte(buf[offset]);
        optDelta = opt.delta();
        optLength = opt.length();

        if ( !parse_option(buf, size, opt.delta(), optDelta, offset, m_littleEndian) )
        {
            ec = make_error_code(CoapStatus::COAP_ERR_OPTION_DELTA);
            return;
        }

        if ( !parse_option(buf, size, opt.length(), optLength, offset, m_littleEndian) )
        {
            ec = make_error_code(CoapStatus::COAP_ERR_OPTION_LENGTH);
            return;
        }

        ++offset;

        if (offset + optLength > size)
        {
            ec = make_error_code(CoapStatus::COAP_ERR_OPTION_LENGTH);
            return;
        }

        optNumber += optDelta;
        opt.number(optNumber);

        for (int i = 0; i < optLength; i++)
        {
            opt.value().push_back(buf[offset + i]);
        }

        options().push_back(opt);
        opt.clear();
    }

    payload_offset(offset + sizeof(PAYLOAD_MARKER));
}

void Packet::parse_payload(const void * buffer, size_t size, std::error_code &ec)
{
    assert(buffer != nullptr);
    assert(size >= payload_offset());

    ec.clear();

    if (buffer == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    if (size < payload_offset())
    {
        ec = make_system_error(EINVAL);
        return;
    }

    const uint8_t * buf = static_cast<const uint8_t *>(buffer);
    const size_t maxPayloadOffset = size - payload_offset();

    for (size_t i = 0; i < maxPayloadOffset; i++)
        payload().push_back(buf[payload_offset() + i]);
}

void Packet::parse(const void * buffer, size_t size, std::error_code &ec)
{
    assert(buffer != nullptr);
    assert(size != 0);

    ec.clear();

    if (buffer == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    if (!size)
    {
        ec = make_system_error(EINVAL);
        return;
    }

    parse_header(buffer, size, ec);
    if (ec) return;

    parse_token(buffer, size, ec);
    if (ec) return;

    parse_options(buffer, size, ec);
    if (ec) return;

    parse_payload(buffer, size, ec);
}

void Packet::add_option(
        OptionNumber number,    // option number
        const void * value,     // option value
        size_t length,          // option length
        error_code &ec
    )
{
    assert(value != nullptr);
    assert(number <= OPTION_MAX_NUMBER);
    assert(length <= OPTION_MAX_LENGTH);

    ec.clear();
    if (value == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    if (number > OPTION_MAX_NUMBER)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_OPTION_NUMBER);
        return;
    }

    if (length > OPTION_MAX_LENGTH)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_OPTION_NUMBER);
        return;
    }

    Option opt;
    opt.header_as_byte(0);
    opt.number(static_cast<std::uint8_t>(number));

    const uint8_t * val = static_cast<const uint8_t *>(value);
    for(size_t i = 0; i < length; ++i)
    {
        opt.value().push_back(val[i]);
    }

    options().push_back(opt);
    sort_options();
}

size_t Packet::find_option(const std::uint16_t number, std::vector<Option *> &rOptions)
{
    size_t index = 0 , size = options().size();
    size_t quantity = 0;
    rOptions.clear();

    sort_options();

    while (index < size)
    {
        if (options()[index].number() == number)
        {
            // the option is found
            do {
                ++quantity;
                rOptions.push_back(&options()[index]);
            } while (++index < size && options()[index].number() == number); // search forward

            return quantity;
        }
        else
        {
            index++;
        }
    }
    return quantity;
}

size_t Packet::get_option_nibble(size_t value)
{
    size_t nibble = 0;

    if (value < MINUS_THIRTEEN_OPT_VALUE)
    {
        nibble = value & 0xFF;
    }
    else if (value <= 0xFF + MINUS_THIRTEEN_OPT_VALUE)
    {
        nibble = MINUS_THIRTEEN;
    }
    else if (value <= 0xFFFF + MINUS_TWO_HUNDRED_SIXTY_NINE_OPT_VALUE)
    {
        nibble = MINUS_TWO_HUNDRED_SIXTY_NINE;
    }
    return nibble;
}

static
void make_option(
            uint8_t *buffer,
            size_t size,
            size_t & offset,
            size_t firstParsing,
            size_t secondParsing,
            bool checkBufferSizeOnly,
            error_code &ec
        )
{
    ec.clear();

    if (buffer == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    if (firstParsing == MINUS_THIRTEEN)
    {
        if (checkBufferSizeOnly)
            offset++;
        else
            buffer [offset++] = secondParsing - MINUS_THIRTEEN_OPT_VALUE;
    }
    else if (firstParsing == MINUS_TWO_HUNDRED_SIXTY_NINE)
    {
        if (checkBufferSizeOnly)
            offset += 2;
        else
        {
            buffer [offset++] = (secondParsing - MINUS_TWO_HUNDRED_SIXTY_NINE_OPT_VALUE) >> 8;
            buffer [offset++] = (secondParsing - MINUS_TWO_HUNDRED_SIXTY_NINE_OPT_VALUE) & 0xFF;
        }
    }
    if (!checkBufferSizeOnly && offset >= size)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);
    }
}

void Packet::serialize(
        error_code &ec,
        void * buffer,
        size_t &size,
        bool checkBufferSizeOnly
    )
{
#define exit_if_buffer_overflow(o, s, e, f)\
        if (!f && o >= s)\
        {\
            assert(0);\
            e = make_error_code(CoapStatus::COAP_ERR_BUFFER_SIZE);\
            return;\
        }

    ec.clear();

    if (!checkBufferSizeOnly)
    {
        if (buffer == nullptr)
        {
            assert(0);
            ec = make_system_error(EFAULT);
            return;
        }

        if (size < static_cast<size_t>(PACKET_MIN_LENGTH + token_length()))
        {
            assert(0);
            ec = make_system_error(EINVAL);
            return;
        }
    }

    uint8_t * buf = static_cast<uint8_t *>(buffer);

    size_t offset = MESSAGE_ID_OFFSET;

    if (!checkBufferSizeOnly)
    {
        buf [HEADER_OFFSET]  = header_as_byte();
        buf [CODE_OFFSET]    = code_as_byte();

        if (little_endian())
        {
            buf [offset]     = (identity() >> 8 ) & 0xFF;
            buf [offset + 1] = identity() & 0xFF;
        }
        else {
            buf [offset]     = identity() & 0xFF;
            buf [offset + 1] = (identity() >> 8 ) & 0xFF;
        }
    }

    offset = TOKEN_OFFSET;

    if (token_length() > TOKEN_MAX_LENGTH)
    {
        assert(0);
        ec = make_error_code(CoapStatus::COAP_ERR_TOKEN_LENGTH);
        return;
    }

    if (token_length() != 0 && !checkBufferSizeOnly)
        memcpy (&buf[offset], token().data(), token_length());

    offset += token_length();

    exit_if_buffer_overflow(offset, size, ec, checkBufferSizeOnly);

    size_t optNumDelta = 0, optDelta = 0;

    for (auto opt : options())
    {
        size_t lengthNibble = 0, deltaNibble = 0;

        exit_if_buffer_overflow(offset, size, ec, checkBufferSizeOnly);

        optDelta = opt.number() - optNumDelta;

        deltaNibble = get_option_nibble(optDelta);

        lengthNibble = get_option_nibble(opt.value().size());

        if (!checkBufferSizeOnly)
        {
            buf [offset] = (deltaNibble << 4 | lengthNibble) & 0xFF;
        }

        offset++;

        exit_if_buffer_overflow(offset, size, ec, checkBufferSizeOnly);

        make_option(buf, size, offset, deltaNibble, optDelta, checkBufferSizeOnly, ec);
        if (ec)
        {
            assert(0);
            return;
        }

        make_option(buf, size, offset, lengthNibble, opt.value().size(), checkBufferSizeOnly, ec);
        if (ec)
        {
            assert(0);
            return;
        }

        if (!checkBufferSizeOnly)
            memcpy (&buf[offset], opt.value().data(), opt.value().size());

        offset += opt.value().size();

        exit_if_buffer_overflow(offset, size, ec, checkBufferSizeOnly);

        optNumDelta = opt.number();
    }

    if (payload().size())
    {
        exit_if_buffer_overflow(offset, size, ec, checkBufferSizeOnly);

        if (!checkBufferSizeOnly)
        {
            buf [offset] = PAYLOAD_MARKER;
        }

        offset += sizeof(PAYLOAD_MARKER);
        exit_if_buffer_overflow(offset, size, ec, checkBufferSizeOnly);
        exit_if_buffer_overflow(offset + payload().size(), size, ec, checkBufferSizeOnly);

        if (!checkBufferSizeOnly)
        {
            memcpy (&buf[offset], payload().data(), payload().size());
        }

        size = offset + payload().size();
    }
    else
    {
        size = offset;
    }
}

/* Before calling of this method you should call add_option to create needed options.
   payload should be in network bytes order
*/
void Packet::make_request(
        std::error_code &ec,
        MessageType type,   // message type
        MessageCode code,   // response code
        uint16_t id,             // message identity
        const void * payload,
        size_t payloadSize,
        std::size_t tokenLength
    )
{
    assert((payloadSize == 0 && payload == nullptr) || (payloadSize != 0 && payload != nullptr));
    assert(tokenLength <= TOKEN_MAX_LENGTH);

    ec.clear();

    if ((payloadSize != 0 && payload == nullptr)
        || (payloadSize == 0 && payload != nullptr) )
    {
        ec = make_system_error(EFAULT);
        return;
    }

    if (tokenLength > TOKEN_MAX_LENGTH)
    {
        ec = make_system_error(EINVAL);
        return;
    }

    version(COAP_VERSION);
    this->type(static_cast<std::uint8_t>(type));
    identity(id);
    code_as_byte(static_cast<std::uint8_t>(code));

    generate_token(tokenLength);

    this->payload().clear();

    const uint8_t * data = static_cast<const uint8_t *>(payload);

    if (payloadSize > 0)
    {
        for (size_t i = 0; i < payloadSize; i++)
        {
            this->payload().push_back(data[i]);
        }
    }
}

void Packet::prepare_answer(
        std::error_code &ec,
        MessageType type,   // message type
        MessageCode code,   // response code
        uint16_t id,             // message identity
        const void * payload,
        size_t payloadSize
    )
{
    assert((payloadSize == 0 && payload == nullptr) || (payloadSize != 0 && payload != nullptr));

    ec.clear();

    version(COAP_VERSION);
    this->type(static_cast<std::uint8_t>(type));
    identity(id);
    code_as_byte(static_cast<std::uint8_t>(code));

    this->payload().clear();

    const uint8_t * data = static_cast<const uint8_t *>(payload);

    if (payloadSize > 0)
    {
        for (size_t i = 0; i < payloadSize; i++)
        {
            this->payload().push_back(data[i]);
        }
    }
}

bool is_little_endian_byte_order()
{
    uint16_t test = 0x1;
    uint8_t *p = (uint8_t *)&test;
    return (*p != 0);
}

uint16_t generate_identity()
{
    static bool initialized  = false;
    if (!initialized )
    {
        srand(time(nullptr));
        initialized  = true;
    }
    return static_cast<uint16_t>(rand() % (USHRT_MAX+ 1));
}

} // namespace coap
