#ifndef _BUFFER_H
#define _BUFFER_H
#include <memory>
#include <cstring>

#define BUFFER_SIZE 1600UL

class Buffer
{
public:
    Buffer(const size_t length)
    : m_length{length},
      m_offset{0},
      m_data{new uint8_t [length]}
    { memset(m_data, 0, length); }

    ~Buffer()
    { delete [] m_data; }

    Buffer &operator=(const Buffer& other)
    {
        if (&other != this)
        {
            m_length = other.m_length;
            m_offset = other.m_offset;
            if (m_data)
                delete [] m_data;
            m_data = new uint8_t [m_length];
            memcpy(m_data, other.m_data, m_length);
        }
        return *this;
    }

    Buffer &operator=(Buffer &&other)
    {
        if (&other != this)
        {
            m_length = 0;
            m_offset = 0;
            std::swap(m_length, other.m_length);
            std::swap(m_offset, other.m_offset);
            if (m_data) 
                delete [] m_data;
            m_data = std::move(other.m_data);
        }
        return *this;        
    }

    Buffer(const Buffer& other)
    : m_length{0},
      m_offset{0},
      m_data{nullptr}
    { operator=(other); }

    Buffer(Buffer&& other)
    : m_length{0},
      m_offset{0},
      m_data{nullptr}
    { operator=(std::move(other)); }

    size_t length() const
    { return m_length; }

    uint8_t *data()
    { return m_data; }

    size_t offset() const
    { return m_offset; }

    void offset(size_t value)
    { m_offset = value; }

    void clear()
    {
        m_offset = 0;
        memset(m_data, 0, m_length);
    }

private:
    size_t      m_length;
    size_t      m_offset;
    uint8_t     *m_data;
};

#endif // _BUFFER_H
