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

#pragma once

#include "Core/Assert.h"
#include "Core/String.h"
#include <fmt/format.h>

namespace alimer
{
    enum class LogLevel : uint32_t
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Off
    };

    class ALIMER_API Log final
    {
    public:
        static bool IsEnabled() { return _enabled; }
        static void SetEnabled(bool value);
        static bool IsLevelEnabled(LogLevel level);

        static void Write(LogLevel level, const char* str);
        static void Write(LogLevel level, const String& str);

    private:
        Log() = delete;
        ~Log() = delete;

        static bool _enabled;
        static LogLevel _level;
    };
} 

#define LOG_TRACE(...) alimer::Log::Write(alimer::LogLevel::Trace, fmt::format(__VA_ARGS__))
#define LOG_DEBUG(...) alimer::Log::Write(alimer::LogLevel::Debug, fmt::format(__VA_ARGS__))
#define LOG_INFO(...) alimer::Log::Write(alimer::LogLevel::Info, fmt::format(__VA_ARGS__))
#define LOG_WARN(...) alimer::Log::Write(alimer::LogLevel::Warning, fmt::format(__VA_ARGS__))

#define LOG_ERROR(...) do \
    { \
        alimer::Log::Write(alimer::LogLevel::Error, fmt::format("{}:{}] {}", __FILE__, __LINE__,  fmt::format(__VA_ARGS__))); \
        ALIMER_FORCE_CRASH(); \
    } while (0)
