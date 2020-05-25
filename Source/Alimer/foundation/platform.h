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

#if !defined(ALIMER_COMPILE)
#   define ALIMER_COMPILE 0
#endif

// Compilers
#define ALIMER_COMPILER_CLANG 0
#define ALIMER_COMPILER_CLANG_ANALYZER 0
#define ALIMER_COMPILER_CLANG_CL 0
#define ALIMER_COMPILER_GCC 0
#define ALIMER_COMPILER_MSVC 0

// Platform traits and groups
#define ALIMER_PLATFORM_APPLE 0
#define ALIMER_PLATFORM_POSIX 0

#define ALIMER_PLATFORM_FAMILY_MOBILE 0
#define ALIMER_PLATFORM_FAMILY_DESKTOP 0
#define ALIMER_PLATFORM_FAMILY_CONSOLE 0

// Platforms
#define ALIMER_PLATFORM_ANDROID 0
#define ALIMER_PLATFORM_LINUX 0
#define ALIMER_PLATFORM_IOS 0
#define ALIMER_PLATFORM_TVOS 0
#define ALIMER_PLATFORM_MACOS 0
#define ALIMER_PLATFORM_WINDOWS 0
#define ALIMER_PLATFORM_UWP 0
#define ALIMER_PLATFORM_XBOXONE 0
#define ALIMER_PLATFORM_EMSCRIPTEN 0

// CPU
#define ALIMER_ARCH_X64 0
#define ALIMER_ARCH_X86 0
#define ALIMER_ARCH_A64 0
#define ALIMER_ARCH_ARM 0
#define ALIMER_ARCH_PPC 0

// Architecture
#define ALIMER_ARCH_32BIT 0
#define ALIMER_ARCH_64BIT 0

// Architecture intrinsics
#define ALIMER_AVX2_INTRINSICS 0
#define ALIMER_FMA3_INTRINSICS 0
#define ALIMER_F16C_INTRINSICS 0
#define ALIMER_AVX_INTRINSICS 0
#define ALIMER_SSE4_INTRINSICS 0
#define ALIMER_SSE3_INTRINSICS 0
#define ALIMER_SSE_INTRINSICS 0
#define ALIMER_NEON_INTRINSICS 0

#define ALIMER_STRINGIZE_HELPER(X) #X
#define ALIMER_STRINGIZE(X) ALIMER_STRINGIZE_HELPER(X)

#define ALIMER_CONCAT_HELPER(X, Y) X##Y
#define ALIMER_CONCAT(X, Y) ALIMER_CONCAT_HELPER(X, Y)

/* Detect compiler first */
#if defined(__clang__)
#   undef ALIMER_COMPILER_CLANG
#   define ALIMER_COMPILER_CLANG 1
#   define ALIMER_COMPILER_NAME "clang"
#   define ALIMER_COMPILER_DESCRIPTION ALIMER_COMPILER_NAME " " ALIMER_STRINGIZE(__clang_major__) "." ALIMER_STRINGIZE(__clang_minor__)
#   define ALIMER_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#   if defined(__clang_analyzer__)
#        undef ALIMER_COMPILER_CLANG_ANALYZER
#        define ALIMER_COMPILER_CLANG_ANALYZER 1
#   endif    // defined(__clang_analyzer__)
#   if defined(_MSC_VER)
#       undef ALIMER_COMPILER_MSVC
#       define ALIMER_COMPILER_MSVC 1
#       undef ALIMER_COMPILER_CLANG_CL
#       define ALIMER_COMPILER_CLANG_CL 1
#   endif
#elif defined(_MSC_VER)
#   undef ALIMER_COMPILER_MSVC
#   define ALIMER_COMPILER_MSVC 1
#   define ALIMER_COMPILER_NAME "msvc"
#   define ALIMER_COMPILER_DESCRIPTION ALIMER_COMPILER_NAME " " ALIMER_STRINGIZE(_MSC_VER)
#elif defined(__GNUC__)
#   undef ALIMER_COMPILER_GCC
#   define ALIMER_COMPILER_GCC 1
#   define ALIMER_COMPILER_NAME "gcc"
#   define ALIMER_COMPILER_DESCRIPTION ALIMER_COMPILER_NAME " " ALIMER_STRINGIZE(__GNUC__) "." ALIMER_STRINGIZE(__GNUC_MINOR__)
#   define ALIMER_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#   error "Unknown compiler"
#endif

/* Detect platform*/
#if defined(_DURANGO) || defined(_XBOX_ONE)
#   undef ALIMER_PLATFORM_FAMILY_CONSOLE
#   define ALIMER_PLATFORM_FAMILY_CONSOLE 1
#   undef ALIMER_PLATFORM_XBOXONE
#   define ALIMER_PLATFORM_XBOXONE 1
#   define ALIMER_PLATFORM_NAME "XboxOne"
#elif defined(_WIN32) || defined(_WIN64)
#   include <winapifamily.h>
#   if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#       undef ALIMER_PLATFORM_FAMILY_DESKTOP
#       define ALIMER_PLATFORM_FAMILY_DESKTOP 1
#       undef ALIMER_PLATFORM_WINDOWS
#       define ALIMER_PLATFORM_WINDOWS 1
#       define ALIMER_PLATFORM_NAME "Windows"
#   else
#       undef ALIMER_PLATFORM_FAMILY_CONSOLE
#       define ALIMER_PLATFORM_FAMILY_CONSOLE 1
#       undef ALIMER_PLATFORM_UWP
#       define ALIMER_PLATFORM_UWP 1
#       define ALIMER_PLATFORM_NAME "UWP"
#   endif
#elif defined(__ANDROID__)
#   undef ALIMER_PLATFORM_POSIX
#   define ALIMER_PLATFORM_POSIX 1
#   undef ALIMER_PLATFORM_FAMILY_MOBILE
#   define ALIMER_PLATFORM_FAMILY_MOBILE 1
#   undef ALIMER_PLATFORM_ANDROID
#   define ALIMER_PLATFORM_ANDROID  1
#   define ALIMER_PLATFORM_NAME "Android"
#elif defined(__EMSCRIPTEN__)
#   undef ALIMER_PLATFORM_POSIX
#   define ALIMER_PLATFORM_POSIX 1
#   undef ALIMER_PLATFORM_LINUX
#   define ALIMER_PLATFORM_LINUX 1
#   undef ALIMER_PLATFORM_EMSCRIPTEN
#   define ALIMER_PLATFORM_EMSCRIPTEN 1
#   define ALIMER_PLATFORM_NAME "Web"
#elif defined(__linux__) // note: __ANDROID__/__EMSCRIPTEN__ implies __linux__
#   undef ALIMER_PLATFORM_POSIX
#   define ALIMER_PLATFORM_POSIX 1
#   undef ALIMER_PLATFORM_LINUX
#   define ALIMER_PLATFORM_LINUX 1
#   define ALIMER_PLATFORM_NAME "Linux"
#elif defined(__APPLE__) // macOS, iOS, tvOS
#   include <TargetConditionals.h>
#   undef ALIMER_PLATFORM_APPLE
#   define ALIMER_PLATFORM_APPLE 1
#   undef ALIMER_PLATFORM_POSIX
#   define ALIMER_PLATFORM_POSIX 1
#   if TARGET_OS_WATCH // watchOS
#       error "Apple Watch is not supported"
#   elif TARGET_OS_IOS // iOS
#       undef ALIMER_PLATFORM_FAMILY_MOBILE
#       define ALIMER_PLATFORM_FAMILY_MOBILE 1
#       undef ALIMER_PLATFORM_IOS
#       define ALIMER_PLATFORM_IOS 1
#       define ALIMER_PLATFORM_NAME "iOS"
#   elif TARGET_OS_TV // tvOS
#       undef ALIMER_PLATFORM_FAMILY_CONSOLE
#       define ALIMER_PLATFORM_FAMILY_CONSOLE 1
#       undef ALIMER_PLATFORM_TVOS
#       define ALIMER_PLATFORM_TVOS 1
#       define ALIMER_PLATFORM_NAME "tvOS"
#   elif TARGET_OS_MAC
#       undef ALIMER_PLATFORM_FAMILY_DESKTOP
#       define ALIMER_PLATFORM_FAMILY_DESKTOP 1
#       undef ALIMER_PLATFORM_MACOS
#       define ALIMER_PLATFORM_MACOS 1
#       define ALIMER_PLATFORM_NAME "macOS"
#   endif
#else
#error "Unknown operating system"
#endif

/**
Architecture defines, see http://sourceforge.net/apps/mediawiki/predef/index.php?title=Architectures
*/
#if defined(__x86_64__) || defined(_M_X64)
#   undef ALIMER_ARCH_X64
#   define ALIMER_ARCH_X64 1
#elif defined(__i386__) || defined(_M_IX86) || defined (__EMSCRIPTEN__)
#   undef ALIMER_ARCH_X86
#   define ALIMER_ARCH_X86 1
#elif defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
#   undef ALIMER_ARCH_A64
#   define ALIMER_ARCH_A64 1
#elif defined(__arm__) || defined(_M_ARM)
#   undef ALIMER_ARCH_ARM
#   define ALIMER_ARCH_ARM 1
#elif defined(__ppc__) || defined(_M_PPC) || defined(__CELLOS_LV2__)
#   undef ALIMER_ARCH_PPC
#   define ALIMER_ARCH_PPC 1
#else
#   error "Unknown architecture"
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(__64BIT__) || \
    defined(__mips64) || defined(__powerpc64__) || defined(__ppc64__) || defined(__LP64__)
#   undef ALIMER_ARCH_64BIT
#   define ALIMER_ARCH_64BIT 1
#else
#   undef ALIMER_ARCH_32BIT
#   define ALIMER_ARCH_32BIT 1
#endif

/**SIMD defines
*/
#if !defined(ALIMER_SIMD_DISABLED)
#if defined(__AVX2__)
#   undef ALIMER_AVX2_INTRINSICS
#   define ALIMER_AVX2_INTRINSICS 1
#endif

#if ALIMER_AVX2_INTRINSICS
#   undef ALIMER_FMA3_INTRINSICS 0
#   define ALIMER_FMA3_INTRINSICS
#endif


#if ALIMER_AVX2_INTRINSICS_
#   undef ALIMER_F16C_INTRINSICS
#   define ALIMER_F16C_INTRINSICS 1
#endif

#if !ALIMER_F16C_INTRINSICS && defined(__F16C__)
#   undef ALIMER_F16C_INTRINSICS
#   define ALIMER_F16C_INTRINSICS 1
#endif

#if ALIMER_FMA3_INTRINSICS && !ALIMER_AVX_INTRINSICS
#   undef ALIMER_AVX_INTRINSICS
#   define ALIMER_AVX_INTRINSICS 1
#endif

#if ALIMER_F16C_INTRINSICS && !ALIMER_AVX_INTRINSICS
#   undef ALIMER_AVX_INTRINSICS
#   define ALIMER_AVX_INTRINSICS 1
#endif

#if !ALIMER_AVX_INTRINSICS && defined(__AVX__)
#   undef ALIMER_AVX_INTRINSICS
#   define ALIMER_AVX_INTRINSICS 1
#endif

#if ALIMER_AVX_INTRINSICS && !ALIMER_SSE4_INTRINSICS
#   undef ALIMER_SSE4_INTRINSICS
#   define ALIMER_SSE4_INTRINSICS 1
#endif

#if ALIMER_SSE4_INTRINSICS && !ALIMER_SSE3_INTRINSICS
#   undef ALIMER_SSE3_INTRINSICS
#   define ALIMER_SSE3_INTRINSICS 1
#endif

#if ALIMER_SSE3_INTRINSICS && !ALIMER_SSE_INTRINSICS
#   undef ALIMER_SSE_INTRINSICS
#   define ALIMER_SSE_INTRINSICS 1
#endif

#if !ALIMER_NEON_INTRINSICS && !ALIMER_SSE_INTRINSICS
#   if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__) && !defined(_M_HYBRID_X86_ARM64)
#       undef ALIMER_SSE_INTRINSICS
#       define ALIMER_SSE_INTRINSICS 1
#   elif defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || __arm__ || __aarch64__
#       undef ALIMER_NEON_INTRINSICS
#       define ALIMER_NEON_INTRINSICS 1
#   endif
#endif /* !ALIMER_NEON_INTRINSICS && !ALIMER_SSE_INTRINSICS */

#endif /* !defined(ALIMER_SIMD_DISABLED) */

/* Compiler defines */
#if defined(__clang__)
#   define ALIMER_RESTRICT __restrict
#   define ALIMER_THREADLOCAL _Thread_local
#   define ALIMER_DEPRECATED __attribute__(deprecated)
#   define ALIMER_FORCEINLINE inline __attribute__(always_inline)
#   define ALIMER_NOINLINE __attribute__(noinline)
#   define ALIMER_PURECALL __attribute__(pure)
#   define ALIMER_CONSTCALL __attribute__(const)
#   define ALIMER_LIKELY(x) __builtin_expect(!!(x), 1)
#   define ALIMER_UNLIKELY(x) __builtin_expect(!!(x), 0)
#   define ALIMER_BREAKPOINT() __builtin_trap();
#   define ALIMER_UNREACHABLE() __builtin_unreachable()
#elif defined(__GNUC__)
#   define ALIMER_RESTRICT __restrict
#   define ALIMER_THREADLOCAL __thread
#   define ALIMER_DEPRECATED __attribute__(deprecated)
#   define ALIMER_FORCEINLINE inline __attribute__(always_inline)
#   define ALIMER_NOINLINE __attribute__(noinline)
#   define ALIMER_PURECALL __attribute__(pure)
#   define ALIMER_CONSTCALL __attribute__(const)
#   define ALIMER_LIKELY(x) __builtin_expect(!!(x), 1)
#   define ALIMER_UNLIKELY(x) __builtin_expect(!!(x), 0)
#   define ALIMER_BREAKPOINT() __builtin_trap();
#   define ALIMER_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#   define ALIMER_RESTRICT __restrict
#   define ALIMER_THREADLOCAL __declspec(thread)
#   define ALIMER_DEPRECATED __declspec(deprecated)
#   define ALIMER_FORCEINLINE __forceinline
#   define ALIMER_NOINLINE __declspec(noinline)
#   define ALIMER_PURECALL __declspec(noalias)
#   define ALIMER_CONSTCALL __declspec(noalias)
#   define ALIMER_LIKELY(x) (x)
#   define ALIMER_UNLIKELY(x) (x)
#   define ALIMER_BREAKPOINT() __debugbreak();
#   define ALIMER_UNREACHABLE() __assume(false)
#else
#define ALIMER_RESTRICT
#define ALIMER_THREADLOCAL
#endif

#ifndef ALIMER_ALIGN
#   if defined(_MSC_VER)
#       define ALIMER_ALIGN(alignment, decl) __declspec(align(alignment)) decl
#       define ALIMER_ALIGN_PREFIX(alignment) __declspec(align(alignment))
#       define ALIMER_ALIGN_SUFFIX(alignment)
#       define ALIMER_ALIGNOF(type) __alignof(type)
#       define ALIMER_OFFSET_OF(X, Y) offsetof(X, Y)
#   elif (defined(__clang__) || defined(__GNUC__))
#       define ALIMER_ALIGN(alignment, decl) decl __attribute__((aligned(alignment)))
#       define ALIMER_ALIGN_PREFIX(alignment)
#       define ALIMER_ALIGN_SUFFIX(alignment) __attribute__((aligned(alignment)))
#       define ALIMER_ALIGNOF(type) __alignof__(type)
#       define ALIMER_OFFSET_OF(X, Y) __builtin_offsetof(X, Y)
#   endif
#endif

// Use for getting the amount of members of a standard C array.
#define ALIMER_COUNT_OF(x) (sizeof(x)/sizeof(x[0]))
#define ALIMER_UNUSED(x) do { (void)sizeof(x); } while(0)

#ifndef ALIMER_DEBUG
#   ifdef _DEBUG
#       define ALIMER_DEBUG 1
#   else
#       define ALIMER_DEBUG 0
#   endif
#endif

/** Assert macro */
#ifndef ALIMER_ENABLE_ASSERT
#   if ALIMER_DEBUG
#       define ALIMER_ENABLE_ASSERT 1
#   else
#       define ALIMER_ENABLE_ASSERT 0
#   endif
#endif

/** DLL export macros */
#ifndef ALIMER_C_EXPORT
#   if ALIMER_PLATFORM_WINDOWS || ALIMER_PLATFORM_UWP || ALIMER_PLATFORM_LINUX
#       define ALIMER_C_EXPORT extern "C"
#   else
#       define ALIMER_C_EXPORT
#   endif
#endif

#if ALIMER_PLATFORM_POSIX && __GNUC__ >= 4
#   define ALIMER_UNIX_EXPORT __attribute__((visibility("default")))
#else
#   define ALIMER_UNIX_EXPORT
#endif

#if (ALIMER_PLATFORM_WINDOWS || ALIMER_PLATFORM_UWP || ALIMER_PLATFORM_XBOXONE)
#   define ALIMER_DLL_EXPORT __declspec(dllexport)
#   define ALIMER_DLL_IMPORT __declspec(dllimport)
#else
#   define ALIMER_DLL_EXPORT ALIMER_UNIX_EXPORT
#   define ALIMER_DLL_IMPORT
#endif

#if defined(ALIMER_SHARED_LIBRARY)
#   if defined(ALIMER_COMPILE)
#       define ALIMER_API ALIMER_DLL_EXPORT
#   else
#       define ALIMER_API ALIMER_DLL_IMPORT
#   endif
#else
#   define ALIMER_API
#endif  // defined(ALIMER_SHARED_LIBRARY)

// Base data types
#include <stddef.h>
#include <stdint.h>

#define FLOAT32_C(x) (x##f)
#define FLOAT64_C(x) (x)

#ifndef ALIMER_SIZE_REAL
#   define ALIMER_SIZE_REAL 4
#endif

#if ALIMER_SIZE_REAL == 8
typedef double real;
#define REAL_C(x) FLOAT64_C(x)
#else
typedef float real;
#define REAL_C(x) FLOAT32_C(x)
#endif

// Format specifiers for 64bit and pointers
#if ALIMER_COMPILER_MSVC
#   define PRId32 "Id"
#   define PRIi32 "Ii"
#   define PRIo32 "Io"
#   define PRIu32 "Iu"
#   define PRIx32 "Ix"
#   define PRIX32 "IX"
#   define PRId64 "I64d"
#   define PRIi64 "I64i"
#   define PRIo64 "I64o"
#   define PRIu64 "I64u"
#   define PRIx64 "I64x"
#   define PRIX64 "I64X"
#   define PRIdPTR "Id"
#   define PRIiPTR "Ii"
#   define PRIoPTR "Io"
#   define PRIuPTR "Iu"
#   define PRIxPTR "Ix"
#   define PRIXPTR "IX"
#   define PRIsize "Iu"
#else
#   include <inttypes.h>
#   define PRIsize "zu"
#endif

#define PRItick PRIi64
#define PRIhash PRIx64

#if ALIMER_SIZE_REAL == 8
#   define PRIreal "lf"
#else
#   define PRIreal "f"
#endif

#if ALIMER_COMPILER_MSVC
#   if ALIMER_SIZE_POINTER == 8
#       define PRIfixPTR "016I64X"
#   else
#       define PRIfixPTR "08IX"
#   endif
#else
#   if ALIMER_SIZE_POINTER == 8
#       define PRIfixPTR "016" PRIXPTR
#   else
#       define PRIfixPTR "08" PRIXPTR
#   endif
#endif
