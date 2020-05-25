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
#include <string>

namespace Alimer
{
    static constexpr uint32_t kMaxLogMessage = 4096;

    enum class LogLevel : uint32_t
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Off
    };

    class ALIMER_API Logger final
    {
    public:
        Logger(const std::string& name);
        ~Logger();

        bool IsEnabled() const { return _enabled; }
        void SetEnabled(bool value);
        bool IsLevelEnabled(LogLevel level) const;

        void Log(LogLevel level, const char* message);
        void Log(LogLevel level, const std::string& message);
        void LogFormat(LogLevel level, const char* format, ...);

    private:
        std::string _name;
        bool _enabled = true;
        LogLevel _level;
    };

    class ALIMER_API Log final
    {
    public:
        static Logger* GetDefault();
    };
}

// Current function macro.
#ifdef WIN32
#define __current__func__ __FUNCTION__
#else
#define __current__func__ __func__
#endif

#define LOG_TRACE(...) Alimer::Log::GetDefault()->LogFormat(Alimer::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) Alimer::Log::GetDefault()->LogFormat(Alimer::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) Alimer::Log::GetDefault()->LogFormat(Alimer::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...) Alimer::Log::GetDefault()->LogFormat(Alimer::LogLevel::Warning, "%s -- %s", __current__func__, __VA_ARGS__)

#ifdef ALIMER_ERRORS_AS_WARNINGS
#define LOG_ERROR LOG_WARN
#else
#define LOG_ERROR(...) do \
    { \
        Alimer::Log::GetDefault()->LogFormat(Alimer::LogLevel::Error, "%s -- %s", __current__func__, __VA_ARGS__); \
        ALIMER_BREAKPOINT(); \
        std::exit(-1); \
    } while (0)
#endif
