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

#include "Assets/AssetManager.h"
#include "Core/Containers.h"
#include "Graphics/Types.h"
#include "Platform/Window.h"
#include <memory>

namespace alimer
{
    struct Config
    {
        const char* title;
        uint32_t width = 1280;
        uint32_t height = 720;
        bool fullscreen = false;
        bool resizable = true;

        GraphicsBackendType backendType = GraphicsBackendType::Count;
        GraphicsDeviceFlags deviceFlags = GraphicsDeviceFlags::None;

        std::string rootDirectory = "Assets";
    };

    class CommandList;
    class Graphics;
    class ImGuiLayer;

    class ALIMER_API Application : public Object
    {
        ALIMER_OBJECT(Application, Object);

    public:
        enum class State
        {
            Uninitialized,
            Splash,
            Loading,
            Running,
            Paused
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

        const std::string& GetName() const;

        void SetName(const std::string& name);

        // Gets the config data used to run the application
        const Config* GetConfig();

        /// Get the main window.
        Window& GetWindow() const;

        AssetManager& GetAssets()
        {
            return assets;
        }

        const AssetManager& GetAssets() const
        {
            return assets;
        }

    protected:
        virtual void Initialize() {}
        virtual void OnDraw(CommandList& commandList) {}

    private:
        void InitBeforeRun();

        std::string name{};
        Config config;

    protected:
        State state;
        AssetManager assets;
        bool headless = false;
        std::unique_ptr<Window> window{nullptr};
        RefPtr<Graphics> graphics;
        std::unique_ptr<ImGuiLayer> imguiLayer{nullptr};
    };

    extern Application* CreateApplication(void);
}
