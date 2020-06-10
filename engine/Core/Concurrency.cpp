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

#include "Core/Concurrency.h"

#if !ALIMER_PLATFORM_WINDOWS
#include <pthread.h>
#endif

namespace alimer
{
    i32 AtomicIncrement(volatile i32* value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedIncrement((volatile long*)value);
#else
        return __sync_fetch_and_add(value, 1) + 1;
#endif
    }

    i64 AtomicIncrement(volatile i64* value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedIncrement64((volatile long long*)value);
#else
        return __sync_fetch_and_add(value, 1) + 1;
#endif
    }

    i32 AtomicDecrement(volatile i32* value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedDecrement((volatile long*)value);
#else
        return __sync_fetch_and_sub(value, 1) - 1;
#endif
    }

    i64 AtomicDecrement(volatile i64* value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedDecrement64((volatile long long*)value);
#else
        return __sync_fetch_and_sub(value, 1) - 1;
#endif
    }

    i32 AtomicAdd(i32 volatile* addend, i32 value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedExchangeAdd((volatile long*)addend, value);
#else
        return __sync_fetch_and_add(addend, value);
#endif
    }

    i64 AtomicAdd(i64 volatile* addend, i64 value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedExchangeAdd64((volatile long long*)addend, value);
#else
        return __sync_fetch_and_add(addend, value);
#endif
    }

    i32 AtomicSubtract(i32 volatile* addend, i32 value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedExchangeAdd((volatile long*)addend, -value);
#else
        return __sync_fetch_and_sub(addend, value);
#endif
    }

    i64 AtomicSubtract(i64 volatile* addend, i64 value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedExchangeAdd64((volatile long long*)addend, -value);
#else
        return __sync_fetch_and_sub(addend, value);
#endif
    }

    bool CompareAndExchange(i32 volatile* dest, i32 exchange, i32 comperand) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedCompareExchange((volatile long*)dest, exchange, comperand) == comperand;
#else
        return __sync_bool_compare_and_swap(dest, comperand, exchange);
#endif
    }

    bool CompareAndExchange64(i64 volatile* dest, i64 exchange, i64 comperand) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedCompareExchange64(dest, exchange, comperand) == comperand;
#else
        return __sync_bool_compare_and_swap(dest, comperand, exchange);
#endif
    }
} 
