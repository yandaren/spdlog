
#pragma once

//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

// Thread safe logger (except for set_pattern(..), set_formatter(..) and set_error_handler())
// Has name, log level, vector of std::shared sink pointers and formatter
// Upon each log write the logger:
// 1. Checks if its log level is enough to log the message
// 2. Format the message using the formatter function
// 3. Pass the formatted message to its sinks to performa the actual logging

#include "common.h"
#include "sinks/base_sink.h"
#include "sinks/stdout_sinks.h"

#include <memory>
#include <string>
#include <vector>

namespace spdlog {
class logger
{
public:
    // ctor with sinks as init list
    logger(const std::string &logger_name, sinks_init_list sinks_list)
        : logger(logger_name, sinks_list.begin(), sinks_list.end())
    {
    }

    // ctor with single sink
    logger(const std::string &logger_name, spdlog::sink_ptr single_sink)
        : logger(logger_name, {std::move(single_sink)})
    {
    }

    // create logger with given name, sinks and the default pattern formatter.
    // all other ctors will call this one.
    template<class It>
    logger(std::string logger_name, const It &begin, const It &end)
        : _name(std::move(logger_name))
        , _sinks(begin, end)
        , _formatter(std::make_shared<pattern_formatter>("%+"))
        , _level(level::info)
        , _flush_level(level::off)
        , _last_err_time(0)
        , _msg_counter(1)
    {
        _err_handler = [this](const std::string &msg) { this->_default_err_handler(msg); };
    }

    virtual ~logger() = default;

    void set_formatter(spdlog::formatter_ptr msg_formatter)
    {
        _set_formatter(std::move(msg_formatter));
    }

    void set_pattern(const std::string &pattern, pattern_time_type pattern_time)
    {
        _set_pattern(pattern, pattern_time);
    }

    template<typename... Args>
    void log(level::level_enum lvl, const char *fmt, const Args &... args)
    {
        if (!should_log(lvl))
        {
            return;
        }

        try
        {
            details::log_msg log_msg(&_name, lvl);

#if defined(SPDLOG_FMT_PRINTF)
            fmt::printf(log_msg.raw, fmt, args...);
#else
            log_msg.raw.write(fmt, args...);
#endif
            _sink_it(log_msg);
        }
        catch (const std::exception &ex)
        {
            _err_handler(ex.what());
        }
        catch (...)
        {
            _err_handler("Unknown exception in logger " + _name);
            throw;
        }
    }

    template<typename... Args>
    void log(level::level_enum lvl, const char *msg)
    {
        if (!should_log(lvl))
        {
            return;
        }
        try
        {
            details::log_msg log_msg(&_name, lvl);
            log_msg.raw << msg;
            _sink_it(log_msg);
        }
        catch (const std::exception &ex)
        {
            _err_handler(ex.what());
        }
        catch (...)
        {
            _err_handler("Unknown exception in logger " + _name);
            throw;
        }
    }

    template<typename T>
    void log(level::level_enum lvl, const T &msg)
    {
        if (!should_log(lvl))
        {
            return;
        }
        try
        {
            details::log_msg log_msg(&_name, lvl);
            log_msg.raw << msg;
            _sink_it(log_msg);
        }
        catch (const std::exception &ex)
        {
            _err_handler(ex.what());
        }
        catch (...)
        {
            _err_handler("Unknown exception in logger " + _name);
            throw;
        }
    }

    template<typename Arg1, typename... Args>
    void trace(const char *fmt, const Arg1 &arg1, const Args &... args)
    {
        log(level::trace, fmt, arg1, args...);
    }

    template<typename Arg1, typename... Args>
    void debug(const char *fmt, const Arg1 &arg1, const Args &... args)
    {
        log(level::debug, fmt, arg1, args...);
    }

    template<typename Arg1, typename... Args>
    void info(const char *fmt, const Arg1 &arg1, const Args &... args)
    {
        log(level::info, fmt, arg1, args...);
    }

    template<typename Arg1, typename... Args>
    void warn(const char *fmt, const Arg1 &arg1, const Args &... args)
    {
        log(level::warn, fmt, arg1, args...);
    }

    template<typename Arg1, typename... Args>
    void error(const char *fmt, const Arg1 &arg1, const Args &... args)
    {
        log(level::err, fmt, arg1, args...);
    }

    template<typename Arg1, typename... Args>
    void critical(const char *fmt, const Arg1 &arg1, const Args &... args)
    {
        log(level::critical, fmt, arg1, args...);
    }

    template<typename T>
    void trace(const T &msg)
    {
        log(level::trace, msg);
    }

    template<typename T>
    void debug(const T &msg)
    {
        log(level::debug, msg);
    }

    template<typename T>
    void info(const T &msg)
    {
        log(level::info, msg);
    }

    template<typename T>
    void warn(const T &msg)
    {
        log(level::warn, msg);
    }

    template<typename T>
    void error(const T &msg)
    {
        log(level::err, msg);
    }

    template<typename T>
    void critical(const T &msg)
    {
        log(level::critical, msg);
    }

#ifdef SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <codecvt>
#include <locale>

    template<typename... Args>
    void log(level::level_enum lvl, const wchar_t *msg)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;

        log(lvl, conv.to_bytes(msg));
    }

    template<typename... Args>
    void log(level::level_enum lvl, const wchar_t *fmt, const Args &... args)
    {
        fmt::WMemoryWriter wWriter;

        wWriter.write(fmt, args...);
        log(lvl, wWriter.c_str());
    }

    template<typename... Args>
    void trace(const wchar_t *fmt, const Args &... args)
    {
        log(level::trace, fmt, args...);
    }

    template<typename... Args>
    void debug(const wchar_t *fmt, const Args &... args)
    {
        log(level::debug, fmt, args...);
    }

    template<typename... Args>
    void info(const wchar_t *fmt, const Args &... args)
    {
        log(level::info, fmt, args...);
    }

    template<typename... Args>
    void warn(const wchar_t *fmt, const Args &... args)
    {
        log(level::warn, fmt, args...);
    }

    template<typename... Args>
    void error(const wchar_t *fmt, const Args &... args)
    {
        log(level::err, fmt, args...);
    }

    template<typename... Args>
    void critical(const wchar_t *fmt, const Args &... args)
    {
        log(level::critical, fmt, args...);
    }

#endif // SPDLOG_WCHAR_TO_UTF8_SUPPORT

    //
    // name and level
    //
    const std::string &name() const
    {
        return _name;
    }

    void set_level(spdlog::level::level_enum log_level)
    {
        _level.store(log_level);
    }

    virtual void set_error_handler(spdlog::log_err_handler err_handler)
    {
        _err_handler = std::move(err_handler);
    }

    virtual spdlog::log_err_handler error_handler()
    {
        return _err_handler;
    }

    virtual void flush()
    {
        for (auto &sink : _sinks)
        {
            sink->flush();
        }
    }

    void flush_on(level::level_enum log_level)
    {
        _flush_level.store(log_level);
    }

    spdlog::level::level_enum level() const
    {
        return static_cast<spdlog::level::level_enum>(_level.load(std::memory_order_relaxed));
    }

    bool should_log(spdlog::level::level_enum msg_level) const
    {
        return msg_level >= _level.load(std::memory_order_relaxed);
    }
    const std::vector<spdlog::sink_ptr> &sinks() const
    {
        return _sinks;
    }

protected:
    //
    // protected virtual called at end of each user log call (if enabled) by the line_logger
    //
    virtual void _sink_it(spdlog::details::log_msg &msg)
    {
#if defined(SPDLOG_ENABLE_MESSAGE_COUNTER)
        _incr_msg_counter(msg);
#endif
        _formatter->format(msg);
        for (auto &sink : _sinks)
        {
            if (sink->should_log(msg.level))
            {
                sink->log(msg);
            }
        }

        if (_should_flush_on(msg))
        {
            flush();
        }
    }

    virtual void _set_pattern(const std::string &pattern, pattern_time_type pattern_time)
    {
        _formatter = std::make_shared<pattern_formatter>(pattern, pattern_time);
    }

    virtual void _set_formatter(formatter_ptr msg_formatter)
    {
        _formatter = std::move(msg_formatter);
    }

    virtual void _default_err_handler(const std::string &msg)
    {
        auto now = time(nullptr);
        if (now - _last_err_time < 60)
        {
            return;
        }
        auto tm_time = details::os::localtime(now);
        char date_buf[100];
        std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &tm_time);
        details::log_msg err_msg;
        err_msg.formatted.write("[*** LOG ERROR ***] [{}] [{}] [{}]{}", name(), msg, date_buf, details::os::default_eol);
        spdlog::sinks::stderr_sink_mt::instance()->log(err_msg);
        _last_err_time = now;
    }

     bool _should_flush_on(const details::log_msg &msg)
    {
        const auto flush_level = _flush_level.load(std::memory_order_relaxed);
        return (msg.level >= flush_level) && (msg.level != level::off);
    }

    void _incr_msg_counter(details::log_msg &msg)
    {
        msg.msg_id = _msg_counter.fetch_add(1, std::memory_order_relaxed);
    }

   
    const std::string _name;
    std::vector<sink_ptr> _sinks;
    formatter_ptr _formatter;
    spdlog::level_t _level;
    spdlog::level_t _flush_level;
    log_err_handler _err_handler;
    std::atomic<time_t> _last_err_time;
    std::atomic<size_t> _msg_counter;
};
} // namespace spdlog
