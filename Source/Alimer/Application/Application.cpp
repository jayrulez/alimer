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

#include "os/os.h"
#include "Application/Application.h"
#include "graphics/GraphicsDevice.h"
#include "Input/InputManager.h"
#include "engine/Log.h"

namespace Alimer
{
    static void GPULogCallback(void* userData, GPULogLevel level, const char* message)
    {
        switch (level)
        {
        case GPULogLevel_Error:
            Log::GetDefault()->Log(LogLevel::Error, message);
            break;
        case GPULogLevel_Warn:
            Log::GetDefault()->Log(LogLevel::Warning, message);
            break;
        case GPULogLevel_Info:
            Log::GetDefault()->Log(LogLevel::Info, message);
            break;
        case GPULogLevel_Debug:
            Log::GetDefault()->Log(LogLevel::Debug, message);
            break;
        default:
            break;
        }
    }

    Application::Application(const Configuration& config_)
        : config(config_)
        , input(new InputManager())
    {
        /* Init OS first */
        os_init();

        engine = Engine::create(allocator);

        gameSystems.Push(input);

        //gpuSetLogCallback(GPULogCallback, this);
    }

    Application::~Application()
    {
        for (auto gameSystem : gameSystems)
        {
            SafeDelete(gameSystem);
        }

        gameSystems.Clear();
        window_destroy(main_window);
        //gpuDeviceDestroy(gpuDevice);
        os_shutdown();
        Engine::destroy(engine);
    }

    void Application::InitBeforeRun()
    {
        if (!engine->Initialize()) {
            return;
        }

        // Create main window.
        if (!headless)
        {
            main_window = window_create(
                config.windowTitle.c_str(),
                config.windowSize.width, config.windowSize.height,
                WINDOW_FLAG_RESIZABLE | WINDOW_FLAG_OPENGL);
            window_set_centered(main_window);

            /*GPUSwapChainDescriptor swapChainDescriptor = {};
            swapChainDescriptor.windowHandle = window_handle(main_window);
            swapChainDescriptor.width = window_width(main_window);
            swapChainDescriptor.height = window_height(main_window);

            bool debug = false;
#ifdef _DEBUG
            debug = true;
#endif
            gpuDevice = gpuDeviceCreate(GPUBackendType_Direct3D11, debug, &swapChainDescriptor);

            // Create texture. 
            gpu_texture_info texture_info = {};
            texture_info.format = GPU_TEXTURE_FORMAT_RGBA8;
            texture_info.usage = GPU_TEXTURE_USAGE_RENDER_TARGET;
            texture_info.width = window_width(main_window);
            texture_info.height = window_height(main_window);
            texture_info.depth = 1u;
            texture_info.mip_levels = 1u;
            texture_info.array_layers = 1;
            auto texture = gpu_texture_create(&texture_info);
            
            // Create swapchain. 
            //swapchain = gpu_swapchain_create(&swapchain_info);
            */
        }

        Initialize();
        if (exitCode)
        {
            //Stop();
            return;
        }

        time.ResetElapsedTime();
        BeginRun();
    }

    void Application::Initialize()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Initialize();
        }
    }

    void Application::BeginRun()
    {

    }

    void Application::EndRun()
    {

    }

    bool Application::BeginDraw()
    {
        //gpuBeginFrame(gpuDevice);
        /*if (!graphics::BeginFrame(mainContext)) {
            return false;
        }*/

        for (auto gameSystem : gameSystems)
        {
            gameSystem->BeginDraw();
        }

        return true;
    }

    void Application::Draw(const GameTime& gameTime)
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Draw(time);
        }

        /*auto texture = swapChain->GetNextTexture();

        ICommandBuffer& commandBuffer = graphicsDevice->GetGraphicsQueue()->RequestCommandBuffer();
        //context->BeginRenderPass(mainSwapChain.get(), Colors::CornflowerBlue);
        commandBuffer.Present(swapChain);
        graphicsDevice->GetGraphicsQueue()->Submit(commandBuffer);*/
    }

#include <DirectXMath.h>
    using namespace DirectX;
    using float4x4 = XMFLOAT4X4;

    struct vs_params_t {
        float4x4 mvp;
    };

    void Application::EndDraw()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

#if defined(TODO_VGPU)

        uint32_t width = window_width(main_window);
        uint32_t height = window_height(main_window);

        XMMATRIX proj = XMMatrixPerspectiveFovRH(XMConvertToRadians(60.0f), (float)width / (float)height, 0.01f, 10.0f);
        XMMATRIX view = XMMatrixLookAtRH(XMVectorSet(0.0f, 1.5f, 6.0f, 0.0f), XMVectorZero(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        XMMATRIX view_proj = XMMatrixMultiply(view, proj);

        static float rx = 0.0f;
        static float ry = 0.0f;

        /* rotated model matrix */
        rx += 1.0f; ry += 2.0f;
        XMMATRIX rxm = XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XMConvertToRadians(rx));
        XMMATRIX rym = XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(ry));
        XMMATRIX model = XMMatrixMultiply(rxm, rym);

        /* model-view-projection matrix for vertex shader */
        vs_params_t vs_params;
        XMMATRIX worldViewProjection = XMMatrixMultiply(model, view_proj);
        XMStoreFloat4x4(&vs_params.mvp, worldViewProjection);
        //vgpu_buffer_sub_data(uboBuffer, 0, 0, &vs_params);

        VGPURenderPassDescriptor renderPass;
        memset(&renderPass, 0, sizeof(VGPURenderPassDescriptor));

        vgpuCmdBeginRenderPass(&renderPass);
        vgpuSetPipeline(render_pipeline);
        vgpuCmdSetVertexBuffers(0, vertexBuffer, 0);
        vgpuCmdSetIndexBuffer(indexBuffer, 0);
        vgpu_set_uniform_buffer_data(0, 0, &vs_params, sizeof(vs_params));
        vgpuCmdDrawIndexed(36, 1, 0);
        vgpuCmdEndRenderPass();
#endif

        //gpuEndFrame(gpuDevice);
        //graphics::EndFrame(mainContext);
    }

    int Application::Run()
    {
        if (running) {
            LOG_ERROR("Application is already running");
            return EXIT_FAILURE;
        }


#if !defined(__GNUC__) && _HAS_EXCEPTIONS
        try
#endif
        {
            Setup();

            if (exitCode)
                return exitCode;

            running = true;

            InitBeforeRun();

            // Main message loop
            while (running)
            {
                os_event evt;
                while (event_poll(&evt))
                {
                    if (evt.type == OS_EVENT_QUIT)
                    {
                        running = false;
                        break;
                    }
                }

                Tick();
            }
        }
#if !defined(__GNUC__) && _HAS_EXCEPTIONS
        catch (std::bad_alloc&)
        {
            //ErrorDialog(GetTypeName(), "An out-of-memory error occurred. The application will now exit.");
            return EXIT_FAILURE;
        }
#endif

        return exitCode;
    }

    void Application::Tick()
    {
        time.Tick([&]()
            {
                Update(time);
            });

        Render();
    }

    void Application::Update(const GameTime& gameTime)
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Update(gameTime);
        }
    }

    void Application::Render()
    {
        // Don't try to render anything before the first Update.
        if (running
            && time.GetFrameCount() > 0
            && !window_is_minimized(main_window)
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
}
