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

#include "core/Application.h"
#include "WindowsPlatform.h"
#include "WindowsPrivate.h"
#include <shellapi.h>
#include <objbase.h>
#include <stdio.h>

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace Alimer
{
    std::unique_ptr<Application> g_Application;

    const wchar_t WindowsPlatform::AppWindowClass[] = TEXT("AlimerWindow");
    HINSTANCE WindowsPlatform::hInstance;

    bool WindowsPlatform::Init(HINSTANCE hInstance_)
    {
        HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
        if (FAILED(hr))
            return false;

#ifdef _DEBUG
        if (AllocConsole()) {
            FILE* fp;
            freopen_s(&fp, "conin$", "r", stdin);
            freopen_s(&fp, "conout$", "w", stdout);
            freopen_s(&fp, "conout$", "w", stderr);
        }
#endif

        // Parse arguments
        LPWSTR* argv;
        int     argc;

        argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        // Ignore the first argument containing the application full path
        std::vector<std::wstring> arg_strings(argv + 1, argv + argc);
        std::vector<std::string>  args;

        for (auto& arg : arg_strings)
        {
            args.push_back(ToUtf8(arg));
        }

        Platform::SetArguments(args);

        hInstance = hInstance_;
        return true;
    }

    void WindowsPlatform::Shutdown()
    {
        CoUninitialize();
    }

    const char* WindowsPlatform::GetName()
    {
        return ALIMER_PLATFORM_NAME;
    }

    PlatformId WindowsPlatform::GetId()
    {
        return PlatformId::Windows;
    }

    PlatformFamily WindowsPlatform::GetFamily()
    {
        return PlatformFamily::Desktop;
    }

    HINSTANCE WindowsPlatform::GetHInstance()
    {
        return hInstance;
    }


    std::string ToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty())
        {
            return {};
        }

        auto wstr_len = static_cast<int>(wstr.size());
        auto str_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len, NULL, 0, NULL, NULL);

        std::string str(str_len, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len, &str[0], str_len, NULL, NULL);

        return str;
    }

    std::wstring ToUtf16(const std::string& str)
    {
        if (str.empty())
        {
            return {};
        }

        auto str_len = static_cast<int>(str.size());
        auto wstr_len = MultiByteToWideChar(CP_UTF8, 0, &str[0], str_len, NULL, 0);

        std::wstring wstr(wstr_len, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], str_len, &wstr[0], wstr_len);
        return wstr;
    }
}

using namespace Alimer;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!Platform::Init(hInstance))
        return 1;

    // Register class and create window
    {
        // Register class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszClassName = Platform::AppWindowClass;
        wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
        if (!RegisterClassExW(&wcex))
            return 1;

        g_Application = CreateApplication(Platform::GetArguments());

        // Main message loop
        MSG msg = {};
        while (WM_QUIT != msg.message)
        {
            if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else
            {
                g_Application->Tick();
            }
        }

        g_Application.reset();

        Platform::Shutdown();
        return 0;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}
