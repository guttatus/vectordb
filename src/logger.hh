#pragma once

#include <memory>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

namespace vdb
{
extern std::shared_ptr<spdlog::logger> GlobalLogger;
void init_global_logger();
void set_log_level(spdlog::level::level_enum log_level);
} // namespace vdb
