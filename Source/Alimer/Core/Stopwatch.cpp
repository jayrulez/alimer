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

#include "Core/Stopwatch.h"

#if defined(_WIN32) || defined(WINAPI_FAMILY)
#include <foundation/windows.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#include <mach/mach_time.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#endif

namespace Alimer
{
    struct TimerGlobalInitializer
    {
        uint64_t frequency;
        bool monotonic = false;

        TimerGlobalInitializer()
        {
#if defined(_WIN32) || defined(WINAPI_FAMILY)
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            frequency = f.QuadPart;
#elif defined(__APPLE__)
            mach_timebase_info_data_t info;
            mach_timebase_info(&info);
            frequency = (info.denom * 1e9) / info.numer;
#elif defined(__EMSCRIPTEN__)
            frequency = 1000000;
#else
#if defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
            struct timespec ts;

            if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
            {
                monotonic = true;
                frequency = 1000000000;
            }
            else
#endif
            {
                frequency = 1000000;
            }
#endif
        }
    };

    TimerGlobalInitializer s_timeGlobalInitializer;

    Stopwatch::Stopwatch()
    {
        Reset();
    }

    Stopwatch::~Stopwatch()
    {

    }

    uint64_t Stopwatch::GetFrequency()
    {
        return s_timeGlobalInitializer.frequency;
    }

    uint64_t Stopwatch::GetTimestamp()
    {
#if defined(_WIN32) || defined(WINAPI_FAMILY)
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return t.QuadPart;
#elif defined(__APPLE__)
        return mach_absolute_time();
#elif defined(__EMSCRIPTEN__)
        return (uint64_t)(emscripten_get_now() * 1000.0);
#else
#if defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
        if (s_timeGlobalInitializer.monotonic)
        {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            return (uint64_t)ts.tv_sec * (uint64_t)1000000000 + (uint64_t)ts.tv_nsec;
        }
        else
#endif
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec;
        }

#endif
    }

    void Stopwatch::Reset()
    {
        elapsed = 0;
        isRunning = false;
        startTimeStamp = 0;
    }

    void Stopwatch::Start()
    {
        // Calling start on a running Stopwatch is a no-op.
        if (!isRunning)
        {
            startTimeStamp = GetTimestamp();
            isRunning = true;
        }
    }

    void Stopwatch::Stop()
    {
        // Calling stop on a stopped Stopwatch is a no-op.
        if (isRunning)
        {
            uint64_t endTimeStamp = GetTimestamp();
            uint64_t elapsedThisPeriod = endTimeStamp - startTimeStamp;
            elapsed += elapsedThisPeriod;
            isRunning = false;

            if (elapsed < 0)
            {
                // When measuring small time periods the Stopwatch.Elapsed*
                // properties can return negative values.  This is due to
                // bugs in the basic input/output system (BIOS) or the hardware
                // abstraction layer (HAL) on machines with variable-speed CPUs
                // (e.g. Intel SpeedStep).

                elapsed = 0;
            }
        }
    }

    void Stopwatch::Restart()
    {
        elapsed = 0;
        startTimeStamp = GetTimestamp();
        isRunning = true;
    }

    uint64_t Stopwatch::GetElapsedTicks() const
    {
        uint64_t timeElapsed = elapsed;

        if (isRunning)
        {
            // If the Stopwatch is running, add elapsed time since
            // the Stopwatch is started last time.
            uint64_t currentTimeStamp = GetTimestamp();
            uint64_t elapsedUntilNow = currentTimeStamp - startTimeStamp;
            timeElapsed += elapsedUntilNow;
        }

        return timeElapsed;
    }

    uint64_t Stopwatch::GetElapsedMilliseconds() const
    {
        static double s_tickFrequency = (double)TicksPerSecond / s_timeGlobalInitializer.frequency;
        return uint64_t(GetElapsedTicks() * s_tickFrequency) / TicksPerMillisecond;
    }
}
