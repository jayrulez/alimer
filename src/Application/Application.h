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

#include "Application/Window.h"
#include "Graphics/GraphicsDevice.h"
#include <EASTL/vector.h>

namespace Alimer
{
    struct Configuration
    {
        eastl::string windowTitle = "Alimer";

        uint32_t windowWidth = 1280;
        uint32_t windowHeight = 720;
    };

    class AppContext;

    class ALIMER_API Application
    {
        friend class AppContext;
    public:
        Application(const Configuration& config);
        Application(AppContext* context, const Configuration& config);

        /// Destructor.
        virtual ~Application();

        /// Run main application loop and setup all required systems.
        void Run();

        /// Tick one frame.
        void Tick();

        /// Get the default window size.
        virtual void GetDefaultWindowSize(uint32_t *width, uint32_t* height) const;

        Window* GetMainWindow() const;

    private:
        /// Called by AppContext
        void InitBeforeRun();

    protected:
        Configuration _config;
        bool _running = false;
        bool _exiting = false;
        GraphicsDevice* graphicsDevice = nullptr;

    private:
        AppContext* _context;
        bool _ownContext;
    };

    extern Application* ApplicationCreate(const eastl::vector<eastl::string>& args);

    // Call this to ensure application-main is linked in correctly without having to mess around
    // with -Wl,--whole-archive.
    ALIMER_API void ApplicationDummy();
}
