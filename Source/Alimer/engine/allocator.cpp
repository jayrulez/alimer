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

#include "engine/allocator.h"
#include <string.h>
#include <malloc.h>

namespace Alimer
{
    void* DefaultAllocator::allocate(size_t size)
    {
        return malloc(size);
    }

    void DefaultAllocator::deallocate(void* ptr)
    {
        free(ptr);
    }

    void* DefaultAllocator::reallocate(void* ptr, size_t size)
    {
        return realloc(ptr, size);
    }

#ifdef _WIN32
    void* DefaultAllocator::allocateAligned(size_t size, size_t align)
    {
        return _aligned_malloc(size, align);
    }


    void DefaultAllocator::deallocateAligned(void* ptr)
    {
        _aligned_free(ptr);
    }

    void* DefaultAllocator::reallocateAligned(void* ptr, size_t size, size_t align)
    {
        return _aligned_realloc(ptr, size, align);
    }
#else
    void* DefaultAllocator::allocateAligned(size_t size, size_t align)
    {
        return aligned_alloc(align, size);
    }

    void DefaultAllocator::deallocateAligned(void* ptr)
    {
        free(ptr);
    }

    void* DefaultAllocator::reallocateAligned(void* ptr, size_t size, size_t align)
    {
        // POSIX and glibc do not provide a way to realloc with alignment preservation
        if (size == 0) {
            free(ptr);
            return nullptr;
        }
        void* newptr = aligned_alloc(align, size);
        if (newptr == nullptr) {
            return nullptr;
        }
        memcpy(newptr, ptr, malloc_usable_size(ptr));
        free(ptr);
        return newptr;
    }
#endif
}
