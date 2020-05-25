//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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
#include <memory>
#include <vector>

namespace Alimer
{
    class GraphicsDevice;

    /// Defines the entry point for interacting with graphics subsystem.
    class GraphicsProvider
    {
    public:
        GraphicsProvider() = default;
        virtual ~GraphicsProvider() = default;

        virtual std::vector<std::shared_ptr<GraphicsAdapter>> EnumerateGraphicsAdapters() = 0;
        virtual std::shared_ptr<GraphicsDevice> CreateDevice(const std::shared_ptr<GraphicsAdapter>& adapter) = 0;
    };

    class GraphicsProviderFactory
    {
    public:
        GraphicsProviderFactory() = default;
        virtual ~GraphicsProviderFactory() = default;

        virtual BackendType GetBackendType() const = 0;
        virtual std::unique_ptr<GraphicsProvider> CreateProvider(bool validation) = 0;
    };
}
