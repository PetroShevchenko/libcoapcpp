#include "wolfssl_error.h"
#include <string>

namespace
{

struct WolfsslErrorCategory : public std::error_category
{
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* WolfsslErrorCategory::name() const noexcept
{ return "wolfssl"; }

std::string WolfsslErrorCategory::message(int ev) const
{
    return wc_GetErrorString(ev);
}

const WolfsslErrorCategory theWolfsslErrorCategory {};

} // namespace

std::error_code make_error_code (WolfsslStatus e)
{
    return {static_cast<int>(e), theWolfsslErrorCategory};
}

