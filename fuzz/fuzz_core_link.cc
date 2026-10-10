#include "core_link.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace std;
using namespace coap;

/*
    The input is a text in CoRE Link Format, for example a payload of /.well-known/core.

    Checked properties:
    1. parse_core_link() never reads outside of the text, whatever the text is.
    2. Parsed records can be written back with create_core_link().
    3. The written text is parsed again without an error and gives the same records:
       the same paths and the same names, types and values of the parameters.
    4. create_record_from_path_if_contains() never reads outside of its arguments,
       when it is called with the path of the first record for every parsed record.
*/

static void fail(const char * what, const error_code &ec = error_code())
{
    if (ec)
        fprintf(stderr, "fuzz_core_link: %s: %s\n", what, ec.message().c_str());
    else
        fprintf(stderr, "fuzz_core_link: %s\n", what);
    abort();
}

// The API takes a C string, the input has no terminating zero
static unique_ptr<char[]> to_c_string(const uint8_t * data, size_t size)
{
    unique_ptr<char[]> text(new char[size + 1]);
    memcpy(text.get(), data, size);
    text[size] = '\0';
    return text;
}

static bool is_same_parameter(const CoreLinkParameter &first, const CoreLinkParameter &second)
{
    if (first.name != second.name || first.value.type != second.value.type)
        return false;

    if (first.value.type == CoreLinkParameter::NUMBER)
        return first.value.asNumber == second.value.asNumber;

    return first.value.asString == second.value.asString;
}

static void compare_records(const vector<CoreLinkType> &first, const vector<CoreLinkType> &second)
{
    if (first.size() != second.size())
        fail("number of records differs after writing and parsing");

    for (size_t i = 0; i < first.size(); i++)
    {
        if (first[i].uri.path() != second[i].uri.path())
            fail("path of a record differs after writing and parsing");
        if (first[i].parameters.size() != second[i].parameters.size())
            fail("number of parameters differs after writing and parsing");

        for (size_t j = 0; j < first[i].parameters.size(); j++)
        {
            if (!is_same_parameter(first[i].parameters[j], second[i].parameters[j]))
                fail("parameter differs after writing and parsing");
        }
    }
}

static void check_records_from_path(const vector<CoreLinkType> &records)
{
    const string &path = records[0].uri.path();

    for (const CoreLinkType &record : records)
    {
        error_code ec;
        CoreLinkType out;

        core_link::create_record_from_path_if_contains(path.c_str(), record, out, ec);
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    const unique_ptr<char[]> text = to_c_string(data, size);
    error_code ec;
    CoreLink parsed;

    parsed.parse_core_link(text.get(), ec);
    if (ec || parsed.payload().empty())
        return 0;

    parsed.create_core_link(ec);
    if (ec)
        fail("parsed records cannot be written back", ec);

    CoreLink again;

    again.parse_core_link(parsed.core_link().c_str(), ec);
    if (ec)
        fail("written text cannot be parsed", ec);

    compare_records(parsed.payload(), again.payload());
    check_records_from_path(parsed.payload());

    return 0;
}
