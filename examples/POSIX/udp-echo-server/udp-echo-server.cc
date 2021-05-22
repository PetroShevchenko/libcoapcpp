#include <iostream>
#include <string>
#include "spdlog/spdlog.h"
#include <spdlog/fmt/fmt.h>
#include "socket.h"
#include "unix_socket.h"
#include "unix_dns_resolver.h"
#include "error.h"
#include "common/safe_queue.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <vector>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;
using namespace spdlog;

static const size_t PAYLOAD_MAX_SIZE = 4 * 1024;
static const size_t CLIENT_MAX_CONNECTIONS = 10;
static const time_t DEFAULT_LIFETIME_IN_SECONDS = 60;


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
            Socket *serverSocket4,
            Socket *serverSocket6,
            time_t lifetime
        )
        :
        m_clientAddress{clientAddress},
        m_serverAddress{serverAddress},
        m_serverSocket4{serverSocket4},
        m_serverSocket6{serverSocket6},
        m_lifetime{lifetime},
        m_receiveQueue{},
        m_processing{false},
        m_threadId{}
    {}

    ~Incomming() = default;

    void push_payload(const void * buffer, size_t size, error_code &ec);
    void clear_receive_queue();

    const SocketAddress     *m_clientAddress;
    const SocketAddress     *m_serverAddress;
    Socket                  *m_serverSocket4;
    Socket                  *m_serverSocket6;
    atomic<time_t>          m_lifetime;
    SafeQueue<Payload *>    m_receiveQueue;
    atomic<bool>            m_processing;
    thread::id              m_threadId;
};

// WARNING: This function allocates the memory that should be free
void Incomming::push_payload(const void * buffer, size_t size, error_code &ec)
{
    Payload *payload = new Payload();
    if (payload == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return;
    }

    payload->size = size;
    payload->data = new char [payload->size];

    if (payload->data == nullptr)
    {
        delete [] payload;
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return;
    }
    // copy received data to the payload
    memcpy(payload->data, buffer, payload->size);

    // send received data to the queue
    m_receiveQueue.push(payload, ec);

    if (ec.value())
    {
        delete [] (char *)payload->data;
        delete payload;
    }
}

void Incomming::clear_receive_queue()
{
    Payload * payload;
    while (!m_receiveQueue.empty())
    {
        payload = m_receiveQueue.pop();
        if (payload)
        {
            if (payload->data)
                delete [] payload->data;
            delete payload;
        }
    }
}

void clear_payload(Payload * payload)
{
    if (payload == nullptr)
        return;
    if (payload->data)
        delete [] payload->data;
    delete payload;
}

typedef void (*ReceivedPacketHandlerCallback)(Incomming *incomming, error_code &ec);

class UdpServer
{
public:
    UdpServer(int port, error_code &ec);

    virtual ~UdpServer();

protected:
    virtual Incomming* find_connection(const UnixSocketAddress &addr);
    virtual Incomming* new_incomming(const UnixSocketAddress &addr, error_code &ec);
    virtual bool remove_connection(Incomming * connection);

    void accept(
            const UnixSocketAddress &address,
            const void * buffer,
            ssize_t received,
            error_code &ec
        );

    void connect(
            UnixSocket * sock,
            void * buffer,
            size_t buferSize,
            ssize_t &received,
            UnixSocketAddress &clientAddress,
            error_code &ec
        );

public:
    virtual void listening(error_code &ec);

    virtual void processing(Incomming* incomming);

    virtual void shutdown(error_code &ec);

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
    int m_port;
    SocketAddress               *m_serverAddress;
    Socket                      *m_serverSocket4;
    Socket                      *m_serverSocket6;
    vector<Incomming*>          m_connections;
    vector<thread>              m_threads;
    atomic<bool>                m_running;
    mutex                       m_mutex;

private:
    ReceivedPacketHandlerCallback m_callback;
};

UdpServer::UdpServer(int port, error_code &ec)
    : m_port{port},
    m_serverAddress{new UnixSocketAddress()},
    m_serverSocket4{nullptr},
    m_serverSocket6{nullptr},
    m_connections{},
    m_running{false},
    m_mutex{}
{
    set_level(level::debug);

#if 0
    // prepare to listening of IPv4
    m_serverSocket4 = new UnixSocket(AF_INET, SOCK_DGRAM, 0, ec);
    if (ec.value())
    {
        debug("UnixSocket error: {}", ec.message());
        return;
    }

    UnixSocketAddress *sa = reinterpret_cast<UnixSocketAddress *>(m_serverAddress);
    sa->address4().sin_family = AF_INET;
    sa->address4().sin_addr.s_addr = INADDR_ANY;
    sa->address4().sin_port = htons(port);

    const int on = 1; // reuse address option

    m_serverSocket4->setsockoption(SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int), ec);
    if (ec.value())
    {
        debug("setsockoption error: {}", ec.message());
        return;
    }

    m_serverSocket4->bind(static_cast<const SocketAddress *>(m_serverAddress), ec);
    if (ec.value())
    {
        debug("bind error: {}", ec.message());
        return;
    }
#endif
    const int on = 1; // reuse address option
    const int off = 0; // disable IPv6 only

    // prepare to listening of IPv6
    m_serverSocket6 = new UnixSocket(AF_INET6, SOCK_DGRAM, 0, ec);
    if (ec.value())
    {
        debug("UnixSocket error: {}", ec.message());
        return;
    }

    UnixSocketAddress *sa = reinterpret_cast<UnixSocketAddress *>(m_serverAddress);

    sa->address6().sin6_family = AF_INET6;
    sa->address6().sin6_addr = in6addr_any;
    sa->address6().sin6_scope_id = 0;
    //memcpy(sa->address6().sin6_addr.s6_addr, in6addr_any.s6_addr, sizeof(in6addr_any.s6_addr));
    sa->address6().sin6_port = htons(port);

    m_serverSocket6->setsockoption(SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int), ec);
    if (ec.value())
    {
        debug("setsockoption error: {}", ec.message());
        return;
    }

    m_serverSocket6->setsockoption(IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(int), ec);
    if (ec.value())
    {
        debug("setsockoption error: {}", ec.message());
        return;
    }

    m_serverSocket6->bind(static_cast<const SocketAddress *>(m_serverAddress), ec);
    if (ec.value())
    {
        debug("bind error: {}", ec.message());
        return;
    }

    start();
}

UdpServer::~UdpServer()
{
    if (m_serverAddress)
    {
        delete m_serverAddress;
    }
    if (m_serverSocket4)
    {
        delete m_serverSocket4;
    }
    if (m_serverSocket6)
    {
        delete m_serverSocket6;
    }
}

// WARNING: This function allocates the memory that should be free
Incomming* UdpServer::new_incomming(const UnixSocketAddress &addr, error_code &ec)
{
    time_t futuretime = chrono::system_clock::to_time_t(
                                    chrono::system_clock::now()
                                ) + DEFAULT_LIFETIME_IN_SECONDS;

    Incomming *connection = new Incomming(
                                    &addr,
                                    m_serverAddress,
                                    m_serverSocket4,
                                    m_serverSocket6,
                                    futuretime
                                );
    if (connection == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return nullptr;
    }

    connection->m_processing = true;

    return connection;
}

bool UdpServer::remove_connection(Incomming * connection)
{
    if (connection == nullptr) { return false; }
    //lock_guard<std::mutex> lg(m_mutex);
    for(vector<thread>::iterator
        iter = m_threads.begin(), last = m_threads.end(); iter != last; ++iter)
    {
        if (iter->get_id() == connection->m_threadId)
        {
            connection->m_processing = false;
            iter->join();
            m_threads.erase(iter);
            connection->clear_receive_queue();
            return true;
        }
    }
    return false;
}

static bool is_connection_timed_out(const Incomming * connection)
{
    if (connection == nullptr) { return false; }
    time_t currentTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
    return (connection->m_lifetime >= currentTime);
}

static void update_lifetime(Incomming * connection)
{
    if (connection == nullptr) { return; }
    connection->m_lifetime = chrono::system_clock::to_time_t(
                                    chrono::system_clock::now()
                                ) + DEFAULT_LIFETIME_IN_SECONDS;
}

Incomming* UdpServer::find_connection(const UnixSocketAddress &addr)
{
    error_code ec;

    lock_guard<std::mutex> lg(m_mutex);

    for(auto connection : m_connections)
    {
        const UnixSocketAddress * ca = reinterpret_cast<const UnixSocketAddress *>(connection->m_clientAddress);
        debug("ca->type() = {0:d}", ca->type());

        if ( (ca->type() == SOCKET_TYPE_IP_V4
            && (ca->address4().sin_addr.s_addr == addr.address4().sin_addr.s_addr) )
            || (ca->type() != SOCKET_TYPE_IP_V4
            && (ca->address6().sin6_addr.s6_addr == addr.address6().sin6_addr.s6_addr) ) )
        {
            debug("ca->address4().sin_addr.s_addr = {0:d}", ca->address4().sin_addr.s_addr);
            debug("addr.address4().sin_addr.s_addr = {0:d}", addr.address4().sin_addr.s_addr);
            debug("ca->address6().sin6_addr.s6_addr = {}", ca->address6().sin6_addr.s6_addr);
            debug("addr.address6().sin6_addr.s6_addr = {}", addr.address6().sin6_addr.s6_addr);

            // if the connection is timed out remove it
            if (is_connection_timed_out(connection))
            {
                remove_connection(connection);
                return nullptr;
            }
            else
            { return connection; }
        }
    }
    return nullptr;
}

void UdpServer::accept(
                    const UnixSocketAddress &address,
                    const void * buffer,
                    ssize_t received,
                    error_code &ec
                )
{
    if (buffer == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }

    if (received <= 0)
    {
        ec = make_system_error(EINVAL);
        return;
    }

    Incomming *connection = find_connection(address);// looking for a client among known ones

    if (connection == nullptr) // it is a new client
    {
        connection = new_incomming(address, ec);
        if (ec.value())
        {
            debug("new_incomming() : error : {}", ec.message());
            return;
        }

        m_connections.push_back(connection);

        thread newThread(processing_thread, connection, this);

        connection->m_threadId = newThread.get_id();

        m_threads.push_back(move(newThread));
    }

    connection->push_payload(buffer, static_cast<size_t>(received), ec);
}

void UdpServer::connect(
                    UnixSocket * sock,
                    void * buffer,
                    size_t buferSize,
                    ssize_t &received,
                    UnixSocketAddress &clientAddress,
                    error_code &ec
                )
{
    set_level(level::debug);

    received = sock->recvfrom( // receive a packet from some client
                        ec,
                        buffer,
                        buferSize,
                        &clientAddress
                    );
    if (ec.value())
    {
        debug("recvfrom() : error : {}", ec.message());
        return;
    }

    accept(
        clientAddress,
        buffer,
        received,
        ec
    );
    if (ec.value())
    {
        debug("accept() : error : {}", ec.message());
    }
}

void UdpServer::listening(error_code &ec)
{
    int status;
    fd_set readDescriptors;
    struct timeval tv;
    set_level(level::debug);

    debug("start listening...");

    char * buffer = new char [PAYLOAD_MAX_SIZE];

    if (buffer == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return;
    }

    while (m_running)
    {
        debug("listening iteration");

        //UnixSocket * sock4 = reinterpret_cast<UnixSocket *>(m_serverSocket4);
        UnixSocket * sock6 = reinterpret_cast<UnixSocket *>(m_serverSocket6);

        FD_ZERO (&readDescriptors);
        //FD_SET (sock4->descriptor(), &readDescriptors);
        FD_SET (sock6->descriptor(), &readDescriptors);

        tv.tv_sec = DEFAULT_LIFETIME_IN_SECONDS;
        tv.tv_usec = 0;

        status = ::select (FD_SETSIZE, &readDescriptors, NULL, NULL, &tv);

        if (status == 0)
        {
            debug("select() : receive timeout {0:d} seconds is over", tv.tv_sec);
            continue;
        }

        if (status == -1)
        {
            debug("select() : error code {0:d}", errno);
            continue;
        }

        ssize_t received = -1;
        UnixSocketAddress clientAddress;
#if 0
        // if there was received something on IPv4 socket
        if (FD_ISSET(sock4->descriptor(), &readDescriptors))
        {
            connect(sock4, buffer, PAYLOAD_MAX_SIZE, received, clientAddress, ec);
            if (ec.value())
            {
                debug("IPv4 connect() error : {}", ec.message());
            }
        }
#endif
        // if there was received something on IPv6 socket
        if (FD_ISSET(sock6->descriptor(), &readDescriptors))
        {
            connect(sock6, buffer, PAYLOAD_MAX_SIZE, received, clientAddress, ec);
            if (ec.value())
            {
                debug("IPv6 connect() error : {}", ec.message());
            }
        }
    } // while

    delete [] buffer;
    ec.clear();
}

void UdpServer::processing(Incomming* incomming)
{
    error_code ec;
    set_level(level::debug);

    if (incomming == nullptr)
    {
        ec = make_system_error(EFAULT);
        debug("processing() error : {}", ec.message());
        return;
    }

    while (incomming->m_processing)
    {
        incomming->m_receiveQueue.wait_wail_empty();

        update_lifetime(incomming);

        if (m_callback)
            m_callback(incomming, ec);

        if (ec.value())
        {
            debug("m_callback error : {}", ec.message());
        }
    }
}

void UdpServer::processing_thread(Incomming* incomming, void *context)
{
    ((UdpServer *)context)->processing(incomming);
}

void UdpServer::shutdown(error_code &ec)
{
    for (auto connection : m_connections)
    {
        if (!remove_connection(connection))
        {
            ec = make_error_code(CoapStatus::COAP_ERR_REMOVE_CONNECTION);
            debug("connection can not be removed");
        }
    }

    if (m_serverSocket4)
    {
        m_serverSocket4->close(ec);
        if (ec.value())
            debug("Unable to close IPv4 socket {0:d}", reinterpret_cast<const UnixSocket *>(m_serverSocket4)->descriptor());
    }

    if (m_serverSocket6)
    {
        m_serverSocket6->close(ec);
        if (ec.value())
            debug("Unable to close IPv6 socket {0:d}", reinterpret_cast<const UnixSocket *>(m_serverSocket6)->descriptor());
    }
}

static void udp_echo_server_packet_handler(Incomming* incomming, error_code &ec)
{
    Payload *payload = incomming->m_receiveQueue.pop();

    if (payload == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NO_PAYLOAD);
        return;
    }

    if (incomming->m_clientAddress->type() == SOCKET_TYPE_IP_V4)
    {
        incomming->m_serverSocket4->sendto(
                                    payload->data,
                                    payload->size,
                                    incomming->m_clientAddress,
                                    ec
                                );
    }
    else
    {
        debug("IPv6 address : {}");
        fmt::print("{:02x}", fmt::join(reinterpret_cast<const UnixSocketAddress *>(incomming->m_clientAddress)->address6().sin6_addr.s6_addr, ", "));
        fmt::print("\n");
        debug("IPv6 port : {0:d}", reinterpret_cast<const UnixSocketAddress *>(incomming->m_clientAddress)->address6().sin6_port);
        incomming->m_serverSocket6->sendto(
                                    payload->data,
                                    payload->size,
                                    incomming->m_clientAddress,
                                    ec
                                );
    }
    clear_payload(payload);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    set_level(level::debug);
    debug("udp-echo-server");

    error_code ec;

    UdpServer server(5683, ec);
    if (ec.value())
    {
        debug("UdpServer constructor error: {}", ec.message());
        return 1;
    }

    server.set_received_packed_handler_callback(udp_echo_server_packet_handler);

    server.listening(ec);
    if (ec.value())
    {
        debug("listening error: {}", ec.message());
    }

    server.shutdown(ec);
    if (ec.value())
    {
        debug("shutdown error: {}", ec.message());
    }

    return 0;
}
