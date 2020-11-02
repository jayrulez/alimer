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
// Code is based on bsf framework: https://github.com/GameFoundry/bsf

#pragma once

#include "PlatformDef.h"
#include <memory>

#if (ALIMER_PLATFORM_WINDOWS || ALIMER_PLATFORM_UWP || ALIMER_PLATFORM_XBOXONE)
#    include <malloc.h>
#endif

#if defined(__ANDROID_API__) && (__ANDROID_API__ < 16)
#    include <cstdlib>
static void* alimer_aligned_alloc(size_t alignment, size_t size)
{
    // alignment must be >= sizeof(void*)
    if (alignment < sizeof(void*))
    {
        alignment = sizeof(void*);
    }

    return memalign(alignment, size);
}
#elif defined(__APPLE__) || defined(__ANDROID__) || (defined(__linux__) && defined(__GLIBCXX__) && !defined(_GLIBCXX_HAVE_ALIGNED_ALLOC))
#    include <cstdlib>

#    if defined(__APPLE__)
#        include <AvailabilityMacros.h>
#    endif

static void* alimer_aligned_alloc(size_t alignment, size_t size)
{
#    if defined(__APPLE__) && (defined(MAC_OS_X_VERSION_10_16) || defined(__IPHONE_14_0))
#        if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_16 || __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_14_0
    // For C++14, usr/include/malloc/_malloc.h declares aligned_alloc()) only
    // with the MacOSX11.0 SDK in Xcode 12 (which is what adds
    // MAC_OS_X_VERSION_10_16), even though the function is marked
    // availabe for 10.15. That's why the preprocessor checks for 10.16 but
    // the __builtin_available checks for 10.15.
    // People who use C++17 could call aligned_alloc with the 10.15 SDK already.
    if (__builtin_available(macOS 10.15, iOS 13, *))
        return aligned_alloc(alignment, size);
#        endif
#    endif
    // alignment must be >= sizeof(void*)
    if (alignment < sizeof(void*))
    {
        alignment = sizeof(void*);
    }

    void* pointer;
    if (posix_memalign(&pointer, alignment, size) == 0)
        return pointer;
    return VMA_NULL;
}
#elif defined(_WIN32)
static void* alimer_aligned_alloc(size_t alignment, size_t size) { return _aligned_malloc(size, alignment); }

static void alimer_aligned_free(void* ptr) { _aligned_free(ptr); }
#else
static void* alimer_aligned_alloc(size_t alignment, size_t size) { return aligned_alloc(alignment, size); }
#endif

#if !defined(_WIN32)
static void alimer_aligned_free(void* ptr) { free(ptr); }
#endif

namespace alimer
{
    /**
     * General allocator provided by the OS. Use for persistent long term allocations, and allocations that don't
     * happen often.
     */
    class GenAlloc
    {};

    /// Thread safe class used for storing total number of memory allocations and deallocations, primarily for statistic purposes.
    class ALIMER_API MemoryCounter
    {
    private:
        friend class MemoryAllocatorBase;

        // Threadlocal data can't be exported, so some magic to make it accessible from MemoryAllocator
        static void incAllocCount() { ++Allocs; }
        static void incFreeCount() { ++Frees; }

        static ALIMER_THREADLOCAL uint64_t Allocs;
        static ALIMER_THREADLOCAL uint64_t Frees;
    };

    /** Base class all memory allocators need to inherit. Provides allocation and free counting. */
    class MemoryAllocatorBase
    {
    protected:
        static void IncreaseAllocCount() { MemoryCounter::incAllocCount(); }
        static void IncreaseFreeCount() { MemoryCounter::incFreeCount(); }
    };

    template <class T>
    class MemoryAllocator : public MemoryAllocatorBase
    {
    public:
        static void* allocate(size_t size)
        {
#if defined(_DEBUG)
            IncreaseAllocCount();
#endif
            return malloc(size);
        }

        static void free(void* ptr)
        {
#if defined(_DEBUG)
            IncreaseFreeCount();
#endif

            ::free(ptr);
        }

        static void* allocate_aligned(size_t alignment, size_t size)
        {
#if defined(_DEBUG)
            IncreaseAllocCount();
#endif

            return alimer_aligned_alloc(alignment, size);
        }

        /** Frees memory allocated with allocateAligned() */
        static void free_aligned(void* ptr)
        {
#if defined(_DEBUG)
            IncreaseFreeCount();
#endif

            alimer_aligned_free(ptr);
        }
    };

    /** Allocates the specified number of bytes. */
    template <class Alloc>
    constexpr void* alimer_alloc(size_t count)
    {
        return MemoryAllocator<Alloc>::allocate(count);
    }

    /** Allocates enough bytes to hold the specified type, but doesn't construct it. */
    template <class T, class Alloc>
    constexpr T* alimer_alloc()
    {
        return (T*) MemoryAllocator<Alloc>::allocate(sizeof(T));
    }

    /** Frees all the bytes allocated at the specified location. */
    template <class Alloc>
    constexpr void alimer_free(void* ptr)
    {
        MemoryAllocator<Alloc>::free(ptr);
    }

    /** Create a new object with the specified allocator and the specified parameters. */
    template <class T, class Alloc, class... Args>
    constexpr T* alimer_new(Args&&... args)
    {
        return new (alimer_alloc<T, Alloc>()) T(std::forward<Args>(args)...);
    }

    /** Creates and constructs an array of @p count elements. */
    template <class T, class Alloc>
    constexpr T* alimer_new_count(size_t count)
    {
        T* ptr = (T*) MemoryAllocator<Alloc>::allocate(sizeof(T) * count);

        for (size_t i = 0; i < count; ++i)
        {
            new (&ptr[i]) T;
        }

        return ptr;
    }

    /** Destructs and frees the specified object. */
    template <class T, class Alloc = GenAlloc>
    constexpr void alimer_delete(T* ptr)
    {
        (ptr)->~T();

        MemoryAllocator<Alloc>::free(ptr);
    }

    /** Destructs and frees the specified array of objects. */
    template <class T, class Alloc = GenAlloc>
    constexpr void alimer_delete_count(T* ptr, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            ptr[i].~T();
        }

        MemoryAllocator<Alloc>::free(ptr);
    }

    /** Callable struct that acts as a proxy for bs_delete */
    template <class T, class Alloc = GenAlloc>
    struct Deleter
    {
        constexpr Deleter() noexcept = default;

        /** Constructor enabling deleter conversion and therefore polymorphism with smart points (if they use the same allocator). */
        template <class T2, std::enable_if_t<std::is_convertible<T2*, T*>::value, int> = 0>
        constexpr Deleter(const Deleter<T2, Alloc>& other) noexcept
        {}

        void operator()(T* ptr) const { alimer_delete<T, Alloc>(ptr); }
    };

    /** Allocates the specified number of bytes. */
    inline void* alimer_alloc(size_t count) { return MemoryAllocator<GenAlloc>::allocate(count); }

    /** Allocates enough bytes to hold the specified type, but doesn't construct it. */
    template <class T>
    constexpr T* alimer_alloc()
    {
        return (T*) MemoryAllocator<GenAlloc>::allocate(sizeof(T));
    }

    /// Allocates the specified number of bytes aligned to the provided boundary. Boundary is in bytes and must be a power of two.
    inline void* alimer_alloc_aligned(size_t alignment, size_t count)
    {
        return MemoryAllocator<GenAlloc>::allocate_aligned(alignment, count);
    }

    /** Create a new object with the specified allocator and the specified parameters. */
    template <class T, class... Args>
    constexpr T* alimer_new(Args&&... args)
    {
        return new (alimer_alloc<T, GenAlloc>()) T(std::forward<Args>(args)...);
    }

    /** Creates and constructs an array of @p count elements. */
    template <class T>
    constexpr T* alimer_new_count(size_t count)
    {
        T* ptr = (T*) MemoryAllocator<GenAlloc>::allocate(count * sizeof(T));

        for (size_t i = 0; i < count; ++i)
        {
            new (&ptr[i]) T;
        }

        return ptr;
    }

    /** Frees all the bytes allocated at the specified location. */
    inline void alimer_free(void* ptr) { MemoryAllocator<GenAlloc>::free(ptr); }

    /** Frees memory previously allocated with bs_alloc_aligned(). */
    inline void alimer_free_aligned(void* ptr) { MemoryAllocator<GenAlloc>::free_aligned(ptr); }

    /** Allocator for the standard library that internally uses engine memory allocator. */
    template <class T, class Alloc = GenAlloc>
    class StdAlloc
    {
    public:
        using value_type      = T;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;

        constexpr StdAlloc() noexcept {}

        constexpr StdAlloc(const StdAlloc&) noexcept = default;
        template <class U>
        constexpr StdAlloc(const StdAlloc<U>&) noexcept
        {}

        [[nodiscard]] T* allocate(const size_type count)
        {
            if (count == 0)
                return nullptr;

            void* const ptr = alimer_alloc<Alloc>(count * sizeof(T));
            if (!ptr)
                return nullptr;        // Error

            return static_cast<T*>(ptr);
        }

        void deallocate(T* ptr, const size_type) { alimer_free<Alloc>(ptr); }
    };
}
