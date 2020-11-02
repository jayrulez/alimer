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

#include "Core/Log.h"
#include "Platform/Application.h"
#include "Platform/Platform.h"

namespace
{
    std::unique_ptr<alimer::Application> g_application;
}

#if defined(__ANDROID__)
void android_main(android_app* state) {}
#elif ALIMER_PLATFORM_WINDOWS
#    include "PlatformIncl.h"
#    include <DirectXMath.h>
#    include <objbase.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    if (!DirectX::XMVerifyCPUSupport())
    {
        return EXIT_FAILURE;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (FAILED(hr))
        return EXIT_FAILURE;

#    ifdef _DEBUG
    if (AllocConsole())
    {
        FILE* fp;
        freopen_s(&fp, "conin$", "r", stdin);
        freopen_s(&fp, "conout$", "w", stdout);
        freopen_s(&fp, "conout$", "w", stderr);
    }
#    endif

    alimer::Platform::ParseArguments(GetCommandLineW());

    // Only error handle in release
#    ifndef DEBUG
    try
    {
#    endif

        g_application = std::unique_ptr<alimer::Application>(alimer::CreateApplication());

        if (g_application)
        {
            g_application->Run();
        }

        g_application.reset();

#    ifndef DEBUG
    }
    catch (const std::exception& e)
    {
        LOGE(e.what());
        return EXIT_FAILURE;
    }
#    endif

#    if ALIMER_PLATFORM_WINDOWS
    CoUninitialize();
#    endif

    return EXIT_SUCCESS;
}

#elif ALIMER_PLATFORM_UWP || ALIMER_PLATFORM_XBOXONE
#    include "platform/uwp/windows_platform.h"
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {}
#else
int main(int argc, char* argv[]) {}
#endif
