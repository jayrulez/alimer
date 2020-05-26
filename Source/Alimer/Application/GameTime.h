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

#include "engine/Stopwatch.h"

namespace Alimer
{
    class ALIMER_API GameTime final
    {

    public:
        GameTime();
         ~GameTime() = default;

         template<typename TUpdate>
         void Tick(const TUpdate& update)
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

         // Get elapsed time since the previous Update call.
         uint64_t GetElapsedTicks() const { return elapsedTicks; }
         double GetElapsedSeconds() const { return TicksToSeconds(elapsedTicks); }

         // Get total time since the start of the program.
         uint64_t GetTotalTicks() const { return totalTicks; }
         double GetTotalSeconds() const { return TicksToSeconds(totalTicks); }

         // Get total number of updates since start of the program.
         uint32_t GetFrameCount() const { return frameCount; }

         // Get the current framerate.
         uint32_t GetFramesPerSecond() const { return framesPerSecond; }

         // Set whether to use fixed or variable timestep mode.
         void SetFixedTimeStep(bool isFixedTimestep) { isFixedTimeStep = isFixedTimestep; }

         // Set how often to call Update when in fixed timestep mode.
         void SetTargetElapsedTicks(uint64_t targetElapsed) { targetElapsedTicks = targetElapsed; }
         void SetTargetElapsedSeconds(double targetElapsed) { targetElapsedTicks = SecondsToTicks(targetElapsed); }

         void ResetElapsedTime();

         // Integer format represents time using 10,000,000 ticks per second.
         static constexpr uint64_t TicksPerSecond = 10000000;

         static double TicksToSeconds(uint64_t ticks) { return static_cast<double>(ticks) / TicksPerSecond; }
         static uint64_t SecondsToTicks(double seconds) { return static_cast<uint64_t>(seconds * TicksPerSecond); }

    private:
        // Source timing data uses QPC units.
        uint64_t qpcFrequency;
        uint64_t qpcLastTime;
        uint64_t qpcMaxDelta;

        // Derived timing data uses a canonical tick format.
        uint64_t elapsedTicks = 0;
        uint64_t totalTicks = 0;
        uint64_t leftOverTicks = 0;

        // Members for tracking the framerate.
        uint32_t frameCount = 0;
        uint32_t framesPerSecond = 0;
        uint32_t framesThisSecond = 0;
        uint64_t qpcSecondCounter = 0;

        // Members for configuring fixed timestep mode.
        bool isFixedTimeStep = false;
        uint64_t targetElapsedTicks;
    };
}
