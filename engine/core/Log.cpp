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

#include "core/Log.h"
#include <vector>

#if defined(__APPLE__)
#   include <TargetConditionals.h>
#endif

#if defined(__ANDROID__)
#   include <android/log.h>
#elif TARGET_OS_IOS || TARGET_OS_TV
#   include <sys/syslog.h>
#elif TARGET_OS_MAC || defined(__linux__)
#   include <unistd.h>
#elif ALIMER_PLATFORM_WINDOWS
#   include <foundation/windows.h>
#   include <strsafe.h>
#elif defined(__EMSCRIPTEN__)
#   include <emscripten.h>
#endif

using namespace std;

namespace alimer
{
    static std::vector<Logger*> _loggers;

    Logger::Logger(const string& name)
        : _name(name)
#ifdef _DEBUG
        , _level{ LogLevel::Debug }
#else
        , _level{ LogLevel::Info }
#endif
    {
        _loggers.push_back(this);
    }

    Logger::~Logger()
    {

    }

    void Logger::SetEnabled(bool value)
    {
        _enabled = value;
    }

    bool Logger::IsLevelEnabled(LogLevel level) const
    {
        return _enabled && level != LogLevel::Off && level >= _level;
    }

    void Logger::Log(LogLevel level, const char* message)
    {
        if (IsLevelEnabled(level))
        {
#if defined(__ANDROID__)
            int priority = 0;
            switch (level)
            {
            case LogLevel::Trace: priority = ANDROID_LOG_VERBOSE; break;
            case LogLevel::Debug: priority = ANDROID_LOG_DEBUG; break;
            case LogLevel::Info: priority = ANDROID_LOG_INFO; break;
            case LogLevel::Warning: priority = ANDROID_LOG_WARN; break;
            case LogLevel::Error: priority = ANDROID_LOG_ERROR; break;
            default: return;
            }
            __android_log_print(priority, _name.c_str(), "%s", message);
#elif TARGET_OS_IOS || TARGET_OS_TV
            int priority = 0;
            switch (level)
            {
            case LogLevel::Trace: priority = LOG_DEBUG; break;
            case LogLevel::Debug: priority = LOG_DEBUG; break;
            case LogLevel::Info: priority = LOG_INFO; break;
            case LogLevel::Warning: priority = LOG_WARNING; break;
            case LogLevel::Error: priority = LOG_ERR; break;
            default: return;
            }
            syslog(priority, "%s", message);
#elif TARGET_OS_MAC || defined(__linux__)
            int fd = 0;
            switch (level)
            {
            case LogLevel::Trace:
            case LogLevel::Debug:
            case LogLevel::Info:
                fd = STDERR_FILENO;
                break;
            case LogLevel::Warning:
            case LogLevel::Error:
                fd = STDOUT_FILENO;
                break;
            default: return;
            }

            vector<char> output(str.begin(), str.end());
            output.push_back('\n');

            size_t offset = 0;
            while (offset < output.size())
            {
                const ssize_t written = write(fd, output.data() + offset, output.size() - offset);
                if (written == -1)
                    return;

                offset += static_cast<size_t>(written);
            }
#elif defined(_WIN32)
            const int bufferSize = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
            if (bufferSize == 0)
                return;

            std::vector<WCHAR> buffer(bufferSize + 1); // +1 for the newline
            if (MultiByteToWideChar(CP_UTF8, 0, message, -1, buffer.data(), static_cast<int>(buffer.size())) == 0)
                return;

            if (FAILED(StringCchCatW(buffer.data(), buffer.size(), L"\n")))
                return;

            OutputDebugStringW(buffer.data());
#   ifdef _DEBUG
            HANDLE handle;
            switch (level)
            {
            case LogLevel::Warning:
            case LogLevel::Error:
                handle = GetStdHandle(STD_ERROR_HANDLE);
                break;
            default:
                handle = GetStdHandle(STD_OUTPUT_HANDLE);
                break;
            }

            DWORD bytesWritten;
            WriteConsoleW(handle, buffer.data(), static_cast<DWORD>(wcslen(buffer.data())), &bytesWritten, nullptr);
#   endif
#elif defined(__EMSCRIPTEN__)
            int flags = EM_LOG_NO_PATHS;
            int flags = EM_LOG_CONSOLE;
            switch (level)
            {
            case LogLevel::Trace:
            case LogLevel::Debug:
            case LogLevel::Info:
                flags |= EM_LOG_CONSOLE;
                break;

            case LogLevel::Warning:
                flags |= EM_LOG_CONSOLE | EM_LOG_WARN;
                break;

            case LogLevel::Error:
                flags |= EM_LOG_CONSOLE | EM_LOG_ERROR;
                break;

            case Log::Level::Info:
            case Log::Level::All:
                break;
            default: return;
            }
            emscripten_log(flags, "%s", message);
#endif
        }
    }

    void Logger::Log(LogLevel level, const string& message)
    {
        Log(level, message.c_str());
    }

    void Logger::LogFormat(LogLevel level, const char* format, ...)
    {
        if (IsLevelEnabled(level))
        {
            va_list args;
            va_start(args, format);
            char message[kMaxLogMessage];
            vsnprintf(message, kMaxLogMessage, format, args);
            size_t len = strlen(message);
            if ((len > 0) && (message[len - 1] == '\n')) {
                message[--len] = '\0';
                if ((len > 0) && (message[len - 1] == '\r')) {  /* catch "\r\n", too. */
                    message[--len] = '\0';
                }
            }
            Log(level, message);
            va_end(args);
        }
    }

    Logger* Log::GetDefault()
    {
        static Logger defaultLogger("alimer");
        return &defaultLogger;
    }
}
