#include "mbedtls_error.h"
#include <string>

namespace
{

struct MbedtlsErrorCategory : public std::error_category
{
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* MbedtlsErrorCategory::name() const noexcept
{ return "mbedtls"; }

std::string MbedtlsErrorCategory::message(int ev) const
{
    return mbedtls_high_level_strerr(ev);
}

const MbedtlsErrorCategory theMbedtlsErrorCategory {};

} // namespace

std::error_code make_error_code (MbedtlsStatus e)
{
    return {static_cast<int>(e), theMbedtlsErrorCategory};
}