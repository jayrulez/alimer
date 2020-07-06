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
#   include <TargetConditionals.h>
#endif

#if defined(__ANDROID__)
#   include <android/log.h>
#elif TARGET_OS_IOS || TARGET_OS_TV
#   include <sys/syslog.h>
#elif TARGET_OS_MAC || defined(__linux__)
#   include <unistd.h>
#elif ALIMER_PLATFORM_WINDOWS
#   include <strsafe.h>
#elif defined(__EMSCRIPTEN__)
#   include <emscripten.h>
#endif

#include <memory>

namespace alimer
{
    bool Log::_enabled = true;
#ifdef _DEBUG
    LogLevel Log::_level = LogLevel::Debug;
#else
    LogLevel Log::_level = LogLevel::Info;
#endif

    void Log::SetEnabled(bool value)
    {
        _enabled = value;
    }

    bool Log::IsLevelEnabled(LogLevel level)
    {
        return _enabled && level != LogLevel::Off && level >= _level;
    }

    void Log::Write(LogLevel level, const char* str)
    {
        if (!IsLevelEnabled(level))
            return;

        Write(level, String(str));
    }

    void Log::Write(LogLevel level, const String& str)
    {
        if (!IsLevelEnabled(level))
            return;

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
        __android_log_print(priority, "Alimer", "%s", message);
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
        syslog(priority, "%s", str.c_str());
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
        const int bufferSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (bufferSize == 0)
            return;

        auto buffer = std::make_unique<WCHAR[]>(bufferSize + 1); // +1 for the newline
        if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.get(), bufferSize) == 0)
            return;

        if (FAILED(StringCchCatW(buffer.get(), static_cast<size_t>(bufferSize+1), L"\n")))
            return;

        OutputDebugStringW(buffer.get());
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
        WriteConsoleW(handle, buffer.get(), static_cast<DWORD>(wcslen(buffer.get())), &bytesWritten, nullptr);
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
        emscripten_log(flags, "%s", str.c_str());
#endif
    }
}
