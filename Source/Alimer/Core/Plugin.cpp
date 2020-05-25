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

#include "Core/Plugin.h"
#include "Core/Log.h"
#include "Core/NativeLibrary.h"
#include "os/os.h"

namespace Alimer
{
    PluginManager::PluginManager(Engine& engine)
        : engine{ engine }
    {

    }

    void PluginManager::InitPlugins()
    {
        for (uint32_t i = 0, count = plugins.Size(); i < count; ++i)
        {
            plugins[i]->Init();
        }
    }

    IPlugin* PluginManager::Load(const char* path)
    {
        LOG_INFO("Loading plugin '%s'", path);

        std::string error;
        CreatePluginFn creator;
        std::unique_ptr<NativeLibrary> lib(new NativeLibrary());
        if (!lib->Open(path, &error) ||
            !lib->GetProc(&creator, "AlimerCreatePlugin", &error))
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

        AddPlugin(plugin);
        libraries.Push(std::move(lib));
        LOG_INFO("Plugin '%s' loaded with success.", plugin->GetName());
        return plugin;
    }

    void PluginManager::AddPlugin(IPlugin* plugin)
    {
        plugins.Push(plugin);
    }
}
