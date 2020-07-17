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
#include <fmt/format.h>

namespace alimer
{
    enum class LogLevel : uint32
    {
        Verbose,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off,
        Count
    };

    ALIMER_API void Log(LogLevel level, const std::string& message);
    ALIMER_API void Verbose(const std::string& message);
    ALIMER_API void Debug(const std::string& message);
    ALIMER_API void Info(const std::string& message);
    ALIMER_API void Warn(const std::string& message);
    ALIMER_API void Error(const std::string& message);
    ALIMER_API void Critical(const std::string& message);
}

#define LOGV(...) alimer::Verbose(fmt::format(__VA_ARGS__));
#define LOGD(...) alimer::Debug(fmt::format(__VA_ARGS__));
#define LOGI(...) alimer::Info(fmt::format(__VA_ARGS__));
#define LOGW(...) alimer::Warn(fmt::format(__VA_ARGS__));
#define LOGE(...) alimer::Error(fmt::format("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__)));
#define LOGC(...) alimer::Critical(fmt::format("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__)));
