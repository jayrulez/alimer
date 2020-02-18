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

#include "Platform.h"

#if defined(_WIN32)
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
#endif

#if TARGET_OS_MAC || defined(__linux__)
    #include <unistd.h>
#endif

namespace alimer
{
    namespace platform
    {
        std::string GetName()
        {
#if defined(_XBOX_ONE)
            return "XboxOne";
#elif defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_APP
            return "UWP";
#elif defined(_WIN64) || defined(_WIN32)
            return "Windows";
#elif defined(__ANDROID__)
            return "Android";
#elif defined (__EMSCRIPTEN__)
            return "Web";
#elif defined(__linux__)
            return "Linux";
#elif TARGET_OS_IOS 
            return "iOS";
#elif TARGET_OS_TV
            return "tvOS";
#elif TARGET_OS_MAC 
            return "macOS";
#else
            return "(?)";
#endif
        }

        PlatformID GetId()
        {
#if defined(_XBOX_ONE)
            return PlatformID::XboxOne;
#elif defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_APP
            return PlatformID::UWP;
#elif defined(_WIN64) || defined(_WIN32)
            return PlatformID::Windows;
#elif defined(__ANDROID__)
            return PlatformID::Android;
#elif defined (__EMSCRIPTEN__)
            return PlatformID::Web;
#elif defined(__linux__)
            return PlatformID::Linux;
#elif TARGET_OS_IOS 
            return PlatformID::iOS;
#elif TARGET_OS_TV
            return PlatformID::tvOS;
#elif TARGET_OS_MAC 
            return PlatformID::macOS;
#else
            return PlatformID::Unknown;
#endif
        }

        PlatformFamily GetFamily()
        {
#if defined(_XBOX_ONE)
            return PlatformFamily::Console;
#elif defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_APP
            return PlatformFamily::Console;
#elif defined(_WIN64) || defined(_WIN32)
            return PlatformFamily::Desktop;
#elif defined(__ANDROID__)
            return PlatformFamily::Mobile;
#elif defined (__EMSCRIPTEN__)
            return PlatformFamily::Mobile;
#elif defined(__linux__)
            return PlatformFamily::Desktop;
#elif TARGET_OS_IOS 
            return PlatformFamily::Mobile;
#elif TARGET_OS_TV
            return PlatformFamily::Mobile;
#elif TARGET_OS_MAC 
            return PlatformFamily::Desktop;
#else
            return PlatformFamily::Unknown;
#endif
        }

        ProcessId GetCurrentProcessId()
        {
#if defined(_WIN64) || defined(_WIN32)
            return static_cast<ProcessId>(::GetCurrentProcessId());
#else
            return getpid();
#endif
        }
    }
}
