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

namespace Alimer
{
    class ALIMER_API Stopwatch final
    {
    public:
        static constexpr uint64_t TicksPerMillisecond = 10000;
        static constexpr uint64_t TicksPerSecond = 10000000;

        /// Constructor.
        Stopwatch();
        /// Destructor.
        ~Stopwatch();

        void Reset();
        void Start();
        void Stop();
        void Restart();

        bool IsRunning() const { return isRunning; }
        uint64_t GetElapsedTicks() const;
        uint64_t GetElapsedMilliseconds() const;

        static uint64_t GetFrequency();
        static uint64_t GetTimestamp();

    private:
        
        bool isRunning = false;
        uint64_t elapsed = 0;
        uint64_t startTimeStamp = 0;
    };
}
