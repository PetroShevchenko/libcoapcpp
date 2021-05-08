#include <cmath>
#include <cassert>
#include <cstdint>
#include "spdlog/spdlog.h"
#include "blockwise.h"
#include "consts.h"

using namespace std;
using namespace spdlog;

namespace coap
{

const uint8_t BLOCK_SZX_MASK  = 0x7;
const uint8_t BLOCK_M_BIT     = 3;
const uint8_t BLOCK_NUM_SHIFT = 4;
const uint16_t BLOCK_MAX_SIZE  = 2048;
const uint16_t BLOCK_SIZE_DEFAULT = BLOCK_MAX_SIZE;

const uint8_t BLOCK_OPT_MIN_LEN = 1;
const uint8_t BLOCK_OPT_MAX_LEN = 3;

static inline bool is_block_option_length_correct(uint8_t length)
{ return !(length < BLOCK_OPT_MIN_LEN || length > BLOCK_OPT_MAX_LEN); }


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
        if (is_little_endian_byte_order()) //Little endian
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

    return true;
}

bool Blockwise::decode_size_option(const Option &opt)
{
    return false;//TODO
}

bool Blockwise::encode_block_option(Option &opt)
{
    return false;//TODO
}

bool Block1::get_header (const Packet & pack)
{
    return false;//TODO
}

bool Block1::set_header (std::uint16_t port, const UriPath &uri, Packet &pack)
{
    return false;//TODO
}

bool Block1::get_block1_option(const Packet &pack, size_t &optionQuantity, const Option ** opt)
{
    return false;//TODO
}

bool Block2::get_header (const Packet & pack)
{
    return false;//TODO
}

bool Block2::set_header (std::uint16_t port, const UriPath &uri, Packet &pack)
{
    return false;//TODO
}

bool Block2::get_block2_option(const Packet &pack, size_t &optionQuantity, const Option ** opt)
{
    return false;//TODO
}


Blocksize size_to_sizeoption(size_t size)
{
    const size_t sizeOptions [] = {
        16, 32, 64, 128, 256, 512, 1024, 2048
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
