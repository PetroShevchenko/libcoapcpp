#ifndef PICO_ERROR_H
#define PICO_ERROR_H
#include <system_error>

enum class PicoStatus
{
    PICO_OK = 0,
};

namespace std
{
template<> struct is_error_condition_enum<PicoStatus> : public true_type {};
}

std::error_code make_error_code (PicoStatus e);
std::error_code make_system_error (int e);

#endif
