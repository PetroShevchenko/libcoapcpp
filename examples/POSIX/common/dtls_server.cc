#include "dtls_server.h"
#include "error.h"
#include "wolfssl_error.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;
using namespace spdlog;

SecureIncomming(
        const SocketAddress *clientAddress,
        const SocketAddress *serverAddress,
        Socket *serverSocket,
        time_t lifetime,
        std::error_code &ec
    )
    : Incoming(
        clientAddress,
        serverAddress,
        serverSocket,
        lifetime
    ),
    m_ssl{nullptr},
    m_pipeFd{-1,-1}
{
    if (pipe(m_pipeFd) < 0)
    { ec = make_system_error(errno); }
}

SecureIncomming::~SecureIncomming()
{
    if (m_pipeFd[0] != -1)
    {
        close(m_pipeFd[0]);
        m_pipeFd[0] = -1;
    }
    if (m_pipeFd[1] != -1)
    {
        close(m_pipeFd[1])
        m_pipeFd[1] = -1;
    }
}

DtlsServer::DtlsServer(int port, std::error_code &ec, bool version4)
    : UdpServer(port, ec, version4),
      m_ctx{nullptr}
{
    set_level(level::debug);
    if(ec.value())
    { debug("UdpServer() error"); return; }

    ec = make_error_code(CoapStatus::COAP_ERR_DTLS_CTX_INIT);

    /* Initialize wolfSSL */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS)
    { debug("wolfSSL_Init() error"); return; }

    wolfSSL_Debugging_ON();

    /* Create and initialize WOLFSSL_CTX */
    m_ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

    if (m_ctx == NULL)
    { debug("wolfSSL_CTX_new() error"); return; }

    if (wolfSSL_CTX_load_verify_locations(
                m_ctx, "../../../third-party/wolfssl/certs/ca-cert.pem", 0) != SSL_SUCCESS)
    { debug("wolfSSL_CTX_load_verify_locations() error"); return; }

    if (wolfSSL_CTX_use_certificate_file(
                m_ctx, "../../../third-party/wolfssl/certs/server-cert.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS)
    { debug("wolfSSL_CTX_use_certificate_file() error"); return; }

    if (wolfSSL_CTX_use_PrivateKey_file(
                m_ctx, "../../../third-party/wolfssl/certs/server-key.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS)
    { debug("wolfSSL_CTX_use_PrivateKey_file() error"); return; }

    ec.clear();
}

bool DtlsServer::remove_connection(Incomming * connection)
{
    if (connection == nullptr) { return false; }

    SecureIncomming *sc = reinterpret_cast<SecureIncomming *>(connection);

    for(vector<thread>::iterator
        iter = m_threads.begin(), last = m_threads.end(); iter != last; ++iter)
    {
        if (iter->get_id() == sc->m_threadId)
        {
            sc->m_processing = false; // flag to stop connection processing

            //send something to the pipe to wake up select() and stop thread
            uint8_t flag = 1;
            if (::write(sc->m_pipeFd[1], &flag, sizeof(flag)) < 0)
            { return false; }

            // give a chance to switch the context
            std::this_thread::sleep_for (std::chrono::milliseconds(100));

            iter->join();                       // join the connection handle thread
            m_threads.erase(iter);              // remove the thread from the thread pool

            for (vector<SecureIncomming *>::iterator
                iter2 = m_connections.begin(), last2 = m_connections.end(); iter2 != last2; ++iter2)
            {
                if (*iter2 == sc)
                { m_connections.erase(iter2); break; } // remove the connection from the connection pool
            }

            delete sc;
            return true;
        }
    }
    return false;
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

    Incomming *connection = new SecureIncomming(
                                    address,
                                    m_serverAddress,
                                    m_serverSocket,
                                    futuretime,
                                    ec
                                );
    if (connection == nullptr)
    {
        ec = make_error_code(CoapStatus::COAP_ERR_MEMORY_ALLOCATE);
        return nullptr;
    }
    if (ec.value())
    { return nullptr; }

    connection->m_processing = true;

    return connection;
}

void DtlsServer::accept(
        const SocketAddress * address,
        const void * buffer,
        ssize_t received,
        std::error_code &ec
    )
{
    //TODO
}

void DtlsServer::connect(
        Socket * sock,
        void * buffer,
        size_t buferSize,
        ssize_t &received,
        SocketAddress *clientAddress,
        std::error_code &ec
    )
{
    //TODO
}

void DtlsServer::listening(std::error_code &ec)
{
    //TODO
}

void DtlsServer::processing(Incomming* incomming)
{
    //TODO
}

void DtlsServer::shutdown(std::error_code &ec)
{
    //TODO
}
