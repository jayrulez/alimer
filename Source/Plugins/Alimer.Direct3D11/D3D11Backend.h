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
#include "graphics/d3d/D3DCommon.h"
#include <d3d11_1.h>

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    // D3D11 functions.
    extern PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    class D3D11GPUDevice;

    static inline UINT ToD3D11BindFlags(TextureUsage usage, bool depthStencilFormat)
    {
        UINT bindFlags = 0;
        if (any(usage & TextureUsage::Sampled))
        {
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }

        if (any(usage & TextureUsage::Storage))
        {
            bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (any(usage & TextureUsage::RenderTarget))
        {
            if (depthStencilFormat)
            {
                bindFlags |= D3D11_BIND_DEPTH_STENCIL;
            }
            else
            {
                bindFlags |= D3D11_BIND_RENDER_TARGET;
            }
        }

        return bindFlags;
    }
}
