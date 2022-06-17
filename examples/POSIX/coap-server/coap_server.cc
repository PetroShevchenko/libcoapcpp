#include "coap_server.h"
#include "error.h"
#include "unix_udp_server.h"
#include "unix_endpoint.h"

#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <array>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <getopt.h>
#include <signal.h>

using namespace std;
using namespace spdlog;
using namespace Unix;
using namespace coap;

struct CommandLineOptions
{
    bool useIPv4;
    int port;
};

static bool g_terminate = false;

static const char * g_contentPath = "data/well-known_core.wlnk";

static void usage()
{
    std::cerr << "Usage: coap-server [OPTIONS]\n";
    std::cerr << "Launch coap-server\n";
    std::cerr << "Example: ./coap-server -p 5693 -4\n";
    std::cerr << "Options:\n";
    std::cerr << "-h,--help\tshow this message and exit\n";
    std::cerr << "-p,--port\t<PORT_NUMBER>\tset the port number to listen to incomming connections. Default: 5683\n";
    std::cerr << "-4,--ipv4\tListen to only IPv4 addresses\n";
    std::cerr << "-6,--ipv6\tListen to only IPv6 addresses. Default\n";
}

static bool parse_arguments(int argc, char ** argv, CommandLineOptions &options)
{
    int opt;
    char * endptr;

    options.port = 5683;
    options.useIPv4 = false;
    set_level(level::debug);
    while(true)
    {
        int option_index = 0;// getopt_long stores the option index here
        static struct option long_options[] = {
                {"help", no_argument, 0, 'h'},
                {"port", required_argument, 0, 'p'},
                {"ipv4", no_argument, 0, '4'},
                {"ipv6", no_argument, 0, '6'},
                {0, 0, 0, 0}
        };
        opt = getopt_long (argc, argv, "hp:46", long_options, &option_index);
        if (opt == -1) break;

        switch (opt)
        {
            case '?':
            case 'h':
                usage();
                return false;
            case 'p':
                options.port = (int)strtol(optarg, &endptr, 10);
                if (options.port <= 0) {
                    debug("Error: Unable to convert --port {} option value to port number", optarg);
                    return false;
                }
                break;
            case '4':
                options.useIPv4 = true;
                break;
            case '6':
                options.useIPv4 = false;
                break;
            default:
                return false;
        }
    }
    return true;
}

static bool read_core_link_content(const char * filePath, string &content)
{
    ifstream ifs(filePath);

    if (ifs.is_open())
    {
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg();
        content.reserve(size);
        ifs.seekg(0);
        ifs.read(&content[0], size);
        ifs.close();
        return true;
    }
    return false;
}

static void signal_handler(int signo)
{
    if (signo == SIGINT)
    {
        debug("SIGINT cought");
        g_terminate = true;
    }
}

int main(int argc, char **argv)
{
    set_level(level::debug);
    debug("{} has been started", argv[0]);

    CommandLineOptions options;
    if (!parse_arguments(argc, argv, options))
    {
        debug("parse_arguments() failed");
        return EXIT_FAILURE;
    }
    debug("port: {0:d}", options.port);
    debug("use IPv4: {}", options.useIPv4);

    string coreLinkContent;

    if (!read_core_link_content(g_contentPath, coreLinkContent))
    {
        debug("read_core_link_content() failed");
        return EXIT_FAILURE;        
    }
    debug("core link content:\n{}", coreLinkContent.c_str());

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        debug("Unable to set SIGINT handler");
        return EXIT_FAILURE;
    }

    error_code ec;
    int status;
    fd_set rd;
    struct timeval tv;
    const UnixSocket *sock;
    shared_ptr<Buffer> bufferPtr;

    bufferPtr = make_shared<Buffer>(BUFFER_SIZE);

    debug("creating a new connection...");

    UdpServerConnection connection(options.port, options.useIPv4, move(bufferPtr), ec);
    if (ec.value())
    {
        debug("FAILED\nerror occured : {}", ec.message());
        return EXIT_FAILURE;
    }
    debug("OK");

    debug("binding socket with IP and port...");
    connection.bind(ec);
    if (ec.value())
    {
        debug("FAILED\nerror occured : {}", ec.message());
        return EXIT_FAILURE;
    }
    debug("OK");

    sock = static_cast<const UnixSocket *>(connection.socket());

    debug("creating a new CoAP server...");

    CoapServer server("CoAP Server", coreLinkContent.c_str(), &connection, 60, 8);
    if (ec.value())
    {
        debug("FAILED\nerror occured : {}", ec.message());
        return EXIT_FAILURE;        
    }
    debug("OK");

    debug("server is launching...");
    server.start();
    debug("OK");

    while(!g_terminate && server.is_started())
    {
        FD_ZERO (&rd);
        FD_SET (sock->descriptor(), &rd);

        tv.tv_sec = server.timeout();
        tv.tv_usec = 0;

        status = select(FD_SETSIZE, &rd, NULL, NULL, &tv);

        if (status == 0)
        {
            debug("select() timeout {0:d} sec expired", server.timeout());
        }
        else if (status < 0)
        {
            debug("select() error, errno = {0:d}", errno);
            server.stop();
            continue;
        }

        if (FD_ISSET(sock->descriptor(), &rd))
        {
            server.receive(ec);
            if (ec.value())
            {
                debug("server.receive() failed: {}", ec.message());    
            }
            else
            {
                debug("server.receive() <-- ({0:d})", server.connection()->bufferPtr().get()->offset()); 
            }
        }
    }

    server.shutdown(ec);
    if (ec.value())
    {
        debug("shutdown() failed: {}", ec.message());        
    }

    debug("{} has been finished", argv[0]);

    return EXIT_SUCCESS;
}

///////////////////////////////////////////////////////////////////
//
// CoapServer class methods
//
///////////////////////////////////////////////////////////////////

CoapServer::CoapServer(
        const char *name,
        const char *coreLink,
        ServerConnection *connection,
        time_t lifetime,
        size_t maxClients
    )
    : m_name{name},
      m_coreLink{coreLink},
      m_connection{connection},
      m_lifetime{lifetime},
      m_timeout{1}, // 1 sec
      m_maxClients{maxClients},
      m_clients{},
      m_threads{},
      m_running{false},
      m_mutex{}  
{
    // start remover thread
    thread newThread(expired_clients_remover_thread, this);
    m_removerThread = move(newThread);
}

CoapServer::~CoapServer()
{
    if (m_removerThread.joinable()) {
        m_removerThread.join();
    }
}

// WARNING: This function allocates the memory that should be free
ConnectedClient* 
CoapServer::new_connected_client(
                const SocketAddress * clientAddr,
                error_code &ec
            )
{
    if (clientAddr == nullptr)
    {
        ec = make_system_error(EFAULT);
        return nullptr;
    }

    time_t futuretime = chrono::system_clock::to_time_t(
                                    chrono::system_clock::now()
                                ) + m_lifetime;

    ConnectedClient *client = new ConnectedClient(
                                    m_name,
                                    m_coreLink,
                                    m_connection,
                                    clientAddr,
                                    futuretime,
                                    ec
                                );
    if (client == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return nullptr;
    }

    if (ec.value()) return nullptr;

    client->m_processing = true;

    return client;
}

bool CoapServer::remove_connected_client(ConnectedClient * client)
{
    if (client == nullptr) { return false; }

    for(vector<thread>::iterator
        iter = m_threads.begin(), last = m_threads.end(); iter != last; ++iter)
    {
        if (iter->get_id() == client->m_threadId)
        {
            client->m_processing = false; // flag to stop client processing 

            // give a chance to switch the context
            std::this_thread::sleep_for (std::chrono::milliseconds(100));

            if (iter->joinable())
                iter->join();                   // join the client handle thread
            m_threads.erase(iter);              // remove the thread from the thread pool
            
            hash<thread::id> hasher;
            debug("[THREAD] [{0:d}] : The thread has been finished", hasher(client->m_threadId));

            for (vector<ConnectedClient*>::iterator
                iter2 = m_clients.begin(), last2 = m_clients.end(); iter2 != last2; ++iter2)
            {
                if (*iter2 == client)
                { m_clients.erase(iter2); break; } // remove the client from the client pool
            }

            delete client;
            return true;
        }
    }
    return false;
}

static bool is_client_connection_timed_out(const ConnectedClient * client)
{
    if (client == nullptr) { return false; }
    time_t currentTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
    return (currentTime >= client->m_endtime);
}

static void update_client_connection_endtime(ConnectedClient * client, const time_t lifetime)
{
    if (client == nullptr) { return; }
    client->m_endtime = chrono::system_clock::to_time_t(
                                    chrono::system_clock::now()
                                ) + lifetime;
}

ConnectedClient* CoapServer::find_connected_client(const SocketAddress * clientAddr)
{
    if (clientAddr == nullptr)
    { return nullptr; }

    error_code ec;

    const UnixSocketAddress *addr = reinterpret_cast<const UnixSocketAddress *>(clientAddr);

    lock_guard<std::mutex> lg(m_mutex);

    for(auto client : m_clients)
    {
        const UnixSocketAddress * ca = reinterpret_cast<const UnixSocketAddress *>(client->m_clientAddress);

        if ( (ca->type() == SOCKET_TYPE_IP_V4
            && (ca->address4().sin_addr.s_addr == addr->address4().sin_addr.s_addr) )
            || (ca->type() != SOCKET_TYPE_IP_V4
            && (ca->address6().sin6_addr.s6_addr == addr->address6().sin6_addr.s6_addr) ) )
        {
            // if the client connection is timed out remove it
            if (is_client_connection_timed_out(client))
            {
                remove_connected_client(client);
                return nullptr;
            }
            else
            { return client; }
        }
    }
    return nullptr;
}

void CoapServer::receive(error_code &ec)
{
    set_level(level::debug);

    if (m_connection->type() != UDP) { // Only UDP connection is supported
        ec = make_error_code(CoapStatus::COAP_ERR_NOT_IMPLEMENTED);
        return;
    }

    UdpServerConnection *connection = reinterpret_cast<UdpServerConnection*>(m_connection);
    uint8_t *buffer = connection->bufferPtr().get()->data();
    size_t length = connection->bufferPtr().get()->length();
    UnixSocketAddress clientAddress;

    connection->receive(buffer, length, &clientAddress, ec); // Receive a message from a client 
    if (ec.value())
    {
        debug("receive() : error : {}", ec.message());
        return;
    }

    connection->bufferPtr().get()->offset(length); // set the received message length

    ConnectedClient * client = find_connected_client(&clientAddress);// looking for a client among the known ones

    if (client == nullptr) // it is a new client
    {
        if (m_clients.size() >= m_maxClients)
        {
            ec = make_error_code(CoapStatus::COAP_ERR_CONNECTIONS_EXCEEDED);
            debug("processing() error: {}", ec.message());
            return;
        }

        client = new_connected_client(&clientAddress, ec);
        if (ec.value())
        {
            debug("new_connected_client() error: {}", ec.message());
            return;
        }

        m_clients.push_back(client); // add a new client to the client pool

        thread newThread(processing_thread, client, this); // create a new thread to process the client request

        client->m_threadId = newThread.get_id(); // map the client and the thread

        m_threads.push_back(move(newThread)); // add a new thread to the thread pool

        hash<thread::id> hasher;

        debug("[THREAD] [{0:d}] : A new thread has been started", hasher(client->m_threadId));
    }

    lock_guard<std::mutex> lg(client->mutex());   

    client->buffer() = *connection->bufferPtr(); // copy recived data to the internal buffer of the apropriate client

    const sockaddr_in * addr = &((reinterpret_cast<const UnixSocketAddress *>(client->m_clientAddress))->address4());
    const struct in_addr  * sin_addr = &addr->sin_addr;

    debug("Client IP address: {0:d}", sin_addr->s_addr);

    client->received(true);
}

void CoapServer::processing(ConnectedClient* client)
{
    error_code ec;
    set_level(level::debug);

    if (client == nullptr)
    {
        ec = make_system_error(EFAULT);
        debug("processing() error : {}", ec.message());
        return;
    }

    while (client->m_processing)
    {
        if (client->m_processing == false               // processing is finished
            || is_client_connection_timed_out(client))  // or connection is timed out
        {
            client->m_processing = false;
            continue;
        }

        if (client->received()) {
            client->received(false);// to debug
            update_client_connection_endtime(client, m_lifetime);
        }

        client->transaction_step(ec);

        if (ec.value())
        {
            debug("transaction_step error : {}", ec.message());
        }

        client->timeout(10);// to debug

        if (client->timeout())
            std::this_thread::sleep_for(std::chrono::seconds(client->timeout()));
    }
}

void CoapServer::processing_thread(ConnectedClient* client, void *context)
{
    ((CoapServer *)context)->processing(client);
}

void CoapServer::shutdown(error_code &ec)
{
    for (auto client : m_clients)
    {
        if (!remove_connected_client(client))
        {
            ec = make_error_code(CoapStatus::COAP_ERR_REMOVE_CONNECTION);
            debug("client can not be removed");
        }
    }
}

void CoapServer::expired_clients_remover()
{
    while(m_running)
    {
        for(vector<ConnectedClient*>::iterator iter = m_clients.begin(),
                last = m_clients.end(); iter != last; ++iter)
        {
            if (is_client_connection_timed_out(static_cast<const ConnectedClient*>(*iter)))
            {
                remove_connected_client(*iter);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void CoapServer::expired_clients_remover_thread(void *context)
{
    ((CoapServer *)context)->expired_clients_remover();
}
