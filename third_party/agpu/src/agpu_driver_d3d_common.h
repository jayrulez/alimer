//
// Copyright (c) 2019-2020 Amer Koleci.
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

#if defined(AGPU_DRIVER_D3D11) || defined(AGPU_DRIVER_D3D12)
#include "agpu_driver.h"

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#ifdef _DEBUG
#include <dxgidebug.h>

// Declare debug guids to avoid linking with "dxguid.lib"
static const IID D3D_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
static const IID D3D_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

#define VHR(hr) if (FAILED(hr)) { AGPU_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

namespace agpu
{
    enum class DXGIFactoryCaps : uint8_t
    {
        None = 0,
        FlipPresent = (1 << 0),
        HDR = (1 << 1),
        Tearing = (1 << 2)
    };
    AGPU_DEFINE_ENUM_FLAG_OPERATORS(DXGIFactoryCaps, uint8_t);

    static inline DXGI_FORMAT ToDXGIFormat(PixelFormat format) {
        switch (format)
        {
            // 8-bit pixel formats
        case PixelFormat::R8Unorm:      return DXGI_FORMAT_R8_UNORM;
        case PixelFormat::R8Snorm:      return DXGI_FORMAT_R8_SNORM;
        case PixelFormat::R8Uint:       return DXGI_FORMAT_R8_UINT;
        case PixelFormat::R8Sint:       return DXGI_FORMAT_R8_SINT;
            // 16-bit pixel formats
        //case AGPU_TEXTURE_FORMAT_R16_UNORM:     return DXGI_FORMAT_R16_UNORM;
        //case AGPU_TEXTURE_FORMAT_R16_SNORM:     return DXGI_FORMAT_R16_SNORM;
        //case AGPU_TEXTURE_FORMAT_R16_UINT:      return DXGI_FORMAT_R16_UINT;
        //case AGPU_TEXTURE_FORMAT_R16_SINT:      return DXGI_FORMAT_R16_SINT;
        //case AGPU_TEXTURE_FORMAT_R16_FLOAT:     return DXGI_FORMAT_R16_FLOAT;
        //case AGPU_TEXTURE_FORMAT_RG8_UNORM:     return DXGI_FORMAT_R8G8_UNORM;
        //case AGPU_TEXTURE_FORMAT_RG8_SNORM:     return DXGI_FORMAT_R8G8_SNORM;
        //case AGPU_TEXTURE_FORMAT_RG8_UINT:      return DXGI_FORMAT_R8G8_UINT;
        //case AGPU_TEXTURE_FORMAT_RG8_SINT:      return DXGI_FORMAT_R8G8_SINT;
        //    // 32-bit pixel formats
        //case AGPU_TEXTURE_FORMAT_R32_UINT:      return DXGI_FORMAT_R32_UINT;
        //case AGPU_TEXTURE_FORMAT_R32_SINT:      return DXGI_FORMAT_R32_SINT;
        //case AGPU_TEXTURE_FORMAT_R32_FLOAT:     return DXGI_FORMAT_R32_FLOAT;
        //case AGPU_TEXTURE_FORMAT_RG16_UINT:     return DXGI_FORMAT_R16G16_UINT;
        //case AGPU_TEXTURE_FORMAT_RG16_SINT:     return DXGI_FORMAT_R16G16_SINT;
        //case AGPU_TEXTURE_FORMAT_RG16_FLOAT:    return DXGI_FORMAT_R16G16_FLOAT;
        //case AGPU_TEXTURE_FORMAT_RGBA8_UNORM:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        //case AGPU_TEXTURE_FORMAT_RGBA8_SRGB:    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        //case AGPU_TEXTURE_FORMAT_RGBA8_SNORM:   return DXGI_FORMAT_R8G8B8A8_SNORM;
        //case AGPU_TEXTURE_FORMAT_RGBA8_UINT:    return DXGI_FORMAT_R8G8B8A8_UINT;
        //case AGPU_TEXTURE_FORMAT_RGBA8_SINT:    return DXGI_FORMAT_R8G8B8A8_SINT;
        //case AGPU_TEXTURE_FORMAT_BGRA8_UNORM:   return DXGI_FORMAT_B8G8R8A8_UNORM;
        //case AGPU_TEXTURE_FORMAT_BGRA8_SRGB:    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        //    // Packed 32-Bit Pixel formats
        //case AGPU_TEXTURE_FORMAT_RGB10A2_UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
        //case AGPU_TEXTURE_FORMAT_RG11B10_FLOAT: return DXGI_FORMAT_R11G11B10_FLOAT;
        //    // 64-Bit Pixel Formats
        //case AGPU_TEXTURE_FORMAT_RG32_UINT:     return DXGI_FORMAT_R32G32_UINT;
        //case AGPU_TEXTURE_FORMAT_RG32_SINT:     return DXGI_FORMAT_R32G32_SINT;
        //case AGPU_TEXTURE_FORMAT_RG32_FLOAT:    return DXGI_FORMAT_R32G32_FLOAT;
        //case AGPU_TEXTURE_FORMAT_RGBA16_UINT:   return DXGI_FORMAT_R16G16B16A16_UINT;
        //case AGPU_TEXTURE_FORMAT_RGBA16_SINT:   return DXGI_FORMAT_R16G16B16A16_SINT;
        //case AGPU_TEXTURE_FORMAT_RGBA16_FLOAT:  return DXGI_FORMAT_R16G16B16A16_FLOAT;
        //    // 128-Bit Pixel Formats
        //case AGPU_TEXTURE_FORMAT_RGBA32_UINT:   return DXGI_FORMAT_R32G32B32A32_UINT;
        //case AGPU_TEXTURE_FORMAT_RGBA32_SINT:   return DXGI_FORMAT_R32G32B32A32_SINT;
        //case AGPU_TEXTURE_FORMAT_RGBA32_FLOAT:  return DXGI_FORMAT_R32G32B32A32_FLOAT;

        //    // Depth-stencil
        //case AGPU_TEXTURE_FORMAT_D16_UNORM:     return DXGI_FORMAT_D16_UNORM;
        //case AGPU_TEXTURE_FORMAT_D32_FLOAT:     return DXGI_FORMAT_D32_FLOAT;
        //case AGPU_TEXTURE_FORMAT_D24_UNORM_S8_UINT:     return DXGI_FORMAT_D24_UNORM_S8_UINT;
        //case AGPU_TEXTURE_FORMAT_D32_FLOAT_S8_UINT:     return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

        //    // Compressed BC formats
        //case AGPU_TEXTURE_FORMAT_BC1_UNORM:     return DXGI_FORMAT_BC1_UNORM;
        //case AGPU_TEXTURE_FORMAT_BC1_SRGB:      return DXGI_FORMAT_BC1_UNORM_SRGB;
        //case AGPU_TEXTURE_FORMAT_BC2_UNORM:     return DXGI_FORMAT_BC2_UNORM;
        //case AGPU_TEXTURE_FORMAT_BC2_SRGB:      return DXGI_FORMAT_BC2_UNORM_SRGB;
        //case AGPU_TEXTURE_FORMAT_BC3_UNORM:     return DXGI_FORMAT_BC3_UNORM;
        //case AGPU_TEXTURE_FORMAT_BC3_SRGB:      return DXGI_FORMAT_BC3_UNORM_SRGB;
        //case AGPU_TEXTURE_FORMAT_BC4_UNORM:     return DXGI_FORMAT_BC4_UNORM;
        //case AGPU_TEXTURE_FORMAT_BC4_SNORM:     return DXGI_FORMAT_BC4_SNORM;
        //case AGPU_TEXTURE_FORMAT_BC5_UNORM:     return DXGI_FORMAT_BC5_UNORM;
        //case AGPU_TEXTURE_FORMAT_BC5_SNORM:     return DXGI_FORMAT_BC5_SNORM;
        //case AGPU_TEXTURE_FORMAT_BC6H_UFLOAT:   return DXGI_FORMAT_BC6H_UF16;
        //case AGPU_TEXTURE_FORMAT_BC6H_SFLOAT:   return DXGI_FORMAT_BC6H_SF16;
        //case AGPU_TEXTURE_FORMAT_BC7_UNORM:     return DXGI_FORMAT_BC7_UNORM;
        //case AGPU_TEXTURE_FORMAT_BC7_SRGB:      return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:
            AGPU_UNREACHABLE();
        }
    }

    static inline DXGI_FORMAT _vgpuGetTypelessFormatFromDepthFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::Depth16Unorm:
            return DXGI_FORMAT_R16_TYPELESS;
        case PixelFormat::Depth24UnormStencil8:
        //case AGPU_TEXTURE_FORMAT_D32_FLOAT_S8_UINT:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case PixelFormat::Depth32Float:
            return DXGI_FORMAT_R32_TYPELESS;
        default:
            //AGPU_ASSERT(vgpuIsDepthFormat(format) == false);
            return ToDXGIFormat(format);
        }
    }

    static inline DXGI_FORMAT ToDXGISwapChainFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::RGBA32Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case PixelFormat::BGRA8Unorm:
        case PixelFormat::BGRA8UnormSrgb:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case PixelFormat::RGBA8Unorm:
        case PixelFormat::RGBA8UnormSrgb:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case PixelFormat::RGB10A2Unorm:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        }

        return DXGI_FORMAT_B8G8R8A8_UNORM;
    }

    static inline IDXGISwapChain1* d3dCreateSwapChain(
        IDXGIFactory2* dxgi_factory, DXGIFactoryCaps factoryCaps,
        IUnknown* deviceOrCommandQueue,
        void* window_handle,
        DXGI_FORMAT format,
        uint32_t width, uint32_t height,
        uint32_t image_count,
        bool fullscreen)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND window = (HWND)window_handle;
        if (!IsWindow(window)) {
            logError("D3D: Invalid HWND handle");
            return NULL;
        }
#else
        IUnknown* window = (IUnknown*)window_handle;
#endif

        UINT flags = 0;

        if (any(factoryCaps & DXGIFactoryCaps::Tearing))
        {
            flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        const DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
        DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        if (!any(factoryCaps & DXGIFactoryCaps::FlipPresent))
        {
            swapEffect = DXGI_SWAP_EFFECT_DISCARD;
        }
#else
        const DXGI_SCALING scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        const DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Format = format;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = image_count;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Scaling = scaling;
        swapChainDesc.SwapEffect = swapEffect;
        swapChainDesc.Flags = flags;

        IDXGISwapChain1* result = nullptr;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = !fullscreen;

        // Create a SwapChain from a Win32 window.
        VHR(dxgi_factory->CreateSwapChainForHwnd(
            deviceOrCommandQueue,
            window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            &result
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(dxgi_factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
        VHR(IDXGIFactory2_CreateSwapChainForCoreWindow(
            dxgi_factory,
            deviceOrCommandQueue,
            window,
            &swapChainDesc,
            nullptr,
            &result
        ));
#endif

        return result;
    }
}

#endif /* defined(VGPU_DRIVER_D3D11) || defined(VGPU_DRIVER_D3D12) */
