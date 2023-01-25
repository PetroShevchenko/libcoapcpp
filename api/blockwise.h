#ifndef _BLOCKWISE_H
#define _BLOCKWISE_H
#include <vector>
#include <cstdint>
#include "error.h"
#include "packet.h"
#include "uri.h"

namespace coap
{

enum Blocksize
{
    BLOCK_SIZE_16,      //0
    BLOCK_SIZE_32,      //1
    BLOCK_SIZE_64,      //2
    BLOCK_SIZE_128,     //3
    BLOCK_SIZE_256,     //4
    BLOCK_SIZE_512,     //5
    BLOCK_SIZE_1024,    //6
};

enum BlockType
{
    BLOCK_TYPE_1,
    BLOCK_TYPE_2,
};

class Blockwise
{
public:
    Blockwise()
        : m_number{0},
          m_offset{0},
          m_size{0},
          m_total{0},
          m_more{false},
          m_littleEndian{is_little_endian_byte_order()}
    {}

    virtual ~Blockwise() = default;

public:
    std::uint32_t number() const
    { return m_number; }

    void number(std::uint32_t number)
    { m_number = number; }

    std::uint64_t offset() const
    { return m_offset; }

    void offset(std::uint64_t offset)
    { m_offset = offset; }

    std::uint16_t size() const
    { return m_size; }

    void size(std::uint16_t size)
    { m_size = size; }

    std::uint64_t total() const
    { return m_total; }

    void total(std::uint64_t total)
    { m_total = total; }

    bool more() const
    { return m_more; }

    void more(bool more)
    { m_more = more; }

public:
    bool get_block_option(BlockType type, Packet &pack, std::vector<Option *> &rOptions);
    bool encode_block_option(BlockType type, Option &opt);
    bool decode_size_option(const Option &opt);

protected:
    bool decode_block_option(const Option &opt);
    bool get_header (BlockType type, Packet &pack, std::error_code *ec = nullptr);
    bool set_header(
            BlockType type,
            std::uint16_t port,
            const UriPath &uriPath,
            Packet &pack,
            std::error_code *ec = nullptr
        );

protected:
    std::uint32_t m_number;  // block number
    std::uint64_t m_offset;  // offset of stored/sent file
    std::uint16_t m_size;    // block size
    std::uint64_t m_total;   // total file size
    bool m_more;             // more bit
    const bool m_littleEndian;// little endian byte order
};

class Block1 : public Blockwise
{
public:
    Block1()
        :Blockwise()
    {}

    ~Block1() = default;

public:
    bool get_header (Packet & pack, std::error_code *ec = nullptr);
    bool set_header(
            std::uint16_t port,
            const UriPath &uriPath,
            Packet &pack,
            std::error_code *ec = nullptr
        );
    bool encode_block_option(Option &opt);

private:
    bool get_block1_option(Packet &pack, std::vector<Option *> &rOptions);
};

class Block2 : public Blockwise
{
public:
    Block2()
        :Blockwise()
    {}

    ~Block2() = default;

public:
    bool get_header (Packet & pack, std::error_code *ec = nullptr);
    bool set_header(
            std::uint16_t port,
            const UriPath &uriPath,
            Packet &pack,
            std::error_code *ec = nullptr
        );
    bool encode_block_option(Option &opt);

private:
    bool get_block2_option(Packet &pack, std::vector<Option *> &rOptions);
};

Blocksize size_to_sizeoption(size_t size);

} // namespace coap

#endif
