#include "error.h"
#include <string>

namespace
{

struct PicoErrorCategory : public std::error_category
{
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* PicoErrorCategory::name() const noexcept
{ return "pico"; }

std::string PicoErrorCategory::message(int ev) const
{
    switch((PicoStatus)ev)
    {
        case PicoStatus::PICO_OK:
            return "Success"; 
    }
    return "Unknown error";
}

const PicoErrorCategory thePicoErrorCategory {};

} // namespace

std::error_code make_error_code (PicoStatus e)
{
    return {static_cast<int>(e), thePicoErrorCategory};
}

std::error_code make_system_error (int e)
{
    return {e, std::generic_category()};
}
