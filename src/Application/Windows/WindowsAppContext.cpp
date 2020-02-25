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

#include "PlatformWindows.h"
#include "WindowsAppContext.h"
#include "WindowsWindow.h"
#include "Application/Application.h"
#include "Core/Platform.h"

#include <objbase.h>
#include <shellapi.h>
//#include <shellscalingapi.h>

#ifndef DPI_ENUMS_DECLARED
typedef enum PROCESS_DPI_AWARENESS {
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
#endif

namespace Alimer
{
    WindowsAppContext::WindowsAppContext(Application* app)
        : AppContext(app, true)
    {
        LPWSTR* argv;
        int     argc;

        argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        // Ignore the first argument containing the application full path
        eastl::vector<eastl::wstring> arg_strings(argv + 1, argv + argc);
        eastl::vector<eastl::string>  args;

        for (auto& arg : arg_strings)
        {
            args.push_back(ToUtf8(arg));
        }

        SetArguments(args);

        // Initialize COM
        if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) == RPC_E_CHANGED_MODE) {
            CoInitializeEx(NULL, COINIT_MULTITHREADED);
        }

        if (AllocConsole())
        {
            FILE* fp;
            freopen_s(&fp, "conin$", "r", stdin);
            freopen_s(&fp, "conout$", "w", stdout);
            freopen_s(&fp, "conout$", "w", stderr);
        }

        HMODULE user32Dll = LoadLibraryW(L"user32.dll");

        typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
        PFN_SetProcessDpiAwarenessContext SetProcessDpiAwarenessContextFunc = reinterpret_cast<PFN_SetProcessDpiAwarenessContext>(GetProcAddress(user32Dll, "SetProcessDpiAwarenessContext"));
        if (SetProcessDpiAwarenessContextFunc)
        {
            SetProcessDpiAwarenessContextFunc(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
        else
        {
            HMODULE shcoreDll = LoadLibraryW(L"shcore.dll");
            if (shcoreDll)
            {
                typedef HRESULT(WINAPI* SetProcessDpiAwarenessFuncType)(PROCESS_DPI_AWARENESS);
                SetProcessDpiAwarenessFuncType SetProcessDpiAwarenessFunc = reinterpret_cast<SetProcessDpiAwarenessFuncType>(GetProcAddress(shcoreDll, "SetProcessDpiAwareness"));

                if (SetProcessDpiAwarenessFunc)
                    SetProcessDpiAwarenessFunc(PROCESS_PER_MONITOR_DPI_AWARE);

                FreeLibrary(shcoreDll);
            }
            else
            {
                SetProcessDPIAware();
            }
        }
    }

    WindowsAppContext::~WindowsAppContext()
    {
        CoUninitialize();
    }

    void WindowsAppContext::Run()
    {
        uint32_t width, height;
        _app->GetDefaultWindowSize(&width, &height);
        _mainWindow.reset(new WindowsWindow("Alimer", width, height, WindowStyle::Default));

        Initialize();

        // Main message loop
        MSG msg = {};
        while (WM_QUIT != msg.message)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                _app->Tick();
            }
        }
    }

    AppContext* AppContext::CreateDefault(Application* app)
    {
        return new WindowsAppContext(app);
    }
}
