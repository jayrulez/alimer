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

#include "Core/Platform.h"

namespace alimer
{
    ALIMER_API i32 AtomicIncrement(volatile i32* value);
    ALIMER_API i64 AtomicIncrement(volatile i64* value);
    ALIMER_API i32 AtomicDecrement(volatile i32* value);
    ALIMER_API i64 AtomicDecrement(volatile i64* value);

    ALIMER_API i32 AtomicAdd(i32 volatile* addend, i32 value);
    ALIMER_API i64 AtomicAdd(i64 volatile* addend, i64 value);
    ALIMER_API i32 AtomicSubtract(i32 volatile* addend, i32 value);
    ALIMER_API i64 AtomicSubtract(i64 volatile* addend, i64 value);

    ALIMER_API bool CompareAndExchange(i32 volatile* dest, i32 exchange, i32 comperand);
    ALIMER_API bool CompareAndExchange64(i64 volatile* dest, i64 exchange, i64 comperand);
}
