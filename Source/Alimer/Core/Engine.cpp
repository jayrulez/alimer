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

#include "Core/Engine.h"
#include "Core/Utils.h"
#include "Core/Plugin.h"
#include "graphics/GraphicsProvider.h"

namespace Alimer
{
    Engine::Engine()
        : pluginManager(new PluginManager(*this))
    {

    }

    Engine::~Engine()
    {
        graphicsDevice.reset();
        graphicsProvider.reset();
        graphicsProviderFactories.clear();
        SafeDelete(pluginManager);
    }

    bool Engine::Initialize()
    {
        if (initialized)
            return true;

        pluginManager->Load("Alimer.Direct3D11.dll");
        pluginManager->InitPlugins();

        if (!graphicsProviderFactories.empty()) {
            bool validation = false;
#ifdef _DEBUG
            validation = true;
#endif
            graphicsProvider = graphicsProviderFactories[0]->CreateProvider(validation);
            auto adapter = graphicsProvider->EnumerateGraphicsAdapters();
            graphicsDevice = graphicsProvider->CreateDevice(adapter[0]);
        }

        initialized = true;
        return true;
    }

    void Engine::RegisterGraphicsProviderFactory(GraphicsProviderFactory* factory)
    {
        graphicsProviderFactories.emplace_back(factory);
    }
}
