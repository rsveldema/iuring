#pragma once

/**
 * @file Logger.hpp
 * @brief Defines the Logger class for logging messages with different severity
 * levels.
 *
 * This class provides methods for logging debug, info, and error messages.
 * It supports logging to the console or to a file stream.
 */

#include <stdint.h>
#include <stdio.h>
#include <string>

#include "ILogger.hpp"

namespace logging
{

enum class LogOutput
{
    CONSOLE,
    FILE_STREAM
};


class Logger : public ILogger
{
public:
    void debug_msg(uint32_t line, const char* file, const std::string msg) override
    {
        log(line, file, Level::DEBUG, msg);
    }

    void info_msg(
        uint32_t line, const char* file, const std::string msg) override
    {
        log(line, file, Level::INFO, msg);
    }

    void error_msg(
        uint32_t line, const char* file, const std::string msg) override
    {
        log(line, file, Level::ERROR, msg);
    }

    Logger(bool debug, bool info, LogOutput output);
    ~Logger();

private:
    LogOutput m_output;
    FILE* m_f = nullptr;

    enum class Level
    {
        DEBUG,
        INFO,
        ERROR
    };

    void log(
        uint32_t line, const char* file, Level level, const std::string& msg);
};


} // namespace logging