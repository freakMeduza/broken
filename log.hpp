#pragma once

#define SPDLOG_NO_NAME
#include <spdlog/spdlog.h>

#define broken_trace(fmt, ...)                                                                                         \
    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::trace, fmt, ##__VA_ARGS__)

#define broken_debug(fmt, ...)                                                                                         \
    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::debug, fmt, ##__VA_ARGS__)

#define broken_info(fmt, ...) SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::info, fmt, ##__VA_ARGS__)

#define broken_warn(fmt, ...) SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::warn, fmt, ##__VA_ARGS__)

#define broken_error(fmt, ...) SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::err, fmt, ##__VA_ARGS__)

#define broken_crit(fmt, ...)                                                                                          \
    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::critical, fmt, ##__VA_ARGS__)
