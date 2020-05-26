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

#include <foundation/platform.h>

#ifndef _WIN32
#   include <new>
#endif

#define ALIMER_NEW(allocator, ...) new (Alimer::NewPlaceholder(), (allocator).AllocateAligned(sizeof(__VA_ARGS__), alignof(__VA_ARGS__))) __VA_ARGS__
#define ALIMER_DELETE(allocator, var) (allocator).DeleteObject(var);

namespace Alimer { struct NewPlaceholder {}; }
inline void* operator new(size_t, Alimer::NewPlaceholder, void* where) { return where; }
inline void operator delete(void*, Alimer::NewPlaceholder, void*) { }

namespace Alimer
{
    struct ALIMER_API IAllocator
    {
        virtual ~IAllocator() {}

        virtual void* Allocate(size_t size) = 0;
        virtual void Deallocate(void* ptr) = 0;
        virtual void* Reallocate(void* ptr, size_t size) = 0;

        virtual void* AllocateAligned(size_t size, size_t align) = 0;
        virtual void DeallocateAligned(void* ptr) = 0;
        virtual void* ReallocateAligned(void* ptr, size_t size, size_t align) = 0;

        template <typename T> void DeleteObject(T* ptr) {
            if (ptr)
            {
                ptr->~T();
                DeallocateAligned(ptr);
            }
        }
    };

    struct ALIMER_API DefaultAllocator final : IAllocator
    {
        void* Allocate(size_t size) override;
        void Deallocate(void* ptr) override;
        void* Reallocate(void* ptr, size_t size) override;

        void* AllocateAligned(size_t size, size_t align) override;
        void DeallocateAligned(void* ptr) override;
        void* ReallocateAligned(void* ptr, size_t size, size_t align) override;
    };
}
