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

#if defined(VGPU_DRIVER_D3D11) || defined(VGPU_DRIVER_D3D12)
#include "vgpu_driver.h"

#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#ifdef _DEBUG
#include <dxgidebug.h>

static const GUID vgpu_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
static const GUID vgpu_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

#define VHR(hr) if (FAILED(hr)) { __debugbreak(); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

namespace vgpu
{

    static inline DXGI_FORMAT _vgpu_d3d_format(PixelFormat format) {
        switch (format) {
        case PixelFormat::R8Unorm:  return DXGI_FORMAT_R8_UNORM;
        case PixelFormat::R8Snorm:  return DXGI_FORMAT_R8_SNORM;
        case PixelFormat::R8Uint:   return DXGI_FORMAT_R8_UINT;
        case PixelFormat::R8Sint:   return DXGI_FORMAT_R8_SINT;

            //case VGPU_PIXEL_FORMAT_R16_UNORM:     return DXGI_FORMAT_R16_UNORM;
            //case VGPU_PIXEL_FORMAT_R16_SNORM:     return DXGI_FORMAT_R16_SNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_R16_UINT:      return DXGI_FORMAT_R16_UINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_R16_SINT:      return DXGI_FORMAT_R16_SINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_R16_FLOAT:     return DXGI_FORMAT_R16_FLOAT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG8_UNORM:     return DXGI_FORMAT_R8G8_UNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG8_SNORM:     return DXGI_FORMAT_R8G8_SNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG8_UINT:      return DXGI_FORMAT_R8G8_UINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG8_SINT:      return DXGI_FORMAT_R8G8_SINT;

        case PixelFormat::VGPU_PIXEL_FORMAT_R32_UINT:      return DXGI_FORMAT_R32_UINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_R32_SINT:       return DXGI_FORMAT_R32_SINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_R32_FLOAT:      return DXGI_FORMAT_R32_FLOAT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG16_UINT:      return DXGI_FORMAT_R16G16_UINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG16_SINT:      return DXGI_FORMAT_R16G16_SINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG16_FLOAT:     return DXGI_FORMAT_R16G16_FLOAT;

        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA8_UNORM:       return DXGI_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA8_UNORM_SRGB:  return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA8_SNORM:       return DXGI_FORMAT_R8G8B8A8_SNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA8_UINT:        return DXGI_FORMAT_R8G8B8A8_UINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA8_SINT:        return DXGI_FORMAT_R8G8B8A8_SINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_BGRA8_UNORM:       return DXGI_FORMAT_B8G8R8A8_UNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_BGRA8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case PixelFormat::VGPU_PIXEL_FORMAT_RGB10A2_UNORM:     return DXGI_FORMAT_R10G10B10A2_UNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG11B10_FLOAT:     return DXGI_FORMAT_R11G11B10_FLOAT;

        case PixelFormat::VGPU_PIXEL_FORMAT_RG32_UINT:         return DXGI_FORMAT_R32G32_UINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG32_SINT:         return DXGI_FORMAT_R32G32_SINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RG32_FLOAT:        return DXGI_FORMAT_R32G32_FLOAT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA16_UINT:       return DXGI_FORMAT_R16G16B16A16_UINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA16_SINT:       return DXGI_FORMAT_R16G16B16A16_SINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA16_FLOAT:      return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA32_UINT:       return DXGI_FORMAT_R32G32B32A32_UINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA32_SINT:       return DXGI_FORMAT_R32G32B32A32_SINT;
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA32_FLOAT:      return DXGI_FORMAT_R32G32B32A32_FLOAT;

            //case VGPU_PIXEL_FORMAT_DEPTH16_UNORM:     return DXGI_FORMAT_D16_UNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_DEPTH32_FLOAT:     return DXGI_FORMAT_D32_FLOAT;
        case PixelFormat::VGPU_PIXEL_FORMAT_DEPTH24_STENCIL8:  return DXGI_FORMAT_D24_UNORM_S8_UINT;

        case PixelFormat::BC1RGBAUnorm:             return DXGI_FORMAT_BC1_UNORM;
        case PixelFormat::BC1RGBAUnormSrgb:         return DXGI_FORMAT_BC1_UNORM_SRGB;
        case PixelFormat::BC2RGBAUnorm:             return DXGI_FORMAT_BC2_UNORM;
        case PixelFormat::BC2RGBAUnormSrgb:         return DXGI_FORMAT_BC2_UNORM_SRGB;
        case PixelFormat::BC3RGBAUnorm:             return DXGI_FORMAT_BC3_UNORM;
        case PixelFormat::BC3RGBAUnormSrgb:         return DXGI_FORMAT_BC3_UNORM_SRGB;
        case PixelFormat::BC4RUnorm:                return DXGI_FORMAT_BC4_UNORM;
        case PixelFormat::BC4RSnorm:                return DXGI_FORMAT_BC4_SNORM;
        case PixelFormat::BC5RGUnorm:               return DXGI_FORMAT_BC5_UNORM;
        case PixelFormat::BC5RGSnorm:               return DXGI_FORMAT_BC5_SNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_BC6HRGB_UFLOAT:      return DXGI_FORMAT_BC6H_UF16;
        case PixelFormat::VGPU_PIXEL_FORMAT_BC6HRGB_SFLOAT:      return DXGI_FORMAT_BC6H_SF16;
        case PixelFormat::VGPU_PIXEL_FORMAT_BC7RGBA_UNORM:       return DXGI_FORMAT_BC7_UNORM;
        case PixelFormat::VGPU_PIXEL_FORMAT_BC7RGBA_UNORM_SRGB:  return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:
            VGPU_UNREACHABLE();
        }
    }

    static inline DXGI_FORMAT vgpu_d3d_swapchain_format(PixelFormat format) {
        switch (format) {
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case PixelFormat::VGPU_PIXEL_FORMAT_BGRA8_UNORM:
        case PixelFormat::VGPU_PIXEL_FORMAT_BGRA8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA8_UNORM:
        case PixelFormat::VGPU_PIXEL_FORMAT_RGBA8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case PixelFormat::VGPU_PIXEL_FORMAT_RGB10A2_UNORM:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        }

        return DXGI_FORMAT_B8G8R8A8_UNORM;
    }

    static inline DXGI_FORMAT _vgpu_d3d_typeless_from_depth_format(PixelFormat format) {
        switch (format) {
            //case VGPUPixelFormat_D16Unorm:
            //    return DXGI_FORMAT_R16_TYPELESS;
        //case VGPU_PIXEL_FORMAT_DEPTH24_PLUS:
        case PixelFormat::VGPU_PIXEL_FORMAT_DEPTH24_STENCIL8:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case PixelFormat::VGPU_PIXEL_FORMAT_DEPTH32_FLOAT:
            return DXGI_FORMAT_R32_TYPELESS;
        default:
            //VGPU_ASSERT(vgpu_is_depth_format(format) == false);
            return _vgpu_d3d_format(format);
        }
    }

    static inline DXGI_FORMAT _vgpu_d3d_format_with_usage(PixelFormat format, uint32_t usage) {
        // If depth and either ua or sr, set to typeless
        /*if (vgpu_is_depth_stencil_format(format)
            && ((usage & (VGPU_TEXTURE_USAGE_SAMPLED | VGPU_TEXTURE_USAGE_STORAGE)) != 0))
        {
            return _vgpu_d3d_typeless_from_depth_format(format);
        }*/

        return _vgpu_d3d_format(format);
    }


    enum class DXGIFactoryCaps : uint8_t
    {
        None = 0,
        FlipPresent = (1 << 0),
        Tearing = (1 << 1),
        HDR = (1 << 2)
    };
    VGPU_DEFINE_ENUM_FLAG_OPERATORS(DXGIFactoryCaps, uint8_t);

    static inline IDXGISwapChain1* vgpu_d3d_create_swapchain(
        IDXGIFactory2* dxgi_factory, DXGIFactoryCaps factoryCaps,
        IUnknown* deviceOrCommandQueue,
        uintptr_t window_handle,
        uint32_t width, uint32_t height,
        PixelFormat format,
        uint32_t backbuffer_count,
        bool is_fullscreen)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND window = (HWND)window_handle;
        if (!IsWindow(window)) {
            //vgpu_log_error("Invalid HWND handle");
            return NULL;
        }
#else
        IUnknown* window = (IUnknown*)handle;
#endif

        UINT flags = 0;

        if (any(factoryCaps & DXGIFactoryCaps::Tearing))
        {
            flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
        DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        if (!any(factoryCaps & DXGIFactoryCaps::FlipPresent))
        {
            swapEffect = DXGI_SWAP_EFFECT_DISCARD;
        }
#else
        DXGI_SCALING scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        swapchain_desc.Width = width;
        swapchain_desc.Height = height;
        swapchain_desc.Format = vgpu_d3d_swapchain_format(format);
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = backbuffer_count;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapchain_desc.Scaling = scaling;
        swapchain_desc.SwapEffect = swapEffect;
        swapchain_desc.Flags = flags;

        IDXGISwapChain1* result = NULL;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchain_fullscreen_desc = {};
        swapchain_fullscreen_desc.Windowed = !is_fullscreen;

        // Create a SwapChain from a Win32 window.
        VHR(dxgi_factory->CreateSwapChainForHwnd(
            deviceOrCommandQueue,
            window,
            &swapchain_desc,
            &swapchain_fullscreen_desc,
            nullptr,
            &result
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(dxgi_factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
        VHR(dxgi_factory->CreateSwapChainForCoreWindow(
            deviceOrCommandQueue,
            window,
            &swapchain_desc,
            nullptr,
            &result
        ));
#endif

        return result;
    }
}

#endif /* defined(VGPU_DRIVER_D3D11) || defined(VGPU_DRIVER_D3D12) */
