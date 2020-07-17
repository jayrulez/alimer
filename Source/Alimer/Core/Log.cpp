//
// Copyright (c) 2020 Amer Koleci and contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Core/Log.h"

#if defined(__APPLE__)
#  include <TargetConditionals.h>
#endif

#if defined(__ANDROID__)
#  include <android/log.h>
#elif TARGET_OS_IOS || TARGET_OS_TV
#  include <sys/syslog.h>
#elif TARGET_OS_MAC || defined(__linux__)
#  include <unistd.h>
#elif defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <Windows.h>
#   include <array>
#elif defined(__EMSCRIPTEN__)
#  include <emscripten.h>
#endif

namespace alimer
{
    static const char* LogLevelPefixes[uint32(LogLevel::Count)] = {
        "VERBOSE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "CRITICAL",
        "OFF"
    };

#if defined(_DEBUG) && defined(_WIN32)
    WORD SetConsoleForegroundColor(HANDLE consoleOutput, WORD attribs)
    {
        CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
        ::GetConsoleScreenBufferInfo(consoleOutput, &orig_buffer_info);
        WORD back_color = orig_buffer_info.wAttributes;
        // retrieve the current background color
        back_color &= static_cast<WORD>(~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));
        // keep the background color unchanged
        ::SetConsoleTextAttribute(consoleOutput, attribs | back_color);
        return orig_buffer_info.wAttributes; // return orig attribs
    }
#endif

    class Logger
    {
    public:
        Logger(const std::string& name);
        ~Logger() = default;

        static Logger* GetDefault()
        {
            static Logger defaultLogger("Alimer");
            return &defaultLogger;
        }

        void Log(LogLevel level, const std::string& message);

    private:
        bool ShouldLog(LogLevel msgLevel) const
        {
            return msgLevel >= level;
        }

        std::string name;
        LogLevel level;

#if defined(_DEBUG) && defined(_WIN32)
        std::array<WORD, size_t(LogLevel::Count)> consoleColors;
#endif
    };

    Logger::Logger(const std::string& name)
        : name{ name }
#ifdef _DEBUG
        , level(LogLevel::Debug)
#else
        , level(LogLevel::Info)
#endif
    {
#if defined(_DEBUG) && defined(_WIN32)
        const WORD BOLD = FOREGROUND_INTENSITY;
        const WORD RED = FOREGROUND_RED;
        const WORD GREEN = FOREGROUND_GREEN;
        const WORD CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE;
        const WORD WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        const WORD YELLOW = FOREGROUND_RED | FOREGROUND_GREEN;

        consoleColors[(uint32_t)LogLevel::Verbose] = WHITE;
        consoleColors[(uint32_t)LogLevel::Debug] = CYAN;
        consoleColors[(uint32_t)LogLevel::Info] = GREEN;
        consoleColors[(uint32_t)LogLevel::Warn] = YELLOW | BOLD;
        consoleColors[(uint32_t)LogLevel::Error] = RED | BOLD;                         // red bold
        consoleColors[(uint32_t)LogLevel::Critical] = BACKGROUND_RED | WHITE | BOLD; // white bold on red background
        consoleColors[(uint32_t)LogLevel::Off] = 0;
#endif
    }

    void Logger::Log(LogLevel level, const std::string& message)
    {
        bool log_enabled = ShouldLog(level);
        if (!log_enabled)
            return;

#if defined(__ANDROID__)
        int priority = 0;
        switch (level)
        {
        case LogLevel::Verbose: priority = ANDROID_LOG_VERBOSE; break;
        case LogLevel::Debug: priority = ANDROID_LOG_DEBUG; break;
        case LogLevel::Info: priority = ANDROID_LOG_INFO; break;
        case LogLevel::Warn: priority = ANDROID_LOG_WARN; break;
        case LogLevel::Error: priority = ANDROID_LOG_ERROR; break;
        case LogLevel::Critical: priority = ANDROID_LOG_FATAL; break;
        default: return;
        }
        __android_log_print(priority, name.c_str(), "%s", message.c_str());
#elif TARGET_OS_IOS || TARGET_OS_TV
        int priority = 0;
        switch (level)
        {
        case LogLevel::Verbose: priority = LOG_DEBUG; break;
        case LogLevel::Debug: priority = LOG_DEBUG; break;
        case LogLevel::Info: priority = LOG_INFO; break;
        case LogLevel::Warn: priority = LOG_WARNING; break;
        case LogLevel::Error: priority = LOG_ERR; break;
        case LogLevel::Critical: priority = LOG_CRIT; break;
        default: return;
        }
        syslog(priority, "%s", message.c_str());
#elif TARGET_OS_MAC || defined(__linux__)
        int fd = 0;
        switch (level)
        {
        case LogLevel::Warn:
        case LogLevel::Error:
        case LogLevel::Critical:
            fd = STDERR_FILENO;
            break;
        case LogLevel::Verbose:
        case LogLevel::Debug:
        case LogLevel::Info:
            fd = STDOUT_FILENO;
            break;
        default: return;
        }

        std::vector<char> output(str.begin(), str.end());
        output.push_back('\n');

        size_t offset = 0;
        while (offset < output.size())
        {
            ssize_t written = write(fd, output.data() + offset, output.size() - offset);
            while (written == -1 && errno == EINTR)
                written = write(fd, output.data() + offset, output.size() - offset);

            if (written == -1)
                return;

            offset += static_cast<std::size_t>(written);
        }
#elif defined(_WIN32)
        std::string fmt_str = fmt::format("[{}] {}\r\n", LogLevelPefixes[(uint32_t)level], message);
        OutputDebugStringA(fmt_str.c_str());

#  ifdef _DEBUG
        HANDLE consoleOutput;
        switch (level)
        {
        case LogLevel::Warn:
        case LogLevel::Error:
        case LogLevel::Critical:
            consoleOutput = GetStdHandle(STD_ERROR_HANDLE);
            break;
        case LogLevel::Verbose:
        case LogLevel::Debug:
        case LogLevel::Info:
            consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        default: return;
        }

        WriteConsoleA(consoleOutput, "[", 1, nullptr, nullptr);
        auto orig_attribs = SetConsoleForegroundColor(consoleOutput, consoleColors[(uint32_t)level]);
        WriteConsoleA(consoleOutput, LogLevelPefixes[(uint32_t)level], static_cast<DWORD>(strlen(LogLevelPefixes[(uint32_t)level])), nullptr, nullptr);
        // reset to orig colors
        ::SetConsoleTextAttribute(consoleOutput, orig_attribs);

        fmt_str = fmt::format("] {}\n", message);
        WriteConsoleA(consoleOutput, fmt_str.c_str(), static_cast<DWORD>(fmt_str.length()), nullptr, nullptr);
#  endif
#elif defined(__EMSCRIPTEN__)
        int flags = EM_LOG_CONSOLE;
        switch (level)
        {
        case Log::Level::error:
            flags |= EM_LOG_ERROR;
            break;
        case Log::Level::warning:
            flags |= EM_LOG_WARN;
            break;
        case Log::Level::info:
        case Log::Level::all:
            break;
        default: return;
    }
        emscripten_log(flags, "%s", str.c_str());
#endif
}

    void Log(LogLevel level, const std::string& message)
    {
        Logger::GetDefault()->Log(level, message);
    }

    void Verbose(const std::string& message)
    {
        Logger::GetDefault()->Log(LogLevel::Verbose, message);
    }

    void Debug(const std::string& message)
    {
        Logger::GetDefault()->Log(LogLevel::Debug, message);
    }

    void Info(const std::string& message)
    {
        Logger::GetDefault()->Log(LogLevel::Info, message);
    }

    void Warn(const std::string& message)
    {
        Logger::GetDefault()->Log(LogLevel::Warn, message);
    }

    void Error(const std::string& message)
    {
        Logger::GetDefault()->Log(LogLevel::Error, message);
    }

    void Critical(const std::string& message)
    {
        Logger::GetDefault()->Log(LogLevel::Critical, message);
    }
}
