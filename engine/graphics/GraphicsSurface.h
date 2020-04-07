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

#include "graphics/Types.h"
#include "core/Utils.h"
#include "math/size.h"

namespace alimer
{
    /// Defines a graphics surface type.
    enum class GraphicsSurfaceType : uint32_t
    {
        Win32,
        UwpCoreWindow,
        UwpSwapChainPanel
    };

    /// Defines the graphics surface which can be rendered on by a graphics device.
    class ALIMER_API GraphicsSurface
    {
    public:
        /// Constructor.
        GraphicsSurface() = default;

        /// Destructor.
        virtual ~GraphicsSurface() = default;

        /// Get the surface size.
        virtual usize GetSize() const = 0;

        /// Get the surface type.
        virtual GraphicsSurfaceType GetType() const = 0;

        /// Get the surface handle.
        virtual void* GetHandle() const = 0;

        /// Get the surface display type.
        virtual void* GetDisplay() const = 0;

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsSurface);
    };
}
