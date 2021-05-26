#ifndef _UDP_SERVER_H
#define _UDP_SERVER_H
#include "socket.h"
#include "error.h"
#include "safe_queue.h"
#include <cstddef>
#include <thread>
#include <mutex>
#include <atomic>
#include <ctime>
#include <vector>

struct Payload
{
    size_t  size;
    char    *data;
};

struct Incomming
{
    Incomming(
            const SocketAddress *clientAddress,
            const SocketAddress *serverAddress,
            Socket *serverSocket,
            time_t lifetime
        )
        :
        m_clientAddress{clientAddress},
        m_serverAddress{serverAddress},
        m_serverSocket{serverSocket},
        m_lifetime{lifetime},
        m_receiveQueue{},
        m_processing{false},
        m_threadId{}
    {}

    virtual ~Incomming() = default;

    static Payload * create_payload(const void * buffer, size_t size, std::error_code &ec);
    static void delete_payload(Payload ** payload);

    void push_payload(const void * buffer, size_t size, std::error_code &ec);
    void clear_receive_queue();

    const SocketAddress     *m_clientAddress;
    const SocketAddress     *m_serverAddress;
    Socket                  *m_serverSocket;
    std::atomic<time_t>     m_lifetime;
    SafeQueue<Payload *>    m_receiveQueue;
    std::atomic<bool>       m_processing;
    std::thread::id         m_threadId;
};

typedef void (*ReceivedPacketHandlerCallback)(Incomming *incomming, std::error_code &ec);

class UdpServer
{
public:
    UdpServer(int port, std::error_code &ec, bool version4 = false);

    virtual ~UdpServer();

protected:
    virtual Incomming* find_connection(const SocketAddress * address);
    virtual Incomming* new_incomming(const SocketAddress * address, std::error_code &ec);
    virtual bool remove_connection(Incomming * connection);

    virtual void accept(
            const SocketAddress * address,
            const void * buffer,
            ssize_t received,
            std::error_code &ec
        );

    virtual void connect(
            Socket * sock,
            void * buffer,
            size_t buferSize,
            ssize_t &received,
            SocketAddress *clientAddress,
            std::error_code &ec
        );

public:
    virtual void listening(std::error_code &ec);

    virtual void processing(Incomming* incomming);

    virtual void shutdown(std::error_code &ec);

    static void processing_thread(Incomming* incomming, void *context);

public:
    void stop()
    { m_running = false;}

    void start()
    { m_running = true; }

    bool id_started() const
    { return m_running;}

    void set_received_packed_handler_callback(ReceivedPacketHandlerCallback callback)
    { m_callback = callback; }

protected:
    int                         m_port;
    const bool                  m_version4;
    SocketAddress               *m_serverAddress;
    Socket                      *m_serverSocket;
    std::vector<Incomming*>     m_connections;
    std::vector<std::thread>    m_threads;
    std::atomic<bool>           m_running;
    std::mutex                  m_mutex;

private:
    ReceivedPacketHandlerCallback m_callback;
};

namespace udpserver
{

Payload * receive(Incomming *connection, std::error_code &ec);
void send(Incomming *connection, Payload *payload, std::error_code &ec);

}// udpserver

#endif
