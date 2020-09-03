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

#include "core/log.h"
#include "platform/application.h"
#include "platform/platform.h"

namespace Alimer
{
    static Application* s_appCurrent = nullptr;

    Application::Application(const Config& config_)
        : config(config_)
        , state(State::Uninitialized)
    {
        ALIMER_ASSERT_MSG(s_appCurrent == nullptr, "Cannot create more than one Application");

        // Figure out the graphics backend API.
        if (config.rendererType  == RendererType::Count)
        {
            //config.rendererType = gpu::getPlatformBackend();
        }

        platform = Platform::Create(this);

        s_appCurrent = this;
    }

    Application::~Application()
    {
        graphics->WaitForGPU();
        graphics.Reset();
        s_appCurrent = nullptr;
        platform.reset();
    }

    Application* Application::Current()
    {
        return s_appCurrent;
    }

    void Application::InitBeforeRun()
    {
        GraphicsDeviceDescription deviceDesc = {};
#ifdef _DEBUG
        deviceDesc.flags |= GraphicsDeviceFlags::DebugRuntime;
#endif
        graphics = GraphicsDevice::Create(RendererType::Count, deviceDesc);
    }

    void Application::Run()
    {
        /*gpu::gpu_config gpu_config{};
#ifdef _DEBUG
        gpu_config.debug = true;
#endif
        gpu_config.device_preference = config.powerPreference;
        gpu::init(config.graphicsBackend , &gpu_config);
        */

        platform->Run();
    }

    void Application::Tick()
    {
        graphics->Frame();
    }

    const Config* Application::GetConfig()
    {
        return &config;
    }

    Window& Application::GetMainWindow() const
    {
        return platform->GetMainWindow();
    }
}
