#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H
#include "packet.h"

struct TestCase {
    const char * name;
    std::vector<uint8_t> message;
    CoapStatus expected;
};

#ifdef PRINT_TESTED_VALUES
void print_options(const coap::Packet & packet);
void print_packet(const coap::Packet & packet);
void print_serialized_packet(const void *data, size_t size);
void print_option_from_list(std::vector<coap::Option *> options);
void print_error(const char* tcName, const std::error_code &ec);
void print_error(const std::error_code &ec);
#endif

#endif
