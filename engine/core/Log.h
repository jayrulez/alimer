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

#include "core/Preprocessor.h"
#include <string>

namespace alimer
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

#define ALIMER_LOGDEBUG(message) alimer::Log::GetDefault()->Log(alimer::LogLevel::Debug, message)
#define ALIMER_LOGINFO(message) alimer::Log::GetDefault()->Log(alimer::LogLevel::Info, message)
#define ALIMER_LOGWARN(message) alimer::Log::GetDefault()->Log(alimer::LogLevel::Warning, message)
#define ALIMER_LOGERROR(message) alimer::Log::GetDefault()->Log(alimer::LogLevel::Error, message)

#define ALIMER_TRACE(message, ...) alimer::Log::GetDefault()->LogFormat(alimer::LogLevel::Trace, message, ##__VA_ARGS__)
#define ALIMER_LOGD(message, ...) alimer::Log::GetDefault()->LogFormat(alimer::LogLevel::Debug, message, ##__VA_ARGS__)
#define ALIMER_LOGI(message, ...) alimer::Log::GetDefault()->LogFormat(alimer::LogLevel::Info, message, ##__VA_ARGS__)
#define ALIMER_LOGW(message, ...) alimer::Log::GetDefault()->LogFormat(alimer::LogLevel::Warning, message, ##__VA_ARGS__)
#define ALIMER_LOGE(message, ...) alimer::Log::GetDefault()->LogFormat(alimer::LogLevel::Error, message, ##__VA_ARGS__)
