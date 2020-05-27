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

#include "engine/Plugin.h"
#include "engine/Engine.h"
#include "Core/Vector.h"
#include "Core/Library.h"
#include "Core/Log.h"

namespace alimer
{
    IPlugin::~IPlugin() = default;

    struct PluginManagerImpl final : PluginManager
    {
    public:
        PluginManagerImpl(Engine& engine_, IAllocator& allocator_)
            : engine(engine_)
            , allocator(allocator_)
            , libraries(/*allocator*/)
            , plugins(/*allocator*/)
        {}

        ~PluginManagerImpl()
        {
        }

        void initPlugins() override
        {
            for (uint32_t i = 0, count = plugins.Size(); i < count; ++i)
            {
                plugins[i]->Init();
            }
        }

        IPlugin* load(const char* path) override
        {
            LOG_INFO("Loading plugin '%s'", path);

            std::string pluginPath(path);
#if WIN32
            pluginPath += ".dll";
#else
            pluginPath += ".so";
#endif

            LibHandle lib = LibraryOpen(pluginPath.c_str());
            if (!lib) {
                return nullptr;
            }

            using PluginCreator = IPlugin * (*)(Engine&);
            PluginCreator creator = (PluginCreator)LibrarySymbol(lib, "AlimerCreatePlugin");
            if (creator)
            {
                return nullptr;
            }

            IPlugin* plugin = creator(engine);
            if (!plugin)
            {
                // TODO: Return error?
                ALIMER_ASSERT_FAIL("Plugin creation failed.");
                return nullptr;
            }

            addPlugin(plugin);
            libraries.Push(std::move(lib));
            LOG_INFO("Plugin '%s' loaded with success.", plugin->GetName());
            return plugin;
        }

        void addPlugin(IPlugin* plugin) override
        {
            plugins.Push(plugin);
        }

        IAllocator& getAllocator() { return allocator; }

    private:
        Engine& engine;
        IAllocator& allocator;
        Vector<void*> libraries;
        Vector<IPlugin*> plugins;
    };

    PluginManager* PluginManager::create(Engine& engine)
    {
        return ALIMER_NEW(engine.getAllocator(), PluginManagerImpl)(engine, engine.getAllocator());
    }

    void PluginManager::destroy(PluginManager* manager)
    {
        ALIMER_DELETE(static_cast<PluginManagerImpl*>(manager)->getAllocator(), manager);
    }
}
