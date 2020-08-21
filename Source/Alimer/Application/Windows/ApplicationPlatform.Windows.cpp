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

#include "Application/ApplicationPlatform.h"
#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>
#include <objbase.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

namespace Alimer
{
    class WindowApplicationPlatform final : public ApplicationPlatform
    {
    public:
        WindowApplicationPlatform(Application* application);
        ~WindowApplicationPlatform() override;
    };

    std::unique_ptr<ApplicationPlatform> ApplicationPlatform::CreateDefault(Application* application)
    {
        return std::make_unique<WindowApplicationPlatform>(application);
    }

    /* WindowApplicationPlatform */
    WindowApplicationPlatform::WindowApplicationPlatform(Application* application)
        : ApplicationPlatform(application)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (hr == RPC_E_CHANGED_MODE) {
            hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        }

        if (AllocConsole()) {
            FILE* fp;
            freopen_s(&fp, "conin$", "r", stdin);
            freopen_s(&fp, "conout$", "w", stdout);
            freopen_s(&fp, "conout$", "w", stderr);
        }
    }

    WindowApplicationPlatform::~WindowApplicationPlatform()
    {
        CoUninitialize();
    }
}
