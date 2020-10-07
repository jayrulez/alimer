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

#include "Core/Assert.h"
#include "Core/Log.h"
#include "RHI/Types.h"
#include "RHI/D3D/D3DHelpers.h"
#define D3D11_NO_HELPERS
#include <d3d11_3.h>

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    class D3D11GraphicsDevice;

    void D3D11SetObjectName(ID3D11DeviceChild* obj, const std::string& name);

    static inline TextureUsage D3D11GetTextureUsage(UINT bindFlags)
    {
        TextureUsage usage = TextureUsage::None;
        if (bindFlags & D3D11_BIND_SHADER_RESOURCE) {
            usage |= TextureUsage::Sampled;
        }
        if (bindFlags & D3D11_BIND_UNORDERED_ACCESS) {
            usage |= TextureUsage::Storage;
        }
        if (bindFlags & D3D11_BIND_RENDER_TARGET) {
            usage |= TextureUsage::RenderTarget;
        }
        if (bindFlags & D3D11_BIND_DEPTH_STENCIL) {
            usage |= TextureUsage::RenderTarget;
        }

        return usage;
    }
}
