#include "blockwise.h"
#include "consts.h"
#include <cmath>
#include <cassert>
#include <cstdint>
#ifdef USE_SPDLOG
#include "spdlog/spdlog.h"
#endif
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <arpa/inet.h>
#else
#include "lwip/def.h"
#endif

#ifndef USE_SPDLOG
#define set_level(level)
#define debug(...)
#endif

using namespace std;
#ifdef USE_SPDLOG
using namespace spdlog;
#endif

namespace coap
{

const uint8_t BLOCK_SZX_MASK  = 0x7;
const uint8_t BLOCK_M_BIT     = 3;
const uint8_t BLOCK_NUM_SHIFT = 4;
const uint8_t BLOCK_MIN_SIZE = 16;
const uint16_t BLOCK_MAX_SIZE  = 1024;
const uint16_t BLOCK_SIZE_DEFAULT = BLOCK_MAX_SIZE;
const uint32_t MAX_BLOCKS = 1048576;

const uint8_t BLOCK_OPT_MIN_LEN = 1;
const uint8_t BLOCK_OPT_MAX_LEN = 3;
const uint8_t SIZE_OPT_MIN_LEN = 1;
const uint8_t SIZE_OPT_MAX_LEN = 4;

static inline bool is_block_option_length_correct(uint8_t length)
{
    return !(length < BLOCK_OPT_MIN_LEN || length > BLOCK_OPT_MAX_LEN);
}

static inline bool is_size_option_length_correct(uint8_t length)
{
    return !(length < SIZE_OPT_MIN_LEN || length > SIZE_OPT_MAX_LEN);
}

bool Blockwise::get_block_option(BlockType type, Packet &pack, vector<Option *> &rOptions)
{
    OptionNumber optNum = (type == BLOCK_TYPE_1) ? BLOCK_1 : BLOCK_2;
    return (pack.find_option(optNum, rOptions) != 0);
}

bool Blockwise::decode_block_option(const Option &opt)
{
    set_level(level::debug);

    if (opt.number() != BLOCK_1
        && opt.number() != BLOCK_2)
    {
        debug("There are no any BLOCK options");
        return false;
    }

    const size_t optLen = opt.value().size();

    if(!is_block_option_length_correct(optLen))
    {
        debug("Wrong size of option");
        return false;
    }

    {
        uint8_t szx = opt.value()[optLen - 1] & BLOCK_SZX_MASK;
        m_size = static_cast<uint16_t>(pow(2, szx + 4));
        m_more = opt.value()[optLen - 1] & (1 << BLOCK_M_BIT);
    }

    if (optLen == 1)
    {
        m_number = static_cast<uint32_t>(opt.value()[0] >> BLOCK_NUM_SHIFT);
    }
    else
    {
        if (m_littleEndian) //Little endian
        {
            m_number = static_cast<uint32_t>(opt.value()[optLen - 1] >> BLOCK_NUM_SHIFT);
            m_number |= static_cast<uint32_t>(opt.value()[optLen - 2] << BLOCK_NUM_SHIFT);
            m_number |= (optLen == 3) ? static_cast<uint32_t>(opt.value()[0] << (BLOCK_NUM_SHIFT + 8)) : 0;
        }
        else //Big endian
        {
            m_number = static_cast<uint32_t>(opt.value()[optLen - 1] << BLOCK_NUM_SHIFT);
            m_number |= static_cast<uint32_t>(opt.value()[optLen - 2] >> BLOCK_NUM_SHIFT);
            m_number |= (optLen == 3) ? static_cast<uint32_t>(opt.value()[0] >> (BLOCK_NUM_SHIFT + 8)) : 0;
        }
    }
    m_offset = m_number * m_size;
    return true;
}

bool Blockwise::decode_size_option(const Option &opt)
{
    set_level(level::debug);

    if (opt.number() != SIZE_1
        && opt.number() != SIZE_2)
    {
        debug("There are no any SIZE options");
        return false;
    }

    const size_t optLen = opt.value().size();

    if(!is_block_option_length_correct(optLen))
    {
        debug("Wrong size of option");
        return false;
    }

    if (optLen == 1)
    {
        m_total = static_cast<uint32_t>(opt.value()[0]);
    }
    else
    {
        if (m_littleEndian) //Little endian
        {
            m_total = static_cast<uint32_t>(opt.value()[optLen - 2] << 8);
            m_total |= static_cast<uint32_t>(opt.value()[optLen - 1]);
            m_total |= (optLen >= 3) ? static_cast<uint32_t>(opt.value()[optLen - 3] << 16) : 0;
            m_total |= (optLen == 4) ? static_cast<uint32_t>(opt.value()[0] << 24) : 0;
        }
        else //Big endian
        {
            m_total = static_cast<uint32_t>(opt.value()[optLen - 2] >> 8);
            m_total |= static_cast<uint32_t>(opt.value()[optLen - 1]);
            m_total |= (optLen >= 3) ? static_cast<uint32_t>(opt.value()[optLen - 3] >> 16) : 0;
            m_total |= (optLen == 4) ? static_cast<uint32_t>(opt.value()[0] >> 24) : 0;
        }
    }

    return true;
}
/*****************************************************************
 *  Note: refer RFC7959 for more details
 *
 *          0
 *          0 1 2 3 4 5 6 7
 *         +-+-+-+-+-+-+-+-+
 *         |  NUM  |M| SZX |
 *         +-+-+-+-+-+-+-+-+
 *
 *          0                   1
 *          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *         |          NUM          |M| SZX |
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *          0                   1                   2
 *          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *         |                   NUM                 |M| SZX |
 *         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *****************************************************************/
bool Blockwise::encode_block_option(BlockType type, Option &opt)
{
    set_level(level::debug);

    if (type == BLOCK_TYPE_1)
        opt.number(BLOCK_1);
    else
        opt.number(BLOCK_2);

    if (m_size % 16 || m_size > BLOCK_MAX_SIZE)
    {
        debug("Invalid block size value");
        return false;
    }

    Blocksize sizeOpt = size_to_sizeoption(m_size);
    uint8_t tmp = 0;

    if (m_number < 16) // 2**4 (4 bits)
    {
        tmp = static_cast<uint8_t>(m_number << BLOCK_NUM_SHIFT);
    }
    else if (m_number < 4096) // 2**12 (12 bits)
    {
        if (m_littleEndian)
        {
            tmp = static_cast<uint8_t>(m_number & 0xFF) >> BLOCK_NUM_SHIFT;
            tmp |= static_cast<uint8_t>((m_number >> 8) & 0xFF) << BLOCK_NUM_SHIFT;//Little endian
            opt.value().push_back(tmp);
            tmp = static_cast<uint8_t>(m_number & 0xF) << BLOCK_NUM_SHIFT;
        }
        else
        {
            tmp = static_cast<uint8_t>((m_number >> 24) & 0xFF) >> BLOCK_NUM_SHIFT;
            tmp |= static_cast<uint8_t>((m_number >> 16) & 0xFF) << BLOCK_NUM_SHIFT;//Big endian
            opt.value().push_back(tmp);
            tmp = static_cast<uint8_t>((m_number >> 24) & 0xF) << BLOCK_NUM_SHIFT;
        }
    }
    else if (m_number < MAX_BLOCKS) // 2**20 (20 bits)
    {
        if (m_littleEndian)
        {
            tmp = static_cast<uint8_t>((m_number >> 16) & 0xFF) << BLOCK_NUM_SHIFT;
            tmp |= static_cast<uint8_t>((m_number >> 8) & 0xFF) >> BLOCK_NUM_SHIFT;//Little endian
            opt.value().push_back(tmp);
            tmp = static_cast<uint8_t>((m_number >> 8) & 0xFF) << BLOCK_NUM_SHIFT;
            tmp |= static_cast<uint8_t>(m_number & 0xFF) >> BLOCK_NUM_SHIFT;
            opt.value().push_back(tmp);
            tmp = static_cast<uint8_t>(m_number & 0xF) << BLOCK_NUM_SHIFT;
        }
        else
        {
            tmp = static_cast<uint8_t>((m_number >> 8) & 0xFF) << BLOCK_NUM_SHIFT;
            tmp |= static_cast<uint8_t>((m_number >> 16) & 0xFF) >> BLOCK_NUM_SHIFT;//Big endian
            opt.value().push_back(tmp);
            tmp = static_cast<uint8_t>((m_number >> 16) & 0xFF) << BLOCK_NUM_SHIFT;
            tmp |= static_cast<uint8_t>((m_number >> 24) & 0xFF) >> BLOCK_NUM_SHIFT;
            opt.value().push_back(tmp);
            tmp = static_cast<uint8_t>((m_number >> 24) & 0xF) << BLOCK_NUM_SHIFT;
        }
    }
    else
    {
        debug("Invalid block number value");
        return false;
    }
    tmp |= static_cast<uint8_t>(sizeOpt) & BLOCK_SZX_MASK;
    if (m_more)
    { tmp |= static_cast<uint8_t>(1 << BLOCK_M_BIT); }
    else
    { tmp &= ~(static_cast<uint8_t>(1 << BLOCK_M_BIT)); }
    opt.value().push_back(tmp);

    return true;
}

bool Blockwise::get_header (BlockType type, Packet & pack, std::error_code *ec)
{
    vector<Option *> options;
    if (!get_block_option(type, pack, options))
    {
        debug("There is no BLOCK 1 option in the packet");
        return false;
    }
    if (options.size() > 1)
    {
        debug("There is more than one BLOCK 1 option in the packet");
        if (ec) *ec = make_error_code(CoapStatus::COAP_ERR_BAD_REQUEST);
        return false;
    }
    const Option opt = static_cast<const Option>(*options[0]);
    if (!decode_block_option(opt))
    {
        debug("Unable to decode BLOCK1 option");
        if (ec) *ec = make_error_code(CoapStatus::COAP_ERR_DECODE_BLOCK_OPTION);
        return false;
    }
    return true;
}

bool Blockwise::set_header(
            BlockType type,
            uint16_t port,
            const UriPath &uriPath,
            Packet &pack,
            error_code *ec
        )
{
    set_level(level::debug);
    error_code iec;
    if (ec == nullptr) ec = &iec;

    // clean all options
    pack.options().clear();
    // add option URI_PORT
    port = htons(port);
    pack.add_option(
                URI_PORT,
                (const uint8_t *)&port,
                sizeof(uint16_t),
                *ec
            );
    if (ec->value())
    {
        debug("add_option(URI_PORT) error : {}", ec->message());
        return false;
    }
    // add options URI_PATH
    for (const string &opt : uriPath.uri().asString())
    {
        pack.add_option(
                    URI_PATH,
                    (const uint8_t *)opt.c_str(),
                    opt.length(),
                    *ec
                );
        if (ec->value())
        {
            debug("add_option(URI_PATH) error : {}", ec->message());
            return false;
        }
    }
    // if it is the first block
    if (number() == 0)
    {
        size( !size() ? BLOCK_SIZE_DEFAULT : size());
        offset(0);
    }

    Option opt;
    opt.number(type == BLOCK_TYPE_1 ? BLOCK_1 : BLOCK_2);
    if (!encode_block_option(type, opt))
    {
        debug("Unable to encode BLOCK option");
        *ec = make_error_code(CoapStatus::COAP_ERR_ENCODE_BLOCK_OPTION);
        return false;
    }
    pack.add_option (
                static_cast<OptionNumber>(opt.number()),
                static_cast<const uint8_t *>(opt.value().data()),
                opt.value().size(),
                *ec
            );
    if (ec->value())
    {
        debug("add_option({}) error : {}", type == BLOCK_TYPE_1 ? "BLOCK_1" : "BLOCK_2", ec->message());
        return false;
    }
    return true;
}

bool Block1::encode_block_option(Option &opt)
{
    return Blockwise::encode_block_option(BLOCK_TYPE_1, opt);
}

bool Block1::get_header (Packet & pack, std::error_code *ec)
{
    return Blockwise::get_header (BLOCK_TYPE_1, pack, ec);
}

bool Block2::encode_block_option(Option &opt)
{
    return Blockwise::encode_block_option(BLOCK_TYPE_2, opt);
}

bool Block2::get_header (Packet & pack, std::error_code *ec)
{
    return Blockwise::get_header (BLOCK_TYPE_2, pack, ec);
}

bool Block1::set_header (uint16_t port, const UriPath &uriPath, Packet &pack, error_code *ec)
{
    return Blockwise::set_header (BLOCK_TYPE_1, port, uriPath, pack, ec);
}

bool Block2::set_header (uint16_t port, const UriPath &uriPath, Packet &pack, error_code *ec)
{
    return Blockwise::set_header (BLOCK_TYPE_2, port, uriPath, pack, ec);
}

bool Block1::get_block1_option(Packet &pack, vector<Option *> &rOptions)
{
    return get_block_option(BLOCK_TYPE_1, pack, rOptions);
}

bool Block2::get_block2_option(Packet &pack, vector<Option *> &rOptions)
{
    return get_block_option(BLOCK_TYPE_1, pack, rOptions);
}

Blocksize size_to_sizeoption(size_t size)
{
    const size_t sizeOptions [] = {
        16, 32, 64, 128, 256, 512, 1024
    };
    const size_t options = sizeof(sizeOptions)/sizeof(sizeOptions[0]);
    for (size_t opt = 0; opt < options; opt++)
    {
        if (sizeOptions[opt] == size)
            return static_cast<Blocksize>(opt);
    }
    return BLOCK_SIZE_64;// 64 by default
}

} // namespace coap
