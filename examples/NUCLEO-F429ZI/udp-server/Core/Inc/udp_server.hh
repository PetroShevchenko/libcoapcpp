#ifndef UDP_SERVER_HH
#define UDP_SERVER_HH
#include "lwip_socket.h"
#include "error.h"
#include <cstddef>
#include <string>

#define RECEIVE_BUFFER_SIZE 256U
#define TRANSMIT_BUFFER_SIZE 512U

typedef void (*ReceivedPacketHandlerCallback)(const std::string &inBuffer, std::string &outBuffer, std::error_code &ec);

class UdpServer
{
public:
	UdpServer(int port, std::error_code &ec, bool version4 = false);

	virtual ~UdpServer()
	{}

public:
    void stop()
    { m_running = false; }

    void start()
    { m_running = true; }

    bool is_started() const
    { return m_running;}

    void set_received_packed_handler_callback(ReceivedPacketHandlerCallback callback)
    { m_callback = callback; }

public:
	void init(std::error_code &ec);
    void process();
    void process2();

protected:
    int                         m_port;
    const bool                  m_version4;
    LwipSocketAddress           m_serverAddress;
    LwipSocket                  m_serverSocket;
    char 				        m_receiveBuffer[RECEIVE_BUFFER_SIZE];
    char 				        m_transmitBuffer[TRANSMIT_BUFFER_SIZE];
    bool                        m_running;

private:
    void handle_request(std::error_code &ec);

private:
    ReceivedPacketHandlerCallback m_callback;
};

#endif
