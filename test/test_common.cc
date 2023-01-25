#include "packet.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstdint>
#include <cstring>

using namespace std;
using namespace coap;
using namespace spdlog;

#ifdef PRINT_TESTED_VALUES
void print_options(const Packet & packet)
{
    info("options :");
    for (auto opt : const_cast<Packet &>(packet).options())
    {
        info("number : {0:d}", opt.number());
        info("value : ");
        fmt::print("{:02x}", fmt::join(opt.value(), ", "));
        fmt::print("\n");
    }
}

void print_option_from_list(vector<Option *> options)
{
    int index = 0;
    for (auto &opt : options)
    {
        info("option number: {0:d}", opt->number());
        info("option quantity : {0:d}", options.size());
        info("option sequens number : {0:d}", index++);
        info("option value length : {0:d}",opt->length());
        info("option value delta : {0:d}",opt->delta());
        info("option value: ");
        fmt::print("0x{:02x}", fmt::join(opt->value(), ", "));
        fmt::print("\n");
    }
}

void print_packet(const Packet & packet)
{
    info("Packet :");
    info("version : {0:d}", packet.version());
    info("token length : {0:d}", packet.token_length());
    if (packet.token_length())
    {
        info("token :");
        fmt::print("0x{:02x}", fmt::join(packet.token(), ", "));
        fmt::print("\n");
    }
    info("type : {0:d}", packet.type());
    info("code : {0:d}", packet.code_as_byte());
    info("code detail : {0:d}", packet.code_detail());
    info("code class : {0:d}", packet.code_class());
    info("message id : {0:d}", packet.identity());
    print_options(packet);
    size_t size = packet.payload().size();
    info("payload size: {0:d}", size);
    if (size)
    {
        info("payload : ");
        for (size_t i = 0; i < size; i++)
        {
            fmt::print("0x{:02x}", packet.payload()[i]);
            fmt::print(" ,");
            if (i != 0 && (i + 1) % 16 == 0)
                fmt::print("\n");
        }
        fmt::print("\n");
    }
}

void print_serialized_packet(const void *data, size_t size)
{
    const uint8_t *d = static_cast<const uint8_t *>(data);
    info("Serialized packet :");
    for (size_t i = 0; i < size; i++)
    {
        fmt::print("0x{:02x}", d[i]);
        fmt::print(" ,");
        if (i != 0 && (i + 1) % 16 == 0)
            fmt::print("\n");
    }
    fmt::print("\n");
}
#endif
