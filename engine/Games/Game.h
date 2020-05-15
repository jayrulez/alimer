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

#include "Containers/Array.h"
#include "core/Object.h"
#include "Games/GameTime.h"
#include "os/os.h"
#include "Games/GameSystem.h"
#include "math/Size.h"
#include "graphics/types.h"
#include <memory>

namespace alimer
{
    struct Configuration
    {
        /// Name of the application.
        std::string applicationName = "Alimer";

        /// Run engine in headless mode.
        bool headless = false;

        /// Main window title.
        std::string windowTitle = "Alimer";

        /// Main window size.
        usize windowSize = { 1280, 720 };
    };

    class InputManager;
    class IGraphicsDevice;

    class ALIMER_API Game : public Object
    {
        ALIMER_OBJECT(Game, Object);

    public:
        /// Constructor.
        Game(const Configuration& config_);
        /// Destructor.
        virtual ~Game();

        /// Run main application loop and setup all required systems.
        int Run();

        /// Tick one frame.
        void Tick();

        /// Get the main (primary window)
        inline window_t* get_main_window() const { return main_window; }

        inline InputManager* GetInput() const noexcept { return input; }

    protected:
        /// Setup before modules initialization. 
        virtual void Setup() {}

        /// Setup after window and graphics setup, by default initializes all GameSystems.
        virtual void Initialize();

        virtual void BeginRun();
        virtual void EndRun();

        virtual void Update(const GameTime& gameTime);

        virtual bool BeginDraw();
        virtual void Draw(const GameTime& gameTime);
        virtual void EndDraw();

    private:
        /// Called by platform backend.
        void InitBeforeRun();
        
        void Render();

    protected:
        int exitCode = 0;
        Configuration config;
        bool running = false;
        // Rendering loop timer.
        GameTime time;
        window_t* main_window = nullptr;
        Vector<GameSystem*> gameSystems;
        std::unique_ptr<IGraphicsDevice> graphicsDevice;

        InputManager* input;
        bool headless{ false };
    };

    extern Game* ApplicationCreate(const Vector<std::string>& args);

    // Call this to ensure application-main is linked in correctly without having to mess around
    // with -Wl,--whole-archive.
    ALIMER_API void ApplicationDummy();
}
