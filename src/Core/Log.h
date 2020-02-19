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

#include "Core/Preprocessor.h"
#include <EASTL/string.h>

namespace Alimer
{
    enum class LogLevel : uint32_t
    {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Off
    };

    class ALIMER_API Logger final
    {
    public:
        Logger(const eastl::string& name);
        ~Logger();

        void Log(LogLevel level, const eastl::string& message);

    private:
        eastl::string _name;
    };

    class ALIMER_API Log final
    {
    public:
        static Logger* GetDefault();
    };
}

#define ALIMER_LOGTRACE(message) Alimer::Log::GetDefault()->Log(Alimer::LogLevel::Trace, message)
#define ALIMER_LOGDEBUG(message) Alimer::Log::GetDefault()->Log(Alimer::LogLevel::Debug, message)
#define ALIMER_LOGINFO(message) Alimer::Log::GetDefault()->Log(Alimer::LogLevel::Info, message)
#define ALIMER_LOGWARN(message) Alimer::Log::GetDefault()->Log(Alimer::LogLevel::Warn, message)
#define ALIMER_LOGERROR(message) Alimer::Log::GetDefault()->Log(Alimer::LogLevel::Error, message)
