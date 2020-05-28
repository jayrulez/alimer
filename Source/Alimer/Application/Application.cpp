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
#include "Application/Window.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/SwapChain.h"
#include "Input/InputManager.h"
#include "Core/Log.h"

namespace alimer
{
    Application::Application()
        : input(new InputManager())
    {
        gameSystems.Push(input);
        bool enableDebugLayer = false;
#ifdef _DEBUG
        enableDebugLayer = true;
#endif
        graphicsDevice = GraphicsDevice::Create(FeatureLevel::Level11_0, enableDebugLayer);
        PlatformConstuct();
    }

    Application::~Application()
    {
        for (auto gameSystem : gameSystems)
        {
            SafeDelete(gameSystem);
        }

        gameSystems.Clear();
        mainWindow.reset();
        PlatformDestroy();
    }

    void Application::InitBeforeRun()
    {
        // Create main window.
        if (!headless)
        {
            mainWindow.reset(new Window(
                config.windowTitle,
                config.windowSize.width, config.windowSize.height,
                WindowFlags::Resizable)
            );
            //window_set_centered(main_window);

            SwapChainDescriptor swapChainDescriptor = {};
            swapChainDescriptor.width = mainWindow->GetSize().width;
            swapChainDescriptor.height = mainWindow->GetSize().height;
            swapChainDescriptor.colorFormat = PixelFormat::BGRA8UNormSrgb;
            mainSwapChain = graphicsDevice->CreateSwapChain(mainWindow->GetHandle(), &swapChainDescriptor);
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
        if (!graphicsDevice->BeginFrame()) {
            return false;
        }

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

        mainSwapChain->Present();
        graphicsDevice->EndFrame();
    }

    int Application::Run()
    {
        if (running) {
            LOG_ERROR("Application is already running");
            return EXIT_FAILURE;
        }

        PlatformRun();
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
            && !mainWindow->IsMinimized()
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
} // namespace alimer
