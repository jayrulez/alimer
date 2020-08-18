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

/* DXGI guids */
static const IID D3D_IID_IDXGIAdapter1 = { 0x29038f61, 0x3839, 0x4626, {0x91, 0xfd, 0x08, 0x68, 0x79, 0x01, 0x1a, 0x05} };
static const IID D3D_IID_IDXGIDevice1 = { 0x77db970f, 0x6276, 0x48ba, {0xba, 0x28, 0x07, 0x01, 0x43, 0xb4, 0x39, 0x2c } };
static const IID D3D_IID_IDXGIFactory2 = { 0x50c83a1c, 0xe072, 0x4c48, {0x87, 0xb0, 0x36, 0x30, 0xfa, 0x36, 0xa6, 0xd0} };
static const IID D3D_IID_IDXGISwapChain3 = { 0x94d99bdb, 0xf1f8, 0x4ab0, {0xb2, 0x36, 0x7d, 0xa0, 0x17, 0x0e, 0xda, 0xb1} };
static const IID D3D_IID_IDXGIFactory4 = { 0x1bc6ea02, 0xef36, 0x464f, {0xbf, 0x0c, 0x21, 0xca, 0x39, 0xe5, 0x16, 0x8a} };
static const IID D3D_IID_IDXGIFactory5 = { 0x7632e1f5, 0xee65, 0x4dca, {0x87, 0xfd, 0x84, 0xcd, 0x75, 0xf8, 0x83, 0x8d} };
static const IID D3D_IID_IDXGIFactory6 = { 0xc1b6694f, 0xff09, 0x44a9, {0xb0, 0x3c, 0x77, 0x90, 0x0a, 0x0a, 0x1d, 0x17 } };

#ifdef _DEBUG
#include <dxgidebug.h>

static const IID D3D_IID_IDXGIInfoQueue = { 0xD67441C7, 0x672A, 0x476f, {0x9E, 0x82, 0xCD, 0x55, 0xB4, 0x49, 0x49, 0xCE} };
static const IID D3D_IID_IDXGIDebug = { 0x119E7452, 0xDE9E, 0x40fe, {0x88, 0x06, 0x88, 0xF9, 0x0C, 0x12, 0xB4, 0x41 } };
static const IID D3D_IID_IDXGIDebug1 = { 0xc5a05f0c,0x16f2,0x4adf, {0x9f,0x4d,0xa8,0xc4,0xd5,0x8a,0xc5,0x50 } };

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
//#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

typedef enum {
    DXGIFACTORY_CAPS_FLIP_PRESENT = (1 << 0),
    DXGIFACTORY_CAPS_TEARING = (1 << 1),
} dxgi_factory_caps;

static inline DXGI_FORMAT _vgpuGetDXGIFormat(agpu_texture_format format) {
    switch (format) {
    case AGPUPixelFormat_R8UNorm:
        return DXGI_FORMAT_R8_UNORM;
    case AGPUPixelFormat_R8SNorm:
        return DXGI_FORMAT_R8_SNORM;
    case AGPUPixelFormat_R8UInt:
        return DXGI_FORMAT_R8_UINT;
    case AGPUPixelFormat_R8SInt:
        return DXGI_FORMAT_R8_SINT;
    case AGPUPixelFormat_R16UInt:
        return DXGI_FORMAT_R16_UINT;
    case AGPUPixelFormat_R16SInt:
        return DXGI_FORMAT_R16_SINT;
    case AGPUPixelFormat_R16Float:
        return DXGI_FORMAT_R16_FLOAT;
    case AGPUPixelFormat_RG8UNorm:
        return DXGI_FORMAT_R8G8_UNORM;
    case AGPUPixelFormat_RG8SNorm:
        return DXGI_FORMAT_R8G8_SNORM;
    case AGPUPixelFormat_RG8UInt:
        return DXGI_FORMAT_R8G8_UINT;
    case AGPUPixelFormat_RG8SInt:
        return DXGI_FORMAT_R8G8_SINT;
    case AGPUPixelFormat_R32Float:
        return DXGI_FORMAT_R32_FLOAT;
    case AGPUPixelFormat_R32UInt:
        return DXGI_FORMAT_R32_UINT;
    case AGPUPixelFormat_R32SInt:
        return DXGI_FORMAT_R32_SINT;
    case AGPUPixelFormat_RG16UInt:
        return DXGI_FORMAT_R16G16_UINT;
    case AGPUPixelFormat_RG16SInt:
        return DXGI_FORMAT_R16G16_SINT;
    case AGPUPixelFormat_RG16Float:
        return DXGI_FORMAT_R16G16_FLOAT;
    case AGPUPixelFormat_RGBA8Unorm:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case AGPUPixelFormat_RGBA8UnormSrgb:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case AGPUPixelFormat_RGBA8SNorm:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case AGPUPixelFormat_RGBA8UInt:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case AGPUPixelFormat_RGBA8SInt:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case AGPUPixelFormat_BGRA8Unorm:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case AGPUPixelFormat_BGRA8UnormSrgb:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case AGPUPixelFormat_RGB10A2Unorm:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    case AGPUPixelFormat_RG11B10Float:
        return DXGI_FORMAT_R11G11B10_FLOAT;
    case AGPUPixelFormat_RG32Float:
        return DXGI_FORMAT_R32G32_FLOAT;
    case AGPUPixelFormat_RG32UInt:
        return DXGI_FORMAT_R32G32_UINT;
    case AGPUPixelFormat_RG32SInt:
        return DXGI_FORMAT_R32G32_SINT;
    case AGPUPixelFormat_RGBA16UInt:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case AGPUPixelFormat_RGBA16SInt:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case AGPUPixelFormat_RGBA16Float:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case AGPUPixelFormat_RGBA32Float:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case AGPUPixelFormat_RGBA32UInt:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case AGPUPixelFormat_RGBA32SInt:
        return DXGI_FORMAT_R32G32B32A32_SINT;

        // Depth-stencil
    case AGPUPixelFormat_Depth16Unorm:          return DXGI_FORMAT_D16_UNORM;
    case AGPUPixelFormat_Depth32Float:          return DXGI_FORMAT_D32_FLOAT;
    case AGPUPixelFormat_Depth24Unorm_Stencil8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case AGPUPixelFormat_Depth32Float_Stencil8: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

        // Compressed BC formats
    case AGPUPixelFormat_BC1RGBAUnorm:      return DXGI_FORMAT_BC1_UNORM;
    case AGPUPixelFormat_BC1RGBAUnormSrgb:  return DXGI_FORMAT_BC1_UNORM_SRGB;
    case AGPUPixelFormat_BC2RGBAUnorm:      return DXGI_FORMAT_BC2_UNORM;
    case AGPUPixelFormat_BC2RGBAUnormSrgb:  return DXGI_FORMAT_BC2_UNORM_SRGB;
    case AGPUPixelFormat_BC3RGBAUnorm:      return DXGI_FORMAT_BC3_UNORM;
    case AGPUPixelFormat_BC3RGBAUnormSrgb:  return DXGI_FORMAT_BC3_UNORM_SRGB;
    case AGPUPixelFormat_BC4RUnorm:         return DXGI_FORMAT_BC4_UNORM;
    case AGPUPixelFormat_BC4RSnorm:         return DXGI_FORMAT_BC4_SNORM;
    case AGPUPixelFormat_BC5RGUnorm:        return DXGI_FORMAT_BC5_UNORM;
    case AGPUPixelFormat_BC5RGSnorm:        return DXGI_FORMAT_BC5_SNORM;
    case AGPUPixelFormat_BC6HRGBUfloat:     return DXGI_FORMAT_BC6H_UF16;
    case AGPUPixelFormat_BC6HRGBSfloat:     return DXGI_FORMAT_BC6H_SF16;
    case AGPUPixelFormat_BC7RGBAUnorm:      return DXGI_FORMAT_BC7_UNORM;
    case AGPUPixelFormat_BC7RGBAUnormSrgb:  return DXGI_FORMAT_BC7_UNORM_SRGB;
    default:
        AGPU_UNREACHABLE();
    }
}

static inline DXGI_FORMAT _vgpuGetTypelessFormatFromDepthFormat(agpu_texture_format format)
{
    switch (format)
    {
    case AGPUPixelFormat_Depth16Unorm:
        return DXGI_FORMAT_R16_TYPELESS;
    case AGPUPixelFormat_Depth24Unorm_Stencil8:
    case AGPUPixelFormat_Depth32Float_Stencil8:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case AGPUPixelFormat_Depth32Float:
        return DXGI_FORMAT_R32_TYPELESS;
    default:
        //AGPU_ASSERT(vgpuIsDepthFormat(format) == false);
        return _vgpuGetDXGIFormat(format);
    }
}


/*static inline UINT _vgpuGetSyncInterval(VGPUPresentMode mode)
{
    switch (mode)
    {
    case VGPUPresentMode_Mailbox:
        return 2;

    case VGPUPresentMode_Immediate:
        return 0;

    case VGPUPresentMode_Fifo:
    default:
        return 1;
    }
}*/

static inline DXGI_FORMAT vgpu_d3d_swapchain_format(agpu_texture_format format)
{
    switch (format)
    {
    case AGPUPixelFormat_RGBA16Float:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;

    case AGPUPixelFormat_BGRA8Unorm:
    case AGPUPixelFormat_BGRA8UnormSrgb:
        return DXGI_FORMAT_B8G8R8A8_UNORM;

    case AGPUPixelFormat_RGBA8Unorm:
    case AGPUPixelFormat_RGBA8UnormSrgb:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case AGPUPixelFormat_RGB10A2Unorm:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    }

    return DXGI_FORMAT_B8G8R8A8_UNORM;
}

static inline IDXGISwapChain1* vgpu_d3d_create_swapchain(
    IDXGIFactory2* dxgiFactory,
    IUnknown* deviceOrCommandQueue,
    uint32_t caps,
    uintptr_t handle,
    uint32_t width,
    uint32_t height,
    agpu_texture_format format,
    uint32_t image_count,
    bool fullscreen)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window = (HWND)handle;
    if (!IsWindow(window)) {
        //agpuLog(AGPULogLevel_Error, "Invalid HWND handle");
        return NULL;
    }
#else
    IUnknown* window = (IUnknown*)handle;
#endif

    UINT flags = 0;

    if (caps & DXGIFACTORY_CAPS_TEARING)
    {
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
    DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if (!(caps & DXGIFACTORY_CAPS_FLIP_PRESENT))
    {
        swapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }
#else
    DXGI_SCALING scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
    DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

    const DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
        .Width = width,
        .Height = height,
        .Format = vgpu_d3d_swapchain_format(format),
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = image_count,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
        .Scaling = scaling,
        .SwapEffect = swapEffect,
        .Flags = flags
    };

    IDXGISwapChain1* result = NULL;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchain_fullscreen_desc = {
       .Windowed = !fullscreen
    };

    // Create a SwapChain from a Win32 window.
    VHR(IDXGIFactory2_CreateSwapChainForHwnd(
        dxgiFactory,
        deviceOrCommandQueue,
        window,
        &swapchain_desc,
        &swapchain_fullscreen_desc,
        NULL,
        &result
    ));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
    VHR(IDXGIFactory2_MakeWindowAssociation(dxgiFactory, window, DXGI_MWA_NO_ALT_ENTER));
#else
    VHR(IDXGIFactory2_CreateSwapChainForCoreWindow(
        dxgiFactory,
        deviceOrCommandQueue,
        window,
        &swapchain_desc,
        NULL,
        &result
    ));
#endif

    return result;
}

#endif /* defined(VGPU_DRIVER_D3D11) || defined(VGPU_DRIVER_D3D12) */
