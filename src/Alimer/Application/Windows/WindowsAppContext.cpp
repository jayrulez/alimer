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

#include "WindowsAppContext.h"
#include "Application/Application.h"
#include "Core/Platform.h"

#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
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
    static inline eastl::string ToUtf8(const eastl::wstring& wstr)
    {
        if (wstr.empty())
        {
            return {};
        }

        auto input_str_length = static_cast<int>(wstr.size());
        auto str_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], input_str_length, nullptr, 0, nullptr, nullptr);

        eastl::string str(str_len, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], input_str_length, &str[0], str_len, nullptr, nullptr);

        return str;
    }

    WindowsAppContext::WindowsAppContext(Application* app)
        : GLFW_AppContext(app)
    {
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
    }

    WindowsAppContext::~WindowsAppContext()
    {
        CoUninitialize();
    }

    AppContext* AppContext::CreateDefault(Application* app)
    {
        return new WindowsAppContext(app);
    }
}
