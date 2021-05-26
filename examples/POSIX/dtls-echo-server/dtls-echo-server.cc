#include "common/dtls_server.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

using namespace std;
using namespace spdlog;

int main(int argc, char **argv)
{
    set_level(level::debug);
    debug("dtls-echo-server");

    return 0;
}
