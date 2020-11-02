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

#include "Platform/Platform.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "Platform/Application.h"
#include "PlatformIncl.h"
#if ALIMER_PLATFORM_WINDOWS
#    include <shellapi.h>
#endif

namespace alimer::Platform
{
    static Vector<String> arguments;

    String GetName()
    {
        return ALIMER_PLATFORM_NAME;
    }

    PlatformId GetId()
    {
        return PlatformId::Windows;
    }

    PlatformFamily Platform::GetFamily()
    {
        return PlatformFamily::Desktop;
    }

    const Vector<String>& Platform::GetArguments()
    {
        return arguments;
    }

#if ALIMER_PLATFORM_WINDOWS
    const Vector<String>& ParseArguments(const wchar_t* cmdLine)
    {
        arguments.clear();

        // Parse command line
        LPWSTR* argv;
        int     argc;

        argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        // Ignore the first argument containing the application full path
        Vector<WString> arg_strings(argv + 1, argv + argc);
        Vector<String>  args;

        for (auto& arg : arg_strings)
        {
            args.push_back(alimer::ToUtf8(arg));
        }

        return arguments;
    }

    Platform::WindowsVersion Platform::GetWindowsVersion()
    {
        WindowsVersion version = WindowsVersion::Unknown;
        auto           RtlGetVersion =
            reinterpret_cast<LONG(WINAPI*)(LPOSVERSIONINFOEXW)>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
        ALIMER_VERIFY_MSG(RtlGetVersion, "Failed to get address to RtlGetVersion from ntdll.dll");

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
}
