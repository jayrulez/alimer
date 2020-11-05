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
#pragma once

#include "PlatformDef.h"

namespace alimer
{
    enum class AssertFailBehavior : uint8_t
    {
        Halt,
        Continue,
    };

    typedef AssertFailBehavior (*AssertHandler)(const char* condition, const char* msg, const char* file, int line);

    ALIMER_API AssertHandler GetAssertHandler();
    ALIMER_API void SetAssertHandler(AssertHandler newHandler);

    ALIMER_API AssertFailBehavior ReportAssertFailure(const char* condition, const char* file, int line, const char* msg, ...);
}

/** Assert macro */
#ifndef ALIMER_ENABLE_ASSERT
    #if (defined(_DEBUG) || defined(PROFILE))
        #define ALIMER_ENABLE_ASSERT 1
    #else
        #define ALIMER_ENABLE_ASSERT 0
    #endif
#endif

#if ALIMER_ENABLE_ASSERT

    #define ALIMER_ASSERT(cond)                                                                                                            \
        do                                                                                                                                 \
        {                                                                                                                                  \
            if (!(cond))                                                                                                                   \
            {                                                                                                                              \
                if (alimer::ReportAssertFailure(#cond, __FILE__, __LINE__, 0) == alimer::AssertFailBehavior::Halt)                         \
                    ALIMER_DEBUG_BREAK();                                                                                                  \
            }                                                                                                                              \
        } while (0)

    #define ALIMER_ASSERT_MSG(cond, msg, ...)                                                                                              \
        do                                                                                                                                 \
        {                                                                                                                                  \
            if (!(cond))                                                                                                                   \
            {                                                                                                                              \
                if (alimer::ReportAssertFailure(#cond, __FILE__, __LINE__, (msg), __VA_ARGS__) == alimer::AssertFailBehavior::Halt)        \
                    ALIMER_DEBUG_BREAK();                                                                                                  \
            }                                                                                                                              \
        } while (0)

    #define ALIMER_ASSERT_FAIL(msg, ...)                                                                                                   \
        do                                                                                                                                 \
        {                                                                                                                                  \
            if (alimer::ReportAssertFailure(0, __FILE__, __LINE__, (msg), __VA_ARGS__) == alimer::AssertFailBehavior::Halt)                \
                ALIMER_DEBUG_BREAK();                                                                                                      \
        } while (0)

    #define ALIMER_VERIFY(cond) ALIMER_ASSERT(cond)
    #define ALIMER_VERIFY_MSG(cond, msg, ...) ALIMER_ASSERT_MSG(cond, msg, ##__VA_ARGS__)

#else
    #define ALIMER_ASSERT(condition)                                                                                                       \
        do                                                                                                                                 \
        {                                                                                                                                  \
        } while (0)
    #define ALIMER_ASSERT_MSG(condition, msg, ...)                                                                                         \
        do                                                                                                                                 \
        {                                                                                                                                  \
        } while (0)
    #define ALIMER_ASSERT_FAIL(msg, ...)                                                                                                   \
        do                                                                                                                                 \
        {                                                                                                                                  \
        } while (0)
    #define ALIMER_VERIFY(cond) (void)(cond)
    #define ALIMER_VERIFY_MSG(cond, msg, ...)                                                                                              \
        do                                                                                                                                 \
        {                                                                                                                                  \
            (void)(cond);                                                                                                                  \
            ALIMER_UNUSED(msg);                                                                                                            \
        } while (0)
#endif
