#include "pclcore/core/Log.hpp"
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace pclcore::log {

static LogCallback s_callback = nullptr;

void set_callback(LogCallback cb) {
    s_callback = cb;
}

void _emit(Level level, const char* fmt, ...) {
    if (s_callback) {
        char buf[4096];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        s_callback(level, buf);
    } else {
        // Default: fprintf to stderr with timestamp + level prefix
        const char* level_str = "?";
        switch (level) {
            case Level::Trace: level_str = "TRACE"; break;
            case Level::Debug: level_str = "DEBUG"; break;
            case Level::Info:  level_str = "INFO "; break;
            case Level::Warn:  level_str = "WARN "; break;
            case Level::Error: level_str = "ERROR"; break;
        }

        std::time_t now = std::time(nullptr);
        char time_buf[32];
        std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", std::localtime(&now));

        std::fprintf(stderr, "[%s] [%s] ", time_buf, level_str);

        va_list args;
        va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        va_end(args);

        std::fprintf(stderr, "\n");
    }
}

}  // namespace pclcore::log
