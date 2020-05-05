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
#include "gpu/gpu.h"
#include "Games/Game.h"
#include "graphics/GraphicsDevice.h"
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
        agpu_wait_gpu(gpuDevice);
        agpuDeviceDestroy(gpuDevice);
        window_destroy(main_window);
        os_shutdown();
    }

    void Game::InitBeforeRun()
    {
        // Create main window.
        if (!headless)
        {
            main_window = window_create(
                config.windowTitle.c_str(),
                config.windowSize.width, config.windowSize.height,
                WINDOW_FLAG_RESIZABLE);
            window_set_centered(main_window);

            AGPUSwapChainDescriptor swapchain = {};
            swapchain.nativeHandle = window_handle(main_window);

            agpu_device_info device_info = {};
            device_info.preferredBackend = AGPUBackendType_D3D11;
            device_info.flags = AGPU_DEVICE_FLAGS_VSYNC;

#ifdef _DEBUG
            device_info.flags |= AGPU_DEVICE_FLAGS_DEBUG;
#endif
            device_info.swapchain = &swapchain;

            gpuDevice = agpuCreateDevice(&device_info);
            if (!gpuDevice) {
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

#if TODO
        struct Vertex
        {
            float3 position;
            float4 color;
        };

        // Define the geometry for a triangle.
        Vertex triangle_vertices[] =
        {
            { { 0.0f, 0.5, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        vgpu_buffer_desc buffer_desc = {};
        buffer_desc.usage = VGPU_BUFFER_USAGE_VERTEX;
        buffer_desc.size = sizeof(triangle_vertices);
        buffer_desc.content = triangle_vertices;
        vertex_buffer = vgpu_create_buffer(&buffer_desc);

        std::string shader_source = R"(
            struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
})";

        vgpu_shader_desc shader_desc = {};
        shader_desc.vertex.source = shader_source.c_str();
        shader_desc.vertex.entry_point = "VSMain";
        shader_desc.fragment.source = shader_source.c_str();
        shader_desc.fragment.entry_point = "PSMain";
        shader = vgpu_create_shader(&shader_desc);

        vgpu_render_pipeline_desc pipeline_desc = {};
        pipeline_desc.shader = shader;
        render_pipeline = vgpu_create_render_pipeline(&pipeline_desc);
#endif // TODO

    }

    void Game::BeginRun()
    {

    }

    void Game::EndRun()
    {

    }

    bool Game::BeginDraw()
    {
        agpu_frame_begin(gpuDevice);

        for (auto gameSystem : gameSystems)
        {
            gameSystem->BeginDraw();
        }

        return true;
    }

    void Game::Draw(const GameTime& gameTime)
    {
        //auto context = graphicsDevice->GetGraphicsContext();
        //context->BeginRenderPass(mainSwapChain.get(), Colors::CornflowerBlue);
        //context->EndMarker();

        for (auto gameSystem : gameSystems)
        {
            gameSystem->Draw(time);
        }
    }

    void Game::EndDraw()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        //auto currentTexture = gpuSwapChainGetCurrentTextureView(gpuSwapChain);
        //graphicsDevice->PresentFrame();

        /*auto clear_color = Colors::CornflowerBlue;
        auto defaultRenderPass = vgpu_get_default_render_pass();
        vgpu_render_pass_set_color_clear_value(defaultRenderPass, 0, &clear_color.r);
        vgpu_cmd_begin_render_pass(defaultRenderPass);
        vgpu_cmd_end_render_pass();
        */

        agpu_frame_end(gpuDevice);
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
