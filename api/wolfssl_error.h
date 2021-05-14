#ifndef _WOLFSSL_ERROR_H
#define _WOLFSSL_ERROR_H
#include <system_error>
#include "wolfssl/wolfcrypt/error-crypt.h"

typedef int WolfsslStatus;

namespace std
{
template<> struct is_error_condition_enum<WolfsslStatus> : public true_type {};
}

std::error_code make_error_code (WolfsslStatus e);


#endif
