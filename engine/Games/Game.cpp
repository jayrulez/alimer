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
#include "graphics/GPUDevice.h"
#include "graphics/SwapChain.h"
#include "Input/InputManager.h"
#include "core/Log.h"

namespace alimer
{
    Game::Game(const Configuration& config_)
        : config(config_)
        , input(new InputManager())
    {
        os::init();
        gameSystems.push_back(input);
    }

    Game::~Game()
    {
        for (auto gameSystem : gameSystems)
        {
            SafeDelete(gameSystem);
        }

        gameSystems.clear();
        gpuDevice.reset();
        os::shutdown();
    }

    void Game::InitBeforeRun()
    {
        // Create main window.
        main_window.create(config.windowTitle, { centered, centered }, config.windowSize, WindowStyle::Resizable);

        // Init graphics with main window.
        GPUDeviceFlags deviceFlags = GPUDeviceFlags::None;
#if defined(GPU_DEBUG)
        deviceFlags |= GPUDeviceFlags::Validation;
#endif
        gpuDevice = GPUDevice::Create(config.gpuBackend, deviceFlags);

        /*agpu_swapchain_desc swapchain_desc = {};
        swapchain_desc.width = main_window.get_size().width;
        swapchain_desc.height = main_window.get_size().height;
        swapchain_desc.native_display = main_window.get_native_display();
        swapchain_desc.native_handle = main_window.get_native_handle();

        agpu_config config = {};
        //config.preferred_backend = AGPU_BACKEND_OPENGL;
        //config.get_gl_proc_address = os::get_proc_address;
        config.preferred_backend = AGPU_BACKEND_VULKAN;
#ifdef _DEBUG
        config.flags |= AGPU_CONFIG_FLAGS_VALIDATION;
#endif
        config.swapchain = &swapchain_desc;

        if (!agpu_init(&config))
        {
            headless = true;
        }*/

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
        //gpuDevice->beginFrame();

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
    }

    void Game::EndDraw()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        gpuDevice->CommitFrame();
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
                os::Event evt{};
                while (os::poll_event(evt))
                {
                    if (evt.type == os::Event::Type::Quit)
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
            && !main_window.is_minimized()
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
    }
