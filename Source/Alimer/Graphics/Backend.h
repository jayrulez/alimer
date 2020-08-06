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

#include "config.h"

#if defined(ALIMER_D3D11)
#   define D3D11_NO_HELPERS
#   include <d3d11_1.h>
#elif defined(ALIMER_D3D12)
#include <d3d12.h>

namespace D3D12MA
{
    class Allocator;
    class Allocation;
};

#elif defined(ALIMER_VULKAN)
#else
#   error "Invalid graphics backend"
#endif

namespace alimer
{
#if defined(ALIMER_D3D11)
    using BufferHandle = ID3D11Buffer*;
    using TextureHandle = ID3D11Resource*;
#elif defined(ALIMER_D3D12)
    using AllocationHandle = D3D12MA::Allocation*;
    using BufferHandle = ID3D12Resource*;
    using TextureHandle = ID3D12Resource*;
#elif defined(ALIMER_VULKAN)
    using BufferHandle = VkBuffer;
    using TextureHandle = VkImage;
#else
#   error "Invalid graphics backend"
#endif
}
