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
#include "Core/Log.h"
#include "Graphics/GraphicsDevice.h"

namespace alimer
{
    struct EngineImpl final : Engine
    {
    public:
        void operator=(const EngineImpl&) = delete;
        EngineImpl(const EngineImpl&) = delete;

        EngineImpl(const Configuration& config, IAllocator& allocator_)
            : allocator(allocator_)
            , initialized(false)
        {
            pluginManager = PluginManager::Create(*this);

            for (auto* pluginName : config.plugins) {
                if (pluginName[0] && !pluginManager->Load(pluginName)) {
                    LOG_INFO("%s plugin has not been loaded", pluginName);
                }
            }
        }

        ~EngineImpl()
        {
            SafeDelete(graphicsDevice);

            for (u32 i = 0, count = graphicsDeviceFactories.Size(); i < count; i++) {
                SafeDelete(graphicsDeviceFactories[i]);
            }
            graphicsDeviceFactories.Clear();
            PluginManager::Destroy(pluginManager);
        }

        bool Initialize() override
        {
            if (initialized)
                return true;

            pluginManager->InitPlugins();

            if (!graphicsDeviceFactories.Empty()) {
                bool validation = false;
#ifdef _DEBUG
                validation = true;
#endif
                graphicsDevice = graphicsDeviceFactories[0]->CreateDevice(validation);
            }

            initialized = true;
            return true;
        }

        void RegisterGraphicsDeviceFactory(GraphicsDeviceFactory* factory) override
        {
            ALIMER_ASSERT(factory);
            graphicsDeviceFactories.Push(factory);
        }

        IAllocator& GetAllocator() override { return allocator; }
        PluginManager& GetPluginManager() override { return *pluginManager; }
        GraphicsDevice& GetGraphicsDevice() override { return *graphicsDevice; }
    private:
        IAllocator& allocator;
        PluginManager* pluginManager;
        Vector<GraphicsDeviceFactory*> graphicsDeviceFactories;
        GraphicsDevice* graphicsDevice = nullptr;
        bool initialized;
    };

    Engine* Engine::Create(const Configuration& config, IAllocator& allocator)
    {
        return ALIMER_NEW(allocator, EngineImpl)(config, allocator);
    }

    void Engine::Destroy(Engine* engine)
    {
        ALIMER_DELETE(static_cast<EngineImpl*>(engine)->GetAllocator(), engine);
    }
}
