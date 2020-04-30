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

#include "graphics/Types.h"
#include "math/Color.h"

namespace alimer
{
    class GraphicsDevice;

    /// Command context class for recording copy GPU commands.
    class CopyContext
    {
        friend class GraphicsDevice;

    public:
        /// Destructor.
        virtual ~CopyContext();

        void BeginMarker(const std::string& name);
        void EndMarker();

        void Flush(bool wait = false);

    protected:
        /// Constructor.
        CopyContext(GraphicsDevice& device_);

        void SetName(const std::string& name_) { name = name_; }

        GraphicsDevice& device;
        std::string name;
    };

    /// Command context class for recording compute GPU commands.
    class ComputeContext : public CopyContext
    {
    public:

    protected:
        /// Constructor.
        ComputeContext(GraphicsDevice& device_);
    };

    /// Command context class for recording graphics GPU commands.
    class GraphicsContext final : public ComputeContext
    {
    public:
        /// Constructor.
        GraphicsContext(GraphicsDevice& device_);

        //void BeginRenderPass(SwapChain* swapchain, const Color& clearColor);
        //void EndRenderPass();

    protected:
    };
}
