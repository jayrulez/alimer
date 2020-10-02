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

enum class DXGIFactoryCaps : uint8_t
{
    None = 0,
    FlipPresent = (1 << 0),
    HDR = (1 << 1),
    Tearing = (1 << 2)
};
DEFINE_ENUM_FLAG_OPERATORS(DXGIFactoryCaps);

static inline DXGI_FORMAT _agpu_d3d11_dxgi_format(agpu_texture_format format) {
    switch (format)
    {
        // 8-bit pixel formats
    case agpu_texture_format::R8Unorm:      return DXGI_FORMAT_R8_UNORM;
    case agpu_texture_format::R8Snorm:      return DXGI_FORMAT_R8_SNORM;
    case agpu_texture_format::R8Uint:       return DXGI_FORMAT_R8_UINT;
    case agpu_texture_format::R8Sint:       return DXGI_FORMAT_R8_SINT;
        // 16-bit pixel formats
    case agpu_texture_format::R16Unorm:     return DXGI_FORMAT_R16_UNORM;
    case agpu_texture_format::R16Snorm:     return DXGI_FORMAT_R16_SNORM;
    case agpu_texture_format::R16Uint:      return DXGI_FORMAT_R16_UINT;
    case agpu_texture_format::R16Sint:      return DXGI_FORMAT_R16_SINT;
    case agpu_texture_format::R16Float:     return DXGI_FORMAT_R16_FLOAT;
    case agpu_texture_format::RG8Unorm:     return DXGI_FORMAT_R8G8_UNORM;
    case agpu_texture_format::RG8Snorm:     return DXGI_FORMAT_R8G8_SNORM;
    case agpu_texture_format::RG8Uint:      return DXGI_FORMAT_R8G8_UINT;
    case agpu_texture_format::RG8Sint:      return DXGI_FORMAT_R8G8_SINT;
        // 32-bit pixel formats
    case agpu_texture_format::R32Uint:          return DXGI_FORMAT_R32_UINT;
    case agpu_texture_format::R32Sint:          return DXGI_FORMAT_R32_SINT;
    case agpu_texture_format::R32Float:         return DXGI_FORMAT_R32_FLOAT;
    case agpu_texture_format::RG16Unorm:        return DXGI_FORMAT_R16G16_UNORM;
    case agpu_texture_format::RG16Snorm:        return DXGI_FORMAT_R16G16_SNORM;
    case agpu_texture_format::RG16Uint:         return DXGI_FORMAT_R16G16_UINT;
    case agpu_texture_format::RG16Sint:         return DXGI_FORMAT_R16G16_SINT;
    case agpu_texture_format::RG16Float:        return DXGI_FORMAT_R16G16_FLOAT;
    case AGPU_TEXTURE_FORMAT_RGBA8_UNORM:       return DXGI_FORMAT_R8G8B8A8_UNORM;
    case AGPU_TEXTURE_FORMAT_RGBA8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case AGPU_TEXTURE_FORMAT_RGBA8_SNORM:       return DXGI_FORMAT_R8G8B8A8_SNORM;
    case AGPU_TEXTURE_FORMAT_RGBA8_UINT:        return DXGI_FORMAT_R8G8B8A8_UINT;
    case AGPU_TEXTURE_FORMAT_RGBA8_SINT:        return DXGI_FORMAT_R8G8B8A8_SINT;
    case AGPU_TEXTURE_FORMAT_BGRA8_UNORM:       return DXGI_FORMAT_B8G8R8A8_UNORM;
    case AGPU_TEXTURE_FORMAT_BGRA8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        // Packed 32-Bit Pixel formats
    case AGPU_TEXTURE_FORMAT_RGB10A2_UNORM:     return DXGI_FORMAT_R10G10B10A2_UNORM;
    case agpu_texture_format::RG11B10Float:     return DXGI_FORMAT_R11G11B10_FLOAT;
    case agpu_texture_format::RGB9E5Float:      return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        // 64-Bit Pixel Formats
    case agpu_texture_format::RG32Uint:         return DXGI_FORMAT_R32G32_UINT;
    case agpu_texture_format::RG32Sint:         return DXGI_FORMAT_R32G32_SINT;
    case agpu_texture_format::RG32Float:        return DXGI_FORMAT_R32G32_FLOAT;
    case AGPU_TEXTURE_FORMAT_RGBA16_UNORM:      return DXGI_FORMAT_R16G16B16A16_UNORM;
    case agpu_texture_format::RGBA16Snorm:      return DXGI_FORMAT_R16G16B16A16_SNORM;
    case agpu_texture_format::RGBA16Uint:       return DXGI_FORMAT_R16G16B16A16_UINT;
    case agpu_texture_format::RGBA16Sint:       return DXGI_FORMAT_R16G16B16A16_SINT;
    case agpu_texture_format::RGBA16Float:      return DXGI_FORMAT_R16G16B16A16_FLOAT;
        // 128-Bit Pixel Formats
    case agpu_texture_format::RGBA32Uint:       return DXGI_FORMAT_R32G32B32A32_UINT;
    case agpu_texture_format::RGBA32Sint:       return DXGI_FORMAT_R32G32B32A32_SINT;
    case AGPU_TEXTURE_FORMAT_RGBA32_FLOAT:      return DXGI_FORMAT_R32G32B32A32_FLOAT;

        // Depth-stencil
    case agpu_texture_format::Stencil8:             return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case agpu_texture_format::Depth16Unorm:         return DXGI_FORMAT_D16_UNORM;
    case agpu_texture_format::Depth24Plus:          return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case agpu_texture_format::Depth24PlusStencil8:  return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case agpu_texture_format::Depth32Float:         return DXGI_FORMAT_D32_FLOAT;

        // Compressed BC formats
    case agpu_texture_format::BC1RGBAUnorm:         return DXGI_FORMAT_BC1_UNORM;
    case agpu_texture_format::BC1RGBAUnormSrgb:     return DXGI_FORMAT_BC1_UNORM_SRGB;
    case agpu_texture_format::BC2RGBAUnorm:         return DXGI_FORMAT_BC2_UNORM;
    case agpu_texture_format::BC2RGBAUnormSrgb:     return DXGI_FORMAT_BC2_UNORM_SRGB;
    case agpu_texture_format::BC3RGBAUnorm:         return DXGI_FORMAT_BC3_UNORM;
    case agpu_texture_format::BC3RGBAUnormSrgb:     return DXGI_FORMAT_BC3_UNORM_SRGB;
    case agpu_texture_format::BC4RUnorm:            return DXGI_FORMAT_BC4_UNORM;
    case agpu_texture_format::BC4RSnorm:            return DXGI_FORMAT_BC4_SNORM;
    case agpu_texture_format::BC5RGUnorm:           return DXGI_FORMAT_BC5_UNORM;
    case agpu_texture_format::BC5RGSnorm:           return DXGI_FORMAT_BC5_SNORM;
    case agpu_texture_format::BC6HRGBUfloat:        return DXGI_FORMAT_BC6H_UF16;
    case agpu_texture_format::BC6HRGBFloat:         return DXGI_FORMAT_BC6H_SF16;
    case agpu_texture_format::BC7RGBAUnorm:         return DXGI_FORMAT_BC7_UNORM;
    case agpu_texture_format::BC7RGBAUnormSrgb:     return DXGI_FORMAT_BC7_UNORM_SRGB;
    default:
        AGPU_UNREACHABLE();
    }
}

static inline DXGI_FORMAT _vgpuGetTypelessFormatFromDepthFormat(agpu_texture_format format)
{
    switch (format)
    {
    case agpu_texture_format::Stencil8:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case agpu_texture_format::Depth16Unorm:
        return DXGI_FORMAT_R16_TYPELESS;
    case agpu_texture_format::Depth24Plus:
    case agpu_texture_format::Depth24PlusStencil8:
        return DXGI_FORMAT_R24G8_TYPELESS;

    case agpu_texture_format::Depth32Float:
        return DXGI_FORMAT_R32_TYPELESS;

    default:
        //AGPU_ASSERT(IsDepthFormat(format) == false);
        return _agpu_d3d11_dxgi_format(format);
    }
}

static inline DXGI_FORMAT _agpu_d3d_swapchain_format(agpu_texture_format format)
{
    switch (format)
    {
    case AGPU_TEXTURE_FORMAT_RGBA32_FLOAT:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;

    case AGPU_TEXTURE_FORMAT_BGRA8_UNORM:
    case AGPU_TEXTURE_FORMAT_BGRA8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM;

    case AGPU_TEXTURE_FORMAT_RGBA8_UNORM:
    case AGPU_TEXTURE_FORMAT_RGBA8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case AGPU_TEXTURE_FORMAT_RGB10A2_UNORM:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    }

    return DXGI_FORMAT_B8G8R8A8_UNORM;
}

static inline IDXGISwapChain1* agpu_d3d_create_swapchain(
    IDXGIFactory2* dxgiFactory, DXGIFactoryCaps factoryCaps,
    IUnknown* deviceOrCommandQueue,
    void* window_handle,
    DXGI_FORMAT format,
    uint32_t width, uint32_t height,
    uint32_t buffer_count,
    bool is_fullscreen)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window = (HWND)window_handle;
    if (!IsWindow(window)) {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "D3D: Invalid HWND handle");
        return nullptr;
    }
#else
    IUnknown* window = (IUnknown*)window_handle;
#endif

    UINT flags = 0;

    if ((factoryCaps & DXGIFactoryCaps::Tearing) != DXGIFactoryCaps::None)
    {
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    const DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
    DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if ((factoryCaps & DXGIFactoryCaps::FlipPresent) == DXGIFactoryCaps::None)
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
    swapChainDesc.BufferCount = buffer_count;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Scaling = scaling;
    swapChainDesc.SwapEffect = swapEffect;
    swapChainDesc.Flags = flags;

    IDXGISwapChain1* result = nullptr;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = !is_fullscreen;

    // Create a SwapChain from a Win32 window.
    VHR(dxgiFactory->CreateSwapChainForHwnd(
        deviceOrCommandQueue,
        window,
        &swapChainDesc,
        &fsSwapChainDesc,
        nullptr,
        &result
    ));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
    VHR(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
    VHR(dxgiFactory->CreateSwapChainForCoreWindow(
        deviceOrCommandQueue,
        window,
        &swapChainDesc,
        nullptr,
        &result
    ));
#endif

    return result;
}

static inline int agpu_StringConvert(const char* from, wchar_t* to)
{
    int num = MultiByteToWideChar(CP_UTF8, 0, from, -1, NULL, 0);
    if (num > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, from, -1, &to[0], num);
    }

    return num;
}

#endif /* defined(VGPU_DRIVER_D3D11) || defined(VGPU_DRIVER_D3D12) */
