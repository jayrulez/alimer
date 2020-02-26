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

#include "Core/Utils.h"
#include "Graphics/PixelFormat.h"

namespace Alimer
{
    /// Enum describing the GraphicsDevice backend.
    enum class GraphicsBackend : uint32_t {
        /// Default best platform supported backend.
        Default,
        /// Null backend.
        Null,
        /// Vulkan backend.
        Vulkan,
        /// Direct3D 12 backend.
        Direct3D12,
        /// Metal backend.
        Metal
    };

    enum class GPUPowerPreference : uint32_t {
        DontCare,
        LowPower,
        HighPerformance
    };

    enum class GraphicsDeviceFlags : uint32_t {
        None = 0x0,
        /// Enable debug runtime.
        DebugRuntime = 0x1,
        /// Enable GPU based validation.
        GPUBasedValidation = 0x2,
        /// Enable RenderDoc integration.
        RenderDoc = 0x4
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(GraphicsDeviceFlags);

    /// GraphicsDevice descriptor.
    struct GraphicsDeviceDescriptor
    {
        GraphicsBackend preferredBackend = GraphicsBackend::Default;

        /// Device flags.
        GraphicsDeviceFlags flags = GraphicsDeviceFlags::None;

        /// GPU device power preference.
        GPUPowerPreference powerPreference;
    };

} 
