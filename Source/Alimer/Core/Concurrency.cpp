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

namespace Alimer
{
    int32_t AtomicIncrement(volatile int32_t* value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedIncrement((volatile long*)value);
#else
        return __sync_fetch_and_add(value, 1) + 1;
#endif
    }

    int64_t AtomicIncrement(volatile int64_t* value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedIncrement64((volatile long long*)value);
#else
        return __sync_fetch_and_add(value, 1) + 1;
#endif
    }

    int32_t AtomicDecrement(volatile int32_t* value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedDecrement((volatile long*)value);
#else
        return __sync_fetch_and_sub(value, 1) - 1;
#endif
    }

    int64_t AtomicDecrement(volatile int64_t* value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedDecrement64((volatile long long*)value);
#else
        return __sync_fetch_and_sub(value, 1) - 1;
#endif
    }

    int32_t AtomicAdd(int32_t volatile* addend, int32_t value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedExchangeAdd((volatile long*)addend, value);
#else
        return __sync_fetch_and_add(addend, value);
#endif
    }

    int64_t AtomicAdd(int64_t volatile* addend, int64_t value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedExchangeAdd64((volatile long long*)addend, value);
#else
        return __sync_fetch_and_add(addend, value);
#endif
    }

    int32_t AtomicSubtract(int32_t volatile* addend, int32_t value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedExchangeAdd((volatile long*)addend, -value);
#else
        return __sync_fetch_and_sub(addend, value);
#endif
    }

    int64_t AtomicSubtract(int64_t volatile* addend, int64_t value) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedExchangeAdd64((volatile long long*)addend, -value);
#else
        return __sync_fetch_and_sub(addend, value);
#endif
    }

    bool CompareAndExchange(int32_t volatile* dest, int32_t exchange, int32_t comperand) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedCompareExchange((volatile long*)dest, exchange, comperand) == comperand;
#else
        return __sync_bool_compare_and_swap(dest, comperand, exchange);
#endif
    }

    bool CompareAndExchange64(int64_t volatile* dest, int64_t exchange, int64_t comperand) {
#if ALIMER_PLATFORM_WINDOWS
        return _InterlockedCompareExchange64(dest, exchange, comperand) == comperand;
#else
        return __sync_bool_compare_and_swap(dest, comperand, exchange);
#endif
    }
} 
