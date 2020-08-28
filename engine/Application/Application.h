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

#include "Core/Log.h"
#include "Math/Size.h"
#include "Application/GameTime.h"
#include "Application/Window.h"
#include "Application/GameSystem.h"

namespace Alimer
{
    struct Configuration
    {
        /// Run engine in headless mode.
        bool headless = false;

        /// Main window size.
        SizeI windowSize = { 1280, 720 };
    };

    class AppHost;

    class ALIMER_API Application : public Object
    {
        friend class AppHost;

        ALIMER_OBJECT(Application, Object);

    public:
        /// Constructor.
        Application(const Configuration& config);

        /// Destructor.
        virtual ~Application();

        static Application* GetCurrent();

        /// Run main application loop and setup all required systems.
        int Run();

        /// Tick one frame.
        void Tick();

        /// Get the application configuration.
        ALIMER_FORCE_INLINE const Configuration& GetConfig() const
        {
            return config;
        }

        /// Gets the application name.
        virtual std::string GetName()
        {
            return "Alimer";
        }

        Window* GetWindow() const;

    protected:
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

        static Application* s_current;
        std::unique_ptr<AppHost> host;

    protected:
        std::vector<String> args;

        int exitCode = 0;
        Configuration config;
        bool running = false;

        // Rendering loop timer.
        GameTime time;

        //std::vector<std::unique_ptr<GameSystem>> gameSystems;
    };

    extern Application* ApplicationCreate(int argc, char* argv[]);

    // Call this to ensure application-main is linked in correctly without having to mess around
    // with -Wl,--whole-archive.
    void ApplicationDummy();
} 
