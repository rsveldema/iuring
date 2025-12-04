#pragma once

/**
 * @file ILogger.hpp
 * @brief Defines the ILogger interface for logging messages with different
 * severity levels.
 *
 * This interface provides methods for logging debug, info, and error messages.
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#include <format>

namespace logging
{
class ILogger
{
public:
    ILogger(bool debug, bool info)
        : m_debug(debug)
        , m_info(info)
    {
    }

    virtual ~ILogger() {}

    virtual void debug_msg(
        uint32_t line, const char* file, const std::string msg) = 0;
    virtual void info_msg(
        uint32_t line, const char* file, const std::string msg) = 0;
    virtual void error_msg(
        uint32_t line, const char* file, const std::string msg) = 0;

    bool enable_debug() const
    {
        return m_debug;
    }
    bool enable_info() const
    {
        return m_info;
    }

protected:
    bool m_debug = true;
    bool m_info = true;
};

const char* strip_prefix(const char* path);


#define LOG_DEBUG(logger, ...)                                                 \
    if (logger.enable_debug())                                                 \
    {                                                                          \
        logger.debug_msg(__LINE__, logging::strip_prefix(__FILE__),            \
            std::format(__VA_ARGS__));                                         \
    }

#define LOG_INFO(logger, ...)                                                  \
    if (logger.enable_info())                                                  \
    {                                                                          \
        logger.info_msg(__LINE__, logging::strip_prefix(__FILE__),             \
            std::format(__VA_ARGS__));                                         \
    }

#define LOG_ERROR(logger, ...)                                                 \
    logger.error_msg(                                                          \
        __LINE__, logging::strip_prefix(__FILE__), std::format(__VA_ARGS__))

} // namespace logging