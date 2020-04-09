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

#include "graphics/GraphicsAdapter.h"
#include <set>
#include <vector>
#include <memory>

namespace alimer
{
    /// Defines a graphics provider class which enumerates the physical adapters.
    class ALIMER_API GraphicsProvider
    {
    public:
        /// Constructor.
        GraphicsProvider() = default;

        /// Destructor.
        virtual ~GraphicsProvider() = default;

        /// Get set of available graphics providers.
        static std::set<BackendType> GetAvailableProviders();

        /// Create new GPUDevice with given preferred backend, fallback to supported one.
        static std::unique_ptr<GraphicsProvider> Create(const std::string& applicationName, GraphicsProviderFlags flags = GraphicsProviderFlags::None, BackendType preferredBackend = BackendType::Count);
       
        /// Enumerate all graphics adapter.
        virtual std::vector<std::unique_ptr<GraphicsAdapter>> EnumerateGraphicsAdapters() = 0;

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsProvider);
    };
}
