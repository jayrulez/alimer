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

#include "core/String.h"
#include "core/DeviceInfo.h"
#include "windows_platform.h"
#include <shellapi.h>
#include <objbase.h>

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace Alimer
{
    WindowsPlatform::WindowsPlatform(HINSTANCE hInstance)
        : hInstance{ hInstance }
    {
        HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
        if (FAILED(hr))
            return;

#ifdef _DEBUG
        if (AllocConsole()) {
            FILE* fp;
            freopen_s(&fp, "conin$", "r", stdin);
            freopen_s(&fp, "conout$", "w", stdout);
            freopen_s(&fp, "conout$", "w", stderr);
        }
#endif

        // Parse command line
        LPWSTR* argv;
        int     argc;

        argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        // Ignore the first argument containing the application full path
        std::vector<std::wstring> arg_strings(argv + 1, argv + argc);
        std::vector<std::string>  args;

        for (auto& arg : arg_strings)
        {
            args.push_back(Alimer::ToUtf8(arg));
        }

        Platform::set_arguments(args);
    }

    WindowsPlatform::~WindowsPlatform()
    {
        CoUninitialize();
    }

    const char* DeviceInfo::GetName()
    {
        return "Windows";
    }

    PlatformId DeviceInfo::GetId()
    {
        return PlatformId::Windows;
    }

    PlatformFamily DeviceInfo::GetFamily()
    {
        return PlatformFamily::Desktop;
    }

    WindowsVersion DeviceInfo::GetWindowsVersion()
    {
        WindowsVersion version = WindowsVersion::Unknown;
        auto RtlGetVersion = reinterpret_cast<LONG(WINAPI*)(LPOSVERSIONINFOEXW)>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
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

    ProcessId GetCurrentProcessId()
    {
        return static_cast<ProcessId>(::GetCurrentProcessId());
    }
}
