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

#if defined(__CYGWIN32__)
#   define ALIMER_INTERFACE_EXPORT __declspec(dllexport)
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#   define ALIMER_INTERFACE_EXPORT __declspec(dllexport)
#elif defined(__MACH__) || defined(__ANDROID__) || defined(__linux__)
#   define ALIMER_INTERFACE_EXPORT
#else
#   define ALIMER_INTERFACE_EXPORT
#endif

#include "Core/Platform.h"

namespace alimer
{
    struct ALIMER_API IPlugin
    {
        virtual ~IPlugin();

        virtual void Init() = 0;
        virtual const char* GetName() const = 0;
    };

    struct ALIMER_API PluginManager
    {
        virtual ~PluginManager() {}

        static PluginManager* Create(struct Engine& engine);
        static void Destroy(PluginManager* manager);

        virtual void InitPlugins() = 0;
        virtual IPlugin* Load(const char* path) = 0;
        virtual void AddPlugin(IPlugin* plugin) = 0;
    };
}
