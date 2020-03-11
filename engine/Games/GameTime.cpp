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

#include "Games/GameTime.h"
#include "Games/Game.h"
#include "Diagnostics/Stopwatch.h"
#include <cmath>

namespace alimer
{
    GameTime::GameTime()
        : targetElapsedTicks(TicksPerSecond / 60)
    {
        qpcFrequency = Stopwatch::GetFrequency();
        qpcLastTime = Stopwatch::GetTimestamp();

        // Initialize max delta to 1/10 of a second.
        qpcMaxDelta = static_cast<uint64_t>(qpcFrequency / 10);
    }

    void GameTime::ResetElapsedTime()
    {
        qpcLastTime = Stopwatch::GetTimestamp();

        leftOverTicks = 0;
        framesPerSecond = 0;
        framesThisSecond = 0;
        qpcSecondCounter = 0;
    }

    void GameTime::Tick(const std::function<void()> update)
    {
        // Query the current time.
        uint64_t currentTime = Stopwatch::GetTimestamp();
        uint64_t timeDelta = currentTime - qpcLastTime;

        qpcLastTime = currentTime;
        qpcSecondCounter += timeDelta;

        // Clamp excessively large time deltas (e.g. after paused in the debugger).
        if (timeDelta > qpcMaxDelta)
        {
            timeDelta = qpcMaxDelta;
        }

        // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
        timeDelta *= TicksPerSecond;
        timeDelta /= qpcFrequency;

        uint32_t lastFrameCount = frameCount;

        if (isFixedTimeStep)
        {
            // Fixed timestep update logic

            // If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
            // the clock to exactly match the target value. This prevents tiny and irrelevant errors
            // from accumulating over time. Without this clamping, a game that requested a 60 fps
            // fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
            // accumulate enough tiny errors that it would drop a frame. It is better to just round
            // small deviations down to zero to leave things running smoothly.

            if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(timeDelta - targetElapsedTicks))) < TicksPerSecond / 4000)
            {
                timeDelta = targetElapsedTicks;
            }

            leftOverTicks += timeDelta;

            while (leftOverTicks >= targetElapsedTicks)
            {
                elapsedTicks = targetElapsedTicks;
                totalTicks += targetElapsedTicks;
                leftOverTicks -= targetElapsedTicks;
                frameCount++;

                update();
            }
        }
        else
        {
            // Variable timestep update logic.
            elapsedTicks = timeDelta;
            totalTicks += timeDelta;
            leftOverTicks = 0;
            frameCount++;

            update();
        }

        // Track the current framerate.
        if (frameCount != lastFrameCount)
        {
            framesThisSecond++;
        }

        if (qpcSecondCounter >= qpcFrequency)
        {
            framesPerSecond = framesThisSecond;
            framesThisSecond = 0;
            qpcSecondCounter %= qpcFrequency;
        }
    }
}
