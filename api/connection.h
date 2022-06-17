#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "dns_resolver.h"
#include "socket.h"
#include "error.h"
#include <string>
#include <memory>
#include <cstring>

#define BUFFER_SIZE 1600UL

enum ConnectionType
{
    TCP,
    UDP,
    TLS,
    DTLS
};

inline bool is_connection_type(ConnectionType type)
{ return !(type < TCP || type > DTLS); }

extern bool uri2connection_type(const char * uri, ConnectionType &type);

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

struct Connection
{
    Connection(ConnectionType type, int port, std::shared_ptr<Buffer> bufferPtr, std::error_code &ec)
        : m_type{type},
          m_port{port},
          m_bufferPtr{std::move(bufferPtr)}
    {
        if (!is_connection_type(m_type))
        { ec = make_system_error(EINVAL); }
    }
    Connection(ConnectionType type, int port, std::error_code &ec)
        : Connection(type, port, std::move(std::make_shared<Buffer>(BUFFER_SIZE)), ec)
    {}

    virtual ~Connection() = default;
    virtual void send(const void * buffer, size_t length, const SocketAddress *destAddr, std::error_code &ec) = 0;
    virtual void receive(void * buffer, size_t &length, SocketAddress * srcAddr, std::error_code &ec, size_t seconds = 0) = 0;
    virtual void close(std::error_code &ec) = 0;

    ConnectionType type() const
    { return m_type; }

    int port() const
    { return m_port; }

    std::shared_ptr<Buffer> &bufferPtr()
    { return m_bufferPtr; }

protected:
    ConnectionType          m_type;
    int                     m_port;
    std::shared_ptr<Buffer> m_bufferPtr;
};

struct ClientConnection : public Connection
{
    ClientConnection(
            ConnectionType type,
            const char * hostname,
            int port,
            std::error_code &ec
        )
        : Connection(type, port, ec),
         m_uri{},
         m_hostname{hostname}
    {}
    ClientConnection(
            ConnectionType type,
            const char * hostname,
            int port,
            std::shared_ptr<Buffer> bufferPtr,
            std::error_code &ec
        )
        : Connection(type, port, std::move(bufferPtr), ec),
         m_uri{},
         m_hostname{hostname}
    {}
    ClientConnection(const char * uri, std::error_code &ec)
        : Connection(UDP, -1, ec),
         m_uri{uri},
         m_hostname{}
    {
        if (!uri2connection_type(uri, m_type))
        { ec = make_system_error(EINVAL); }
    }
    ClientConnection(
            const char * uri,
            std::shared_ptr<Buffer> bufferPtr,
            std::error_code &ec
        )
        : Connection(UDP, -1, std::move(bufferPtr), ec),
         m_uri{uri},
         m_hostname{}
    {
        if (!uri2connection_type(uri, m_type))
        { ec = make_system_error(EINVAL); }
    }

    virtual ~ClientConnection() = default;
    virtual void connect(std::error_code &ec) = 0;

    virtual void send(const void * buffer, size_t length, std::error_code &ec) = 0;
    virtual void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) = 0;

    std::string uri() const
    { return m_uri; }

    std::string hostname() const
    { return m_hostname; }

protected:
    std::string     m_uri;
    std::string     m_hostname;
};

struct ServerConnection : public Connection
{
    ServerConnection(ConnectionType type, int port, bool version4, std::error_code &ec)
        : Connection(type, port, ec),
         m_version4{version4}
    {}
    ServerConnection(
            ConnectionType type,
            int port, bool version4,
            std::shared_ptr<Buffer> bufferPtr,
            std::error_code &ec
        )
        : Connection(type, port, std::move(bufferPtr), ec),
         m_version4{version4}
    {}

    virtual ~ServerConnection() = default;
    virtual void bind(std::error_code &ec) = 0;
    virtual void listen(std::error_code &ec, int max_connections_in_queue = 1) = 0;
    virtual Socket * accept(std::error_code * ec = nullptr) = 0;
    virtual void send(const void * buffer, size_t length, std::error_code &ec) = 0;
    virtual void receive(void * buffer, size_t &length, std::error_code &ec, size_t seconds = 0) = 0;

    bool version4() const
    { return m_version4; }

protected:
    bool            m_version4;
};


Socket * create_socket(ConnectionType type, DnsResolver *dns, std::error_code &ec);
ClientConnection * create_client_connection(ConnectionType type, const char * hostname, int port, std::error_code &ec);
ClientConnection * create_client_connection(const char * uri, std::error_code &ec);
ServerConnection * create_server_connection(ConnectionType type, int port, bool version4, std::error_code &ec);

#endif
