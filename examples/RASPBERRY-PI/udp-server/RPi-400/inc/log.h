#ifndef LOG_H
#define LOG_H

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#define DEBUG_LOG_INIT() spdlog::set_level(level::debug)
#define DEBUG_LOG_ENTER() spdlog::debug("=== Entering to {} ===",__func__)
#define DEBUG_LOG_EXIT() spdlog::debug("=== Exiting from {} ===",__func__)

#endif // LOG_H
