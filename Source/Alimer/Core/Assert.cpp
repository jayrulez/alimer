//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#include "Core/Assert.h"
#include <cstdio>

#if ALIMER_PLATFORM_WINDOWS
#include "Platform/Windows/Windows.PCH.h"
#endif

namespace Alimer
{
    AssertFailBehavior DefaultHandler(const char* condition,
        const char* msg,
        const char* file,
        const int line)
    {
        const uint64_t BufferSize = 2048;
        char buffer[BufferSize];
        snprintf(buffer, BufferSize, "%s(%d): Assert Failure: ", file, line);

        if (condition != nullptr) {
            snprintf(buffer, BufferSize, "%s'%s' ", buffer, condition);
        }

        if (msg != nullptr) {
            snprintf(buffer, BufferSize, "%s%s", buffer, msg);
        }

        snprintf(buffer, BufferSize, "%s\n", buffer);

#if defined(_WIN32) || defined(_WIN64)
        OutputDebugStringA(buffer);
#else
        fprintf(stderr, "%s", buffer);
#endif

        return AssertFailBehavior::Halt;
    }

    AssertHandler& GetAssertHandlerInstance()
    {
        static AssertHandler s_handler = &DefaultHandler;
        return s_handler;
    }

    AssertHandler GetAssertHandler()
    {
        return GetAssertHandlerInstance();
    }

    void SetAssertHandler(AssertHandler newHandler)
    {
        GetAssertHandlerInstance() = newHandler;
    }

    AssertFailBehavior ReportAssertFailure(const char* condition, const char* file, const int line, const char* msg, ...)
    {
        const char* message = nullptr;
        if (msg != nullptr)
        {
            char messageBuffer[1024];
            {
                va_list args;
                va_start(args, msg);
                vsnprintf(messageBuffer, 1024, msg, args);
                va_end(args);
            }

            message = messageBuffer;
        }

        return GetAssertHandlerInstance()(condition, message, file, line);
    }
}
