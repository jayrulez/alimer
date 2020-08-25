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
#include "Win32_Window.h"
#include "Win32_Include.h"

namespace Alimer
{
    class WindowAppHost final : public AppHost
    {
    public:
        WindowAppHost(Application* application);
        void Run() override;
        Window* GetWindow() const override;

    private:
        RefPtr<Win32_Window> window;
    };

    std::unique_ptr<AppHost> AppHost::CreateDefault(Application* application)
    {
        return std::make_unique<WindowAppHost>(application);
    }

    /* WindowApplicationPlatform */
    WindowAppHost::WindowAppHost(Application* application)
        : AppHost(application)
    {
        auto& config = application->GetConfig();
        window = new Win32_Window(config.windowTitle, 0, 0, config.windowSize.width, config.windowSize.height);
    }

    void WindowAppHost::Run()
    {
        InitBeforeRun();
        window->Show();

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
                application->Tick();
            }
        }

        window.Reset();
    }

    Window* WindowAppHost::GetWindow() const
    {
        return window.Get();
    }
}
