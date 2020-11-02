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
    ALIMER_API int32_t AtomicIncrement(volatile int32_t* value);
    ALIMER_API int64_t AtomicIncrement(volatile int64_t* value);
    ALIMER_API int32_t AtomicDecrement(volatile int32_t* value);
    ALIMER_API int64_t AtomicDecrement(volatile int64_t* value);

    ALIMER_API int32_t AtomicAdd(volatile int32_t* addend, int32_t value);
    ALIMER_API int64_t AtomicAdd(volatile int64_t* addend, int64_t value);
    ALIMER_API int32_t AtomicSubtract(volatile int32_t* addend, int32_t value);
    ALIMER_API int64_t AtomicSubtract(volatile int64_t* addend, int64_t value);

    ALIMER_API bool CompareAndExchange(volatile int32_t* dest, int32_t exchange, int32_t comperand);
    ALIMER_API bool CompareAndExchange64(volatile int64_t* dest, int64_t exchange, int64_t comperand);
}
