#ifndef _MBEDTLS_ERROR_H
#define _MBEDTLS_ERROR_H

#include <system_error>
#include "mbedtls/error.h"

typedef int MbedtlsStatus;

namespace std
{
template<> struct is_error_condition_enum<MbedtlsStatus> : public true_type {};
}

std::error_code make_error_code (MbedtlsStatus e);

#endif
