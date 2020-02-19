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

// Compilers
#define ALIMER_VC 0
#define ALIMER_CLANG 0
#define ALIMER_SNC 0
#define ALIMER_GHS 0
#define ALIMER_GCC 0

// Platforms
#define ALIMER_XBOXONE 0
#define ALIMER_UWP 0
#define ALIMER_WIN64 0
#define ALIMER_WIN32 0
#define ALIMER_ANDROID 0
#define ALIMER_LINUX 0
#define ALIMER_IOS 0
#define ALIMER_OSX 0
#define ALIMER_EMSCRIPTEN 0

/**
Compiler defines, see http://sourceforge.net/p/predef/wiki/Compilers/
*/
#if defined(_MSC_VER)
    #undef ALIMER_VC
    #if _MSC_VER >= 1910
        #define ALIMER_VC 15
    #elif _MSC_VER >= 1900
        #define ALIMER_VC 14
    #elif _MSC_VER >= 1800
        #define ALIMER_VC 12
    #elif _MSC_VER >= 1700
        #define ALIMER_VC 11
    #elif _MSC_VER >= 1600
        #define ALIMER_VC 10
    #elif _MSC_VER >= 1500
        #define ALIMER_VC 9
    #else
        #error "Unknown VC version"
    #endif
#elif defined(__clang__)
    #undef ALIMER_CLANG
    #define ALIMER_CLANG 1
    #if defined (__clang_major__) 
        #define ALIMER_CLANG_MAJOR __clang_major__
    #elif defined (_clang_major)
        #define ALIMER_CLANG_MAJOR _clang_major
    #else
        #define ALIMER_CLANG_MAJOR 0
    #endif	
#elif defined(__SNC__)
    #undef ALIMER_SNC
    #define ALIMER_SNC 1
#elif defined(__ghs__)
    #undef ALIMER_GHS
    #define ALIMER_GHS 1
#elif defined(__GNUC__) /* note: __clang__, __SNC__, or __ghs__ implies __GNUC__ */
    #undef ALIMER_GCC
    #define ALIMER_GCC 1
#else
    #error "Unknown compiler"
#endif

/**
Operating system defines, see http://sourceforge.net/p/predef/wiki/OperatingSystems/
*/
#if defined(_XBOX_ONE)
    #undef ALIMER_XBOXONE
    #define ALIMER_XBOXONE 1
#elif defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_APP
    #undef ALIMER_UWP
    #define ALIMER_UWP 1 
#elif defined(_WIN64) // note: _XBOX_ONE implies _WIN64
    #undef ALIMER_WIN64
    #define ALIMER_WIN64 1
#elif defined(_WIN32) // note: _M_PPC implies _WIN32
    #undef ALIMER_WIN32
    #define ALIMER_WIN32 1
#elif defined(__ANDROID__)
    #undef ALIMER_ANDROID
    #define ALIMER_ANDROID 1
#elif defined(__linux__) || defined (__EMSCRIPTEN__) || defined(__CYGWIN__) /* note: __ANDROID__ implies __linux__ */
    #undef ALIMER_LINUX
    #define ALIMER_LINUX 1
#elif defined(__APPLE__) && (defined(__arm__) || defined(__arm64__))
    #undef ALIMER_IOS
    #define ALIMER_IOS 1
#elif defined(__APPLE__)
    #undef ALIMER_OSX
    #define ALIMER_OSX 1
#else
    #error "Unknown operating system"
#endif

/* family shortcuts */
// compiler
#define ALIMER_GCC_FAMILY (ALIMER_CLANG || ALIMER_SNC || ALIMER_GHS || ALIMER_GCC)
// os
#define ALIMER_WINDOWS (ALIMER_WIN32 || ALIMER_WIN64)
#define ALIMER_WINDOWS_FAMILY (ALIMER_WIN32 || ALIMER_WIN64 || ALIMER_UWP)
#define ALIMER_MICROSOFT_FAMILY (ALIMER_XBOXONE || ALIMER_WINDOWS_FAMILY)
#define ALIMER_LINUX_FAMILY (ALIMER_LINUX || ALIMER_ANDROID)
#define ALIMER_APPLE_FAMILY (ALIMER_IOS || ALIMER_OSX)                  // equivalent to #if __APPLE__
#define ALIMER_UNIX_FAMILY (ALIMER_LINUX_FAMILY || ALIMER_APPLE_FAMILY) // shortcut for unix/posix platforms

#if defined(__EMSCRIPTEN__)
    #undef ALIMER_EMSCRIPTEN
    #define ALIMER_EMSCRIPTEN 1
#endif

#if (ALIMER_WINDOWS_FAMILY || ALIMER_XBOXONE || ALIMER_PS4)
    #define ALIMER_DLL_EXPORT __declspec(dllexport)
    #define ALIMER_DLL_IMPORT __declspec(dllimport)
#else
    #define ALIMER_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define ALIMER_DLL_IMPORT
#endif

// Shared library exports
#if defined(ALIMER_EXPORTS)
    #define ALIMER_API ALIMER_DLL_EXPORT
#else
    #define ALIMER_API
#endif

/**
Inline macro
*/
#define ALIMER_INLINE inline
#if ALIMER_MICROSOFT_FAMILY
    #pragma inline_depth(255)
#endif

#if ALIMER_MICROSOFT_FAMILY
    #define ALIMER_NOINLINE __declspec(noinline)
    #define ALIMER_FORCE_INLINE __forceinline
    #define ALIMER_BREAKPOINT() __debugbreak();
    #define ALIMER_UNREACHABLE() __assume(false)
    #define ALIMER_ALIGN_OF(T) __alignof(T)
    #define ALIMER_ALIGN(alignment, decl) __declspec(align(alignment)) decl
    #define ALIMER_ALIGN_PREFIX(alignment) __declspec(align(alignment))
    #define ALIMER_ALIGN_SUFFIX(alignment)
#elif ALIMER_GCC_FAMILY
    #define ALIMER_NOINLINE __attribute__((noinline))
    #define ALIMER_FORCE_INLINE inline __attribute__((always_inline))
    #define ALIMER_BREAKPOINT() __builtin_trap();
    #define ALIMER_UNREACHABLE() __builtin_unreachable()
    #define ALIMER_ALIGN_OF(T)	__alignof__(T)
    #define ALIMER_OFFSET_OF(X, Y) __builtin_offsetof(X, Y)
    #define ALIMER_ALIGN(alignment, decl) decl __attribute__((aligned(alignment)))
    #define ALIMER_ALIGN_PREFIX(alignment)
    #define ALIMER_ALIGN_SUFFIX(alignment) __attribute__((aligned(alignment)))
#else
    #define ALIMER_NOINLINE
    #define ALIMER_FORCE_INLINE inline
    #define ALIMER_BREAKPOINT()
    #define ALIMER_ALIGN_OF(T) __alignof(T)
    #define ALIMER_OFFSET_OF(X, Y) offsetof(X, Y)
    #define ALIMER_ALIGN(alignment, decl)
    #define ALIMER_ALIGN_PREFIX(alignment)
    #define ALIMER_ALIGN_SUFFIX(alignment)
#endif

// Use for getting the amount of members of a standard C array.
#define ALIMER_COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

// Other defines
#define ALIMER_STRINGIZE_HELPER(X) #X
#define ALIMER_STRINGIZE(X) ALIMER_STRINGIZE_HELPER(X)

#define ALIMER_CONCAT_HELPER(X, Y) X##Y
#define ALIMER_CONCAT(X, Y) ALIMER_CONCAT_HELPER(X, Y)

#define ALIMER_UNUSED(x) do { (void)sizeof(x); } while(0)

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus

#include <type_traits>

#define ALIMER_DISABLE_COPY(ClassType) \
    ClassType(const ClassType&) = delete; ClassType& operator=(const ClassType&) = delete; \
    ClassType(ClassType&&) = default; ClassType& operator=(ClassType&&) = default;

#define ALIMER_DISABLE_COPY_MOVE(ClassType) \
    ClassType(const ClassType&) = delete; ClassType& operator=(const ClassType&) = delete; \
    ClassType(ClassType&&) = delete; ClassType& operator=(ClassType&&) = delete;

#define ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(EnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) \
    { return EnumType(((std::underlying_type<EnumType>::type)a) | ((std::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator |= (EnumType &a, EnumType b) \
    { return a = a | b; } \
inline constexpr EnumType operator & (EnumType a, EnumType b) \
    { return EnumType(((std::underlying_type<EnumType>::type)a) & ((std::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator &= (EnumType &a, EnumType b) \
    { return a = a & b; } \
inline constexpr EnumType operator ~ (EnumType a) \
    { return EnumType(~((std::underlying_type<EnumType>::type)a)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) \
    { return EnumType(((std::underlying_type<EnumType>::type)a) ^ ((std::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator ^= (EnumType &a, EnumType b) \
    { return a = a ^ b; } \
inline constexpr bool any(EnumType a) \
    { return ((std::underlying_type<EnumType>::type)a) != 0; } 

namespace Alimer
{
    template <typename T>
    void SafeDelete(T*& resource)
    {
        delete resource;
        resource = nullptr;
    }

    template <typename T>
    void SafeDeleteContainer(T& resource)
    {
        for (auto& element : resource)
        {
            SafeDelete(element);
        }
        resource.clear();
    }

    template <typename T>
    void SafeDeleteArray(T*& resource)
    {
        delete[] resource;
        resource = nullptr;
    }

    template <typename T>
    void SafeRelease(T& resource)
    {
        if (resource)
        {
            resource->Release();
            resource = nullptr;
        }
    }
}

#endif
