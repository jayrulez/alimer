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

#include "Application/Application.h"
#include "Application/AppHost.h"
#include "Win32_Include.h"
#include <shellapi.h>
#include <objbase.h>
#include <DirectXMath.h>

namespace
{
    std::string WStringToString(const std::wstring& wstr)
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

}

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!DirectX::XMVerifyCPUSupport())
        return 1;

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

    int argc;
    wchar_t** wide_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<char*> argv_buffer(argc + 1);
    char** argv = nullptr;
    std::vector<std::string> argv_strings(argc);

    if (wide_argv)
    {
        argv = argv_buffer.data();
        for (int i = 0; i < argc; i++)
        {
            argv_strings[i] = WStringToString(wide_argv[i]);
            argv_buffer[i] = const_cast<char*>(argv_strings[i].c_str());
        }
    }

    //Alimer::Platform::set_arguments(argv_strings);

    int exitCode = 1;
    std::unique_ptr<Alimer::Application> app = std::unique_ptr<Alimer::Application>(Alimer::ApplicationCreate(argc, argv));

    if (app)
    {
        exitCode = app->Run();
        app.reset();
    }

    CoUninitialize();
    return exitCode;
}
