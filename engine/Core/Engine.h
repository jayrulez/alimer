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

#include "Core/Vector.h"

namespace alimer
{
    struct ALIMER_API Engine
    {
    public:
        struct Configuration {
            const char* workingDir = nullptr;
            Vector<const char*> plugins;
        };

        /// Destructor.
        virtual ~Engine() {}

        static Engine* Create(const Configuration& config, IAllocator& allocator);
        static void Destroy(Engine* engine);

        /// Initialize engine with all subsystems and load all plugins.
        virtual bool Initialize() = 0;

        virtual IAllocator& GetAllocator() = 0;
        virtual struct PluginManager& GetPluginManager() = 0;
    };
}
