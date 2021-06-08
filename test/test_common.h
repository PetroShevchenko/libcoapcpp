#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#ifdef PRINT_TESTED_VALUES
#include "packet.h"
void print_options(const coap::Packet & packet);
void print_packet(const coap::Packet & packet);
void print_serialized_packet(const void *data, size_t size);
void print_option_from_list(std::vector<coap::Option *> options);
#endif

#endif
