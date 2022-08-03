#ifndef _UDP_SERVER_H
#define _UDP_SERVER_H

#include "unix_udp_server.h"
#include <cstddef>
#include <string>
#include "log.h"

namespace Unix
{

typedef void (*ReceivedPacketHandlerCallback)(const std::string &inBuffer, std::string &outBuffer, std::error_code &ec);

class UdpServer : public UdpServerConnection
{
public:
    UdpServer(int port, bool version4, std::error_code &ec):
    UdpServerConnection(port, version4, ec),
    m_running{false},
    m_callback{nullptr}
    {}

    ~UdpServer() = default;

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

private:
    void handle_request(std::error_code &ec);

private:
    bool m_running;
    ReceivedPacketHandlerCallback m_callback;
};

}// namespace Unix

#endif // _UDP_SERVER_H

