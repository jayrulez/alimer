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

#include "core/Platform.h"
#include "core/Assert.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if TARGET_OS_MAC || defined(__linux__)
#include <unistd.h>
#endif

using namespace std;

namespace alimer
{
    string Platform::GetName()
    {
        return ALIMER_PLATFORM_NAME;
    }

    PlatformId Platform::GetId()
    {
#if defined(_XBOX_ONE)
        return PlatformId::XboxOne;
#elif defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_APP
        return PlatformId::UWP;
#elif defined(_WIN64) || defined(_WIN32)
        return PlatformId::Windows;
#elif defined(__ANDROID__)
        return PlatformId::Android;
#elif defined (__EMSCRIPTEN__)
        return PlatformId::Web;
#elif defined(__linux__)
        return PlatformId::Linux;
#elif TARGET_OS_IOS 
        return PlatformId::iOS;
#elif TARGET_OS_TV
        return PlatformId::tvOS;
#elif TARGET_OS_MAC 
        return PlatformId::macOS;
#else
        return PlatformId::Unknown;
#endif
    }

    PlatformFamily Platform::GetFamily()
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

#if defined(_WIN64) || defined(_WIN32) || defined(WINAPI_FAMILY)
    WindowsVersion Platform::GetWindowsVersion()
    {
        WindowsVersion version = WindowsVersion::Unknown;
        auto RtlGetVersion = reinterpret_cast<LONG(WINAPI*)(LPOSVERSIONINFOEXW)>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
        ALIMER_ASSERT_MSG(RtlGetVersion, "Failed to get address to RtlGetVersion from ntdll.dll");

        RTL_OSVERSIONINFOEXW osinfo;
        osinfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
        if (RtlGetVersion(&osinfo) == 0)
        {
            if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
            {
                if (osinfo.dwMajorVersion == 6)
                {
                    if (osinfo.dwMinorVersion == 1)
                    {
                        version = WindowsVersion::Win7;
                    }
                    else if (osinfo.dwMinorVersion == 2)
                    {
                        version = WindowsVersion::Win8;
                    }
                    else if (osinfo.dwMinorVersion == 3)
                    {
                        version = WindowsVersion::Win81;
                    }
                }
                else if (osinfo.dwMajorVersion == 10)
                {
                    version = WindowsVersion::Win10;
                }
            }
        }

        return version;
    }
#endif

    ProcessId Platform::GetCurrentProcessId()
    {
#if defined(_WIN64) || defined(_WIN32)
        return static_cast<ProcessId>(::GetCurrentProcessId());
#else
        return getpid();
#endif
    }

    void Platform::OpenConsole()
    {
#if defined(_WIN32)
        if (AllocConsole()) {
            FILE* fp;
            freopen_s(&fp, "conin$", "r", stdin);
            freopen_s(&fp, "conout$", "w", stdout);
            freopen_s(&fp, "conout$", "w", stderr);
        }
#endif
    }
}
