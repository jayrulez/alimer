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

#pragma once

#include "Platform/Platform.h"
#include "Graphics/gpu/gpu.h"

namespace Alimer
{
    struct Config
    {
        const char* title;
        uint32_t width = 1280;
        uint32_t height = 720;
        bool fullscreen = false; 
        bool resizable = true;
        bool debug;
        int vsync;
        uint32_t sample_count;

        graphics::BackendType graphics_backend;
        graphics::PowerPreference power_preference;
    };

    class ALIMER_API Application 
    {
    public:
        enum class State
        {
            Uninitialized,
        };

        /// Constructor.
        Application(const Config& config);

        /// Destructor.
        virtual ~Application();

        /// Gets the current Application instance.
        static Application* Current();

        /// Setups all subsystem and run's platform main loop.
        void Run();

        void Tick();

        // Gets the config data used to run the application
        const Config* GetConfig();

        /// Get the main window.
        Window& GetMainWindow() const;

    private:
        std::unique_ptr<Platform> platform;
        Config _config;
        State _state;

    };

    extern Application* CreateApplication(void);
} 