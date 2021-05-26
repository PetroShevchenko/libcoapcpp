#include "udp_server.h"
#include "unix_socket.h"
#include "unix_dns_resolver.h"

#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstring>
#include <chrono>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std;
using namespace spdlog;

static const size_t PAYLOAD_MAX_SIZE = 4 * 1024;
static const size_t CLIENT_MAX_CONNECTIONS = 10;
static const time_t DEFAULT_LIFETIME_IN_SECONDS = 60;

// WARNING: This function allocates the memory that should be freed
Payload * Incomming::create_payload(const void * buffer, size_t size, error_code &ec)
{
    Payload *payload = new Payload();
    if (payload == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return nullptr;
    }
    payload->size = size;
    if (size && buffer)
    {
        payload->data = new char [size];
        if (payload->data == nullptr)
        {
            delete payload;
            ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
            return nullptr;
        }
        // copy data to the payload
        memcpy(payload->data, buffer, payload->size);
    }
    else
    {
        payload->size = 0;
        payload->data = nullptr;
    }
    return payload;
}

void Incomming::delete_payload(Payload ** payload)
{
    if (payload == nullptr)
        return;
    if (*payload == nullptr)
        return;
    if ((*payload)->data)
    {
        delete [] static_cast<char *>((*payload)->data);
        (*payload)->data = nullptr;
    }
    delete *payload;
    *payload = nullptr;
}

void Incomming::push_payload(const void * buffer, size_t size, error_code &ec)
{
    Payload * payload = create_payload(buffer, size, ec);
    if (ec.value())
    { return; }

    // send received data to the queue
    m_receiveQueue.push(payload, ec);

    if (ec.value())
    { delete_payload(&payload); }
}

void Incomming::clear_receive_queue()
{
    Payload * payload;
    while (!m_receiveQueue.empty())
    {
        payload = m_receiveQueue.pop();
        if (payload)
        { delete_payload(&payload); }
    }
}

UdpServer::UdpServer(int port, error_code &ec, bool version4)
    : m_port{port},
    m_version4{version4},
    m_serverAddress{new UnixSocketAddress()},
    m_serverSocket{nullptr},
    m_connections{},
    m_running{false},
    m_mutex{}
{
    set_level(level::debug);

    m_serverSocket = new UnixSocket(m_version4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0, ec);
    if (ec.value())
    {
        debug("UnixSocket error: {}", ec.message());
        return;
    }

    UnixSocketAddress *sa = reinterpret_cast<UnixSocketAddress *>(m_serverAddress);

    if (m_version4)
    {
        sa->type(SOCKET_TYPE_IP_V4);
        sa->address4().sin_family = AF_INET;
        sa->address4().sin_addr.s_addr = INADDR_ANY;
        sa->address4().sin_port = htons(port);
    }
    else
    {
        sa->type(SOCKET_TYPE_IP_V6);
        sa->address6().sin6_family = AF_INET6;
        sa->address6().sin6_addr = in6addr_any;
        sa->address6().sin6_scope_id = 0;
        sa->address6().sin6_port = htons(port);
    }

    const int on = 1; // reuse address option

    m_serverSocket->setsockoption(SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int), ec);
    if (ec.value())
    {
        debug("setsockoption error: {}", ec.message());
        return;
    }

    if (!m_version4)
    {
        const int off = 0; // disable IPv6 only
        m_serverSocket->setsockoption(IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(int), ec);
        if (ec.value())
        {
            debug("setsockoption error: {}", ec.message());
            return;
        }
    }

    m_serverSocket->bind(static_cast<const SocketAddress *>(m_serverAddress), ec);
    if (ec.value())
    {
        debug("bind error: {}", ec.message());
        return;
    }
}

UdpServer::~UdpServer()
{
    if (m_serverAddress)
    {
        delete m_serverAddress;
    }
    if (m_serverSocket)
    {
        delete m_serverSocket;
    }
}

// WARNING: This function allocates the memory that should be free
Incomming* UdpServer::new_incomming(const SocketAddress * address, error_code &ec)
{
    if (address == nullptr)
    {
        ec = make_system_error(EFAULT);
        return nullptr;
    }

    time_t futuretime = chrono::system_clock::to_time_t(
                                    chrono::system_clock::now()
                                ) + DEFAULT_LIFETIME_IN_SECONDS;

    Incomming *connection = new Incomming(
                                    address,
                                    m_serverAddress,
                                    m_serverSocket,
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

    for(vector<thread>::iterator
        iter = m_threads.begin(), last = m_threads.end(); iter != last; ++iter)
    {
        if (iter->get_id() == connection->m_threadId)
        {
            connection->m_processing = false; // flag to stop connection processing
            // kick the message queue with an empty message
            {
                error_code ec;
                connection->push_payload(nullptr, 0, ec);
                if (ec.value())
                {
                    debug("ec.message():{}",ec.message());
                    return false;
                }
            }
            // give a chance to switch the context
            std::this_thread::sleep_for (std::chrono::milliseconds(100));

            iter->join();                       // join the connection handle thread
            m_threads.erase(iter);              // remove the thread from the thread pool
            connection->clear_receive_queue();  // remove all messages from the queue

            for (vector<Incomming *>::iterator
                iter2 = m_connections.begin(), last2 = m_connections.end(); iter2 != last2; ++iter2)
            {
                if (*iter2 == connection)
                { m_connections.erase(iter2); break; } // remove the connection from the connection pool
            }

            delete connection;
            return true;
        }
    }
    return false;
}

static bool is_connection_timed_out(const Incomming * connection)
{
    if (connection == nullptr) { return false; }
    time_t currentTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
    return (currentTime >= connection->m_lifetime);
}

static void update_lifetime(Incomming * connection)
{
    if (connection == nullptr) { return; }
    connection->m_lifetime = chrono::system_clock::to_time_t(
                                    chrono::system_clock::now()
                                ) + DEFAULT_LIFETIME_IN_SECONDS;
}

Incomming* UdpServer::find_connection(const SocketAddress * address)
{
    if (address == nullptr)
    { return nullptr; }

    error_code ec;

    const UnixSocketAddress *addr = reinterpret_cast<const UnixSocketAddress *>(address);

    lock_guard<std::mutex> lg(m_mutex);

    for(auto connection : m_connections)
    {
        const UnixSocketAddress * ca = reinterpret_cast<const UnixSocketAddress *>(connection->m_clientAddress);

        if ( (ca->type() == SOCKET_TYPE_IP_V4
            && (ca->address4().sin_addr.s_addr == addr->address4().sin_addr.s_addr) )
            || (ca->type() != SOCKET_TYPE_IP_V4
            && (ca->address6().sin6_addr.s6_addr == addr->address6().sin6_addr.s6_addr) ) )
        {
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
                    const SocketAddress * address,
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
        if (m_connections.size() >= CLIENT_MAX_CONNECTIONS)
        {
            ec = make_error_code(CoapStatus::COAP_ERR_CONNECTIONS_EXCEEDED);
            debug("accept() error: {}", ec.message());
            return;
        }

        connection = new_incomming(address, ec);
        if (ec.value())
        {
            debug("new_incomming() error: {}", ec.message());
            return;
        }

        m_connections.push_back(connection); // add a new connection to the connection pool

        thread newThread(processing_thread, connection, this); // create a new thread to process the connection

        connection->m_threadId = newThread.get_id(); // map the connection and the thread

        m_threads.push_back(move(newThread)); // add a new thread to the thread pool
    }

    connection->push_payload(buffer, static_cast<size_t>(received), ec);
}

void UdpServer::connect(
                    Socket * sock,
                    void * buffer,
                    size_t buferSize,
                    ssize_t &received,
                    SocketAddress * clientAddress,
                    error_code &ec
                )
{
    set_level(level::debug);

    received = sock->recvfrom( // receive a packet from some client
                        ec,
                        buffer,
                        buferSize,
                        clientAddress
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

    static size_t iteration = 0;

    while (m_running)
    {
        debug("listening iteration {0:d}", ++iteration);

        UnixSocket * sock = reinterpret_cast<UnixSocket *>(m_serverSocket);

        FD_ZERO (&readDescriptors);
        FD_SET (sock->descriptor(), &readDescriptors);

        tv.tv_sec = DEFAULT_LIFETIME_IN_SECONDS;
        tv.tv_usec = 0;

        status = ::select (FD_SETSIZE, &readDescriptors, NULL, NULL, &tv);

        if (status == 0)
        {
            debug("select() : receive timeout {0:d} seconds is over", DEFAULT_LIFETIME_IN_SECONDS);
            continue;
        }

        if (status == -1)
        {
            debug("select() : error code {0:d}", errno);
            continue;
        }

        ssize_t received = -1;
        UnixSocketAddress clientAddress;

        // if there was received something on socket
        if (FD_ISSET(sock->descriptor(), &readDescriptors))
        {
            connect(sock, buffer, PAYLOAD_MAX_SIZE, received, &clientAddress, ec);
            if (ec.value())
            {
                debug("connect() error : {}", ec.message());
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

        Payload * payload = incomming->m_receiveQueue.front();

        if (incomming->m_processing == false        // processing is finished
            || (payload && (payload->size == 0))    // or it is a signalling message to stop processing
            || is_connection_timed_out(incomming))  // or connection is timed out
        {
            incomming->m_processing = false;
            continue;
        }

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

    if (m_serverSocket)
    {
        m_serverSocket->close(ec);
        if (ec.value())
            debug("Unable to close socket {0:d}", reinterpret_cast<const UnixSocket *>(m_serverSocket)->descriptor());
    }
}

namespace udpserver
{

Payload * receive(Incomming *connection, std::error_code &ec)
{
    if (connection == nullptr)
    {
        ec = make_system_error(EFAULT);
        return nullptr;
    }

    Payload *payload = connection->m_receiveQueue.pop();

    if (payload == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_NO_PAYLOAD);
        return nullptr;
    }
    return payload;
}

void send(Incomming *connection, Payload *payload, std::error_code &ec)
{
    if (connection == nullptr || payload == nullptr)
    {
        ec = make_system_error(EFAULT);
        return;
    }
    connection->m_serverSocket->sendto(
                                payload->data,
                                payload->size,
                                connection->m_clientAddress,
                                ec
                            );
    connection->delete_payload(&payload);
}

}// udpserver
