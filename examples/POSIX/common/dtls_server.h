#ifndef _DTLS_SERVER_H
#define _DTLS_SERVER_H
#include "udp_server.h"
#include "error.h"
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

struct SecureIncomming : public Incomming
{
    SecureIncomming(
            const SocketAddress *clientAddress,
            const SocketAddress *serverAddress,
            Socket *serverSocket,
            time_t lifetime,
            std::error_code &ec
        );

    ~SecureIncomming();

    WOLFSSL         *m_ssl;
    int             m_pipeFd[2];
};

class DtlsServer : public UdpServer
{
public:
    DtlsServer(int port, std::error_code &ec, bool version4 = false);
    ~DtlsServer() = default;

public:
    //Incomming* find_connection(const SocketAddress * address) override;
    bool remove_connection(Incomming * connection) override;
    Incomming* new_incomming(const SocketAddress * address, std::error_code &ec) override;


    void accept(
            const SocketAddress * address,
            const void * buffer,
            ssize_t received,
            std::error_code &ec
        ) override;

    void connect(
            Socket * sock,
            void * buffer,
            size_t buferSize,
            ssize_t &received,
            SocketAddress *clientAddress,
            std::error_code &ec
        ) override;

    void listening(std::error_code &ec) override;

    void processing(Incomming* incomming) override;

    void shutdown(std::error_code &ec) override;

    //void processing_thread(Incomming* incomming, void *context);

private:
    std::vector<SecureIncomming*>     m_connections;
    WOLFSSL_CTX                       *m_ctx;
};



#endif
