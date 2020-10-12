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
#include "Platform/Platform.h"
#include "Platform/Event.h"
#include "Platform/Application.h"
#include "IO/FileSystem.h"
#include "Graphics/SwapChain.h"
#include "Graphics/GraphicsDevice.h"
#include "UI/ImGuiLayer.h"
#include <vgpu.h>

namespace Alimer
{
    static void vgpu_log_callback(void* userData, vgpu_log_level level, const char* message)
    {
        switch (level)
        {
        case VGPU_LOG_LEVEL_INFO:
            LOGI(message);
            break;
        case VGPU_LOG_LEVEL_WARN:
            LOGW(message);
            break;
        case VGPU_LOG_LEVEL_ERROR:
            LOGE(message);
            break;
        default:
            break;
        }
    }

    static Application* s_appCurrent = nullptr;

    Application::Application(const Config& config)
        : config{ config }
        , state(State::Uninitialized)
        , assets(config.rootDirectory)
    {
        ALIMER_ASSERT_MSG(s_appCurrent == nullptr, "Cannot create more than one Application");

        vgpu_set_log_callback(vgpu_log_callback, this);
        vgpu_set_preferred_backend(VGPU_BACKEND_TYPE_VULKAN);

        s_appCurrent = this;
    }

    Application::~Application()
    {
        //ImGuiLayer::Shutdown();
        vgpu_shutdown();
        s_appCurrent = nullptr;
    }

    Application* Application::Current()
    {
        return s_appCurrent;
    }

    void Application::InitBeforeRun()
    {
        // Create main window.
        WindowFlags windowFlags = WindowFlags::None;
        if (config.resizable)
            windowFlags |= WindowFlags::Resizable;

        if (config.fullscreen)
            windowFlags |= WindowFlags::Fullscreen;

        window = std::make_unique<Window>(config.title, Window::Centered, Window::Centered, config.width, config.height, windowFlags);

        // Init vgpu
        vgpu_config gpu_config = {};
#ifdef _DEBUG
        gpu_config.debug = true;
#endif
        gpu_config.swapchain_info.window_handle = window->GetHandle();

        if (vgpu_init("Alimer", &gpu_config))
        {
            headless = true;
        }

        ImGuiLayer::Initialize();

        Initialize();
    }

    void Application::Run()
    {
        InitBeforeRun();

        state = State::Running;
        while (state == State::Running)
        {
            Event evt{};
            while (PollEvent(evt))
            {
                if (evt.type == EventType::Quit)
                {
                    state = State::Uninitialized;
                    break;
                }
            }

            Tick();
        }
    }

    void Application::Tick()
    {
        if (!vgpu_begin_frame())
            return;

        OnDraw();

        /*agpu_push_debug_group("Frame");

        context->BeginRenderPass(renderPass);
        context->EndRenderPass();
        agpu_bind_pipeline(render_pipeline);
        agpu_draw(3, 1, 0);
        agpu_pop_debug_group();*/

        vgpu_end_frame();
    }

    const Config* Application::GetConfig()
    {
        return &config;
    }
}
