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
#include "Games/Game.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/ICommandQueue.h"
#include "graphics/ICommandBuffer.h"
#include "graphics/ISwapChain.h"
#include "Input/InputManager.h"
#include "core/Log.h"

namespace alimer
{
    Game::Game(const Configuration& config_)
        : config(config_)
        , input(new InputManager())
    {
        /* Init OS first */
        os_init();

        gameSystems.push_back(input);
    }

    Game::~Game()
    {
        for (auto gameSystem : gameSystems)
        {
            SafeDelete(gameSystem);
        }

        gameSystems.clear();
        vgpu_shutdown();
        window_destroy(main_window);
        os_shutdown();
    }

    /*static vgpu_buffer* vertexBuffer;
    static vgpu_buffer* indexBuffer;
    static vgpu_buffer* uboBuffer;
    static vgpu_shader shader;
    static agpu_pipeline render_pipeline;*/

    static void  gpu_callback(void* context, vgpu_log_level level, const char* message)
    {
    }

    void Game::InitBeforeRun()
    {
        // Create main window.
        if (!headless)
        {
            main_window = window_create(
                config.windowTitle.c_str(),
                config.windowSize.width, config.windowSize.height,
                WINDOW_FLAG_RESIZABLE | WINDOW_FLAG_OPENGL);
            window_set_centered(main_window);

            vgpu_swapchain_info swapchain_info = {};
            swapchain_info.handle = window_handle(main_window);
            swapchain_info.width = window_width(main_window);
            swapchain_info.height = window_height(main_window);

            vgpu_config gpu_config = { };
            gpu_config.type = VGPU_BACKEND_TYPE_VULKAN;
            //deviceDesc.type = VGPU_BACKEND_TYPE_OPENGL;
#ifdef _DEBUG
            gpu_config.debug = true;
#endif
            gpu_config.callback = gpu_callback;
            gpu_config.context = this;
            gpu_config.get_proc_address = gl_get_proc_address;
            gpu_config.swapchain_info = &swapchain_info;
            if (!vgpu_init(&gpu_config)) {
                headless = true;
            }
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

    void Game::Initialize()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Initialize();
        }
    }

    void Game::BeginRun()
    {

    }

    void Game::EndRun()
    {

    }

    bool Game::BeginDraw()
    {
        vgpu_frame_begin();

        for (auto gameSystem : gameSystems)
        {
            gameSystem->BeginDraw();
        }

        return true;
    }

    void Game::Draw(const GameTime& gameTime)
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

    void Game::EndDraw()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        vgpu_frame_end();

        return;

#if TODO
        /*auto clear_color = Colors::CornflowerBlue;
auto defaultRenderPass = vgpu_get_default_render_pass();
vgpu_render_pass_set_color_clear_value(defaultRenderPass, 0, &clear_color.r);
vgpu_cmd_begin_render_pass(defaultRenderPass);
vgpu_cmd_end_render_pass();
*/
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
        vgpuFrameFinish();
        window_swap_buffers(main_window);
#endif // TODO

    }

    int Game::Run()
    {
        if (running) {
            ALIMER_LOGERROR("Application is already running");
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

    void Game::Tick()
    {
        time.Tick([&]()
            {
                Update(time);
            });

        Render();
    }

    void Game::Update(const GameTime& gameTime)
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Update(gameTime);
        }
    }

    void Game::Render()
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
