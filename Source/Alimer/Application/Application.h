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
#include "Core/Window.h"
#include "Application/GameTime.h"
#include "Application/GameSystem.h"
#include "Graphics/Graphics.h"
#include "Math/Size.h"

namespace alimer
{
    struct Configuration
    {
        /// Name of the application.
        String applicationName = "Alimer";

        /// Run engine in headless mode.
        bool headless = false;

        /// Main window title.
        String windowTitle = "Alimer";

        /// Main window size.
        SizeI windowSize = { 1280, 720 };
    };

    class ALIMER_API Application : public Object
    {
        ALIMER_OBJECT(Application, Object);

    public:
        /// Constructor.
        Application();

        /// Destructor.
        virtual ~Application();

        /// Run main application loop and setup all required systems.
        int Run();

        /// Tick one frame.
        void Tick();

        /// Get the main (primary window)
        Window& GetWindow() { return *window; }

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
        void PlatformConstruct();
        void PlatformDestroy();
        int PlatformRun();

        /// Called by platform backend.
        void InitBeforeRun();
        
        void Render();

    protected:
        std::vector<std::string> args;

        int exitCode = 0;
        Configuration config;
        bool running = false;

        // Rendering loop timer.
        GameTime time;

        bool headless{ false };
        std::unique_ptr<Window> window;
        std::vector<std::unique_ptr<GameSystem>> gameSystems;
    };
} 

#if defined(_WIN32) && !defined(ALIMER_WIN32_CONSOLE)
#include <windows.h>
#include <crtdbg.h>
#endif

// MSVC debug mode: use memory leak reporting
#if defined(_WIN32) && defined(_DEBUG) && !defined(ALIMER_WIN32_CONSOLE)
#define ALIMER_DEFINE_MAIN(function) \
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) \
{ \
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); \
    return function; \
}
// Other Win32 or minidumps disabled: just execute the function
#elif defined(_WIN32) && !defined(ALIMER_WIN32_CONSOLE)
#define ALIMER_DEFINE_MAIN(function) \
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) \
{ \
    return function; \
}
#else
#define ALIMER_DEFINE_MAIN(function) \
int main(int argc, char** argv) \
{ \
    Alimer::ParseArguments(argc, argv); \
    return function; \
}
#endif

#define ALIMER_DEFINE_APPLICATION(className) \
int RunApplication() \
{ \
    className application; \
    return application.Run(); \
} \
ALIMER_DEFINE_MAIN(RunApplication())