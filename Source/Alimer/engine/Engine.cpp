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

#include "engine/Engine.h"
#include "Core/Utils.h"
#include "engine/Plugin.h"
#include "gpu/gpu.h"

namespace alimer
{
    struct EngineImpl final : Engine
    {
    public:
        void operator=(const EngineImpl&) = delete;
        EngineImpl(const EngineImpl&) = delete;

        EngineImpl(IAllocator& allocator_)
            : allocator(allocator_)
            , initialized(false)
        {
            pluginManager = PluginManager::create(*this);
        }

        ~EngineImpl()
        {
            //graphicsDevice.reset();
            //graphicsProvider.reset();
            //graphicsProviderFactories.Clear();
            PluginManager::destroy(pluginManager);
        }

        bool Initialize() override
        {
            if (initialized)
                return true;

            pluginManager->load("Alimer.Direct3D12");
            pluginManager->load("Alimer.Direct3D11");
            pluginManager->initPlugins();

            /*if (!graphicsProviderFactories.Empty()) {
                bool validation = false;
#ifdef _DEBUG
                validation = true;
#endif
                //graphicsProvider = graphicsProviderFactories[0]->CreateProvider(validation);
                //auto adapter = graphicsProvider->EnumerateGraphicsAdapters();
                //graphicsDevice = graphicsProvider->CreateDevice(adapter[0]);
            }*/

            initialized = true;
            return true;
        }

        /*void registerGraphicsProviderFactory(GraphicsProviderFactory* factory) override
        {
            ALIMER_ASSERT(factory);
            //graphicsProviderFactories.Push(factory);
        }*/

        IAllocator& getAllocator() override { return allocator; }
        PluginManager& getPluginManager() override { return *pluginManager; }

    private:
        IAllocator& allocator;
        PluginManager* pluginManager;
        //Vector<GraphicsProviderFactory*> graphicsProviderFactories;
        bool initialized;
    };

    Engine* Engine::create(IAllocator& allocator)
    {
        return ALIMER_NEW(allocator, EngineImpl)(allocator);
    }

    void Engine::destroy(Engine* engine)
    {
        ALIMER_DELETE(static_cast<EngineImpl*>(engine)->getAllocator(), engine);
    }
}
