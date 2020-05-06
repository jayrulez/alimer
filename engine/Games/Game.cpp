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
        agpu_shutdown();
        window_destroy(main_window);
        os_shutdown();
    }

    static agpu_buffer buffer;
    static agpu_shader shader;
    static agpu_pipeline render_pipeline;

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

            AGPUSwapChainDescriptor swapchain = {};
            swapchain.nativeHandle = window_handle(main_window);
            swapchain.width = window_width(main_window);
            swapchain.height = window_height(main_window);

            agpu_config gpu_config = {};
            //device_info.preferredBackend = AGPUBackendType_D3D11;
            gpu_config.flags = AGPU_DEVICE_FLAGS_VSYNC;

#ifdef _DEBUG
            gpu_config.flags |= AGPU_DEVICE_FLAGS_DEBUG;
#endif
            gpu_config.gl.get_proc_address = gl_get_proc_address;

            gpu_config.swapchain = &swapchain;

            if (!agpu_init(&gpu_config)) {
                headless = true;
            }

            /*const float vertices[] = {
                // positions            // colors
                 0.0f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
                 0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
                -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f
            };*/

            const float vertices[] = {
                // positions           
                0.0f, 0.5f, 0.5f,
                0.5f, -0.5f, 0.5f,
                -0.5f,  -0.5f, 0.5f
            };

            agpu_buffer_info buffer_info = {};
            buffer_info.size = sizeof(vertices);
            buffer_info.usage = GPU_BUFFER_USAGE_VERTEX;
            buffer_info.content = vertices;
            buffer = agpu_create_buffer(&buffer_info);

            const char* vertexShaderSource = "#version 330 core\n"
                "layout (location = 0) in vec3 aPos;\n"
                "void main()\n"
                "{\n"
                "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                "}\0";

            const char* fragmentShaderSource = "#version 330 core\n"
                "out vec4 FragColor;\n"
                "void main()\n"
                "{\n"
                "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                "}\n\0";


            agpu_shader_info shader_info = {};
            shader_info.vertex.source = vertexShaderSource;
            shader_info.fragment.source = fragmentShaderSource;
            shader = agpu_create_shader(&shader_info);

            agpu_pipeline_info pipeline_info = {};
            pipeline_info.shader = shader;
            render_pipeline = agpu_create_pipeline(&pipeline_info);
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
        agpu_frame_begin();

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

        agpu_set_vertex_buffers(0, 1, &buffer);
        agpu_set_pipeline(render_pipeline);
        agpu_draw(3, 1, 0);
        agpu_frame_finish();
        window_swap_buffers(main_window);
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
