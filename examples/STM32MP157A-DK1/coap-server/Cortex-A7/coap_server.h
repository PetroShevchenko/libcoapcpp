#ifndef _COAP_SERVER_H
#define _COAP_SERVER_H
#include "error.h"
#include "unix_udp_server.h"
#include "unix_endpoint.h"

#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <array>
#include <fstream>
#include <cstddef>
#include <thread>
#include <mutex>
#include <atomic>
#include <ctime>
#include <vector>

struct ConnectedClient : public Unix::ServerEndpoint {

    ConnectedClient(
    		const char *name,
    		const char *coreLink,
    		ServerConnection *connection,
            const SocketAddress *clientAddress,
            time_t endtime,
            std::error_code &ec
        )
        : ServerEndpoint(name, coreLink, connection, ec),
        m_clientAddress{clientAddress},
        m_endtime{endtime},
        m_threadId{}
    {}
    ~ConnectedClient() = default;

    const SocketAddress     *m_clientAddress;
    std::atomic<time_t>     m_endtime;
    std::atomic<bool>       m_processing;
    std::thread::id         m_threadId;
};

class CoapServer
{
public:
    CoapServer(
            const char *name,
            const char *coreLink,
            ServerConnection *connection,
            time_t lifetime,
            size_t maxClients
        );
    ~CoapServer();

public:
    void receive(std::error_code &ec);
    void processing(ConnectedClient* client);
    void shutdown(std::error_code &ec);
    void expired_clients_remover();

    static void processing_thread(ConnectedClient* client, void *context);
    static void expired_clients_remover_thread(void *context);

public:
    void stop()
    { m_running = false; }

    void start()
    { m_running = true; }

    bool is_started() const
    { return m_running;}

    void timeout(time_t value)
    { m_timeout = value; }

    time_t timeout() const
    { return m_timeout; }

    ServerConnection *connection()
    { return m_connection; }

private:
    ConnectedClient* 
    new_connected_client(
            const SocketAddress * clientAddr,
            std::error_code &ec
        );

    bool remove_connected_client(
                ConnectedClient * client
            );

    ConnectedClient*
    find_connected_client(
            const SocketAddress * clientAddr
        );

private:
    const char                  *m_name;
    const char                  *m_coreLink;
    ServerConnection            *m_connection;
    time_t                      m_lifetime;
    time_t                      m_timeout;
    size_t                      m_maxClients;
    std::vector<ConnectedClient*>
                                m_clients;
    std::vector<std::thread>    m_threads;
    std::thread                 m_removerThread;
    std::atomic<bool>           m_running;
    std::mutex                  m_mutex;
};

#endif
