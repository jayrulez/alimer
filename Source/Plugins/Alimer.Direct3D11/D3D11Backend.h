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
#include "Core/Assert.h"
#include "Core/Log.h"
#include "graphics/Types.h"
#include "graphics/PixelFormat.h"
#include "graphics/Texture.h"

#ifndef NOMINMAX
#   define NOMINMAX
#endif 

#if defined(_WIN32)
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <wrl/client.h>

#include <d3d11_1.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#if ( defined(_DEBUG) || defined(PROFILE) )
#include <dxgidebug.h>
#endif

#define VHR(hr) if (FAILED(hr)) { ALIMER_ASSERT(false); }
#define SAFE_RELEASE(obj) if ((obj)) { obj->Release(); (obj) = nullptr; }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2Func;
    extern PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1Func;
#else
#define CreateDXGIFactory2Func CreateDXGIFactory2
#define DXGIGetDebugInterface1Func DXGIGetDebugInterface1
#endif

    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    struct DxgiFormatDesc
    {
        PixelFormat format;
        DXGI_FORMAT dxgiFormat;
    };

    extern const DxgiFormatDesc kDxgiFormatDesc[];

    static inline DXGI_FORMAT ToDXGIFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kDxgiFormatDesc[(uint32_t)format].format == format);
        return kDxgiFormatDesc[(uint32_t)format].dxgiFormat;
    }


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
