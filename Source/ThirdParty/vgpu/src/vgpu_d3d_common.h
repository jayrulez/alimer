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

#if defined(VGPU_DRIVER_D3D11) || defined(VGPU_DRIVER_D3D12)
#include "vgpu_driver.h"

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

#define VHR(hr) if (FAILED(hr)) { VGPU_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

typedef enum {
    DXGIFACTORY_CAPS_FLIP_PRESENT = (1 << 0),
    DXGIFACTORY_CAPS_TEARING = (1 << 1),
} dxgi_factory_caps;

static inline DXGI_FORMAT _vgpuGetDXGIFormat(VGPUPixelFormat format) {
    switch (format) {
    case VGPUTextureFormat_R8UNorm:
        return DXGI_FORMAT_R8_UNORM;
    case VGPUTextureFormat_R8SNorm:
        return DXGI_FORMAT_R8_SNORM;
    case VGPUTextureFormat_R8UInt:
        return DXGI_FORMAT_R8_UINT;
    case VGPUTextureFormat_R8SInt:
        return DXGI_FORMAT_R8_SINT;
    case VGPUTextureFormat_R16UInt:
        return DXGI_FORMAT_R16_UINT;
    case VGPUTextureFormat_R16SInt:
        return DXGI_FORMAT_R16_SINT;
    case VGPUTextureFormat_R16Float:
        return DXGI_FORMAT_R16_FLOAT;
    case VGPUTextureFormat_RG8UNorm:
        return DXGI_FORMAT_R8G8_UNORM;
    case VGPUTextureFormat_RG8SNorm:
        return DXGI_FORMAT_R8G8_SNORM;
    case VGPUTextureFormat_RG8UInt:
        return DXGI_FORMAT_R8G8_UINT;
    case VGPUTextureFormat_RG8SInt:
        return DXGI_FORMAT_R8G8_SINT;
    case VGPUTextureFormat_R32Float:
        return DXGI_FORMAT_R32_FLOAT;
    case VGPUTextureFormat_R32UInt:
        return DXGI_FORMAT_R32_UINT;
    case VGPUTextureFormat_R32SInt:
        return DXGI_FORMAT_R32_SINT;
    case VGPUTextureFormat_RG16UInt:
        return DXGI_FORMAT_R16G16_UINT;
    case VGPUTextureFormat_RG16SInt:
        return DXGI_FORMAT_R16G16_SINT;
    case VGPUTextureFormat_RG16Float:
        return DXGI_FORMAT_R16G16_FLOAT;
    case VGPUTextureFormat_RGBA8UNorm:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case VGPUTextureFormat_RGBA8UNormSrgb:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case VGPUTextureFormat_RGBA8SNorm:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case VGPUTextureFormat_RGBA8UInt:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case VGPUTextureFormat_RGBA8SInt:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case VGPUTextureFormat_BGRA8UNorm:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case VGPUTextureFormat_BGRA8UNormSrgb:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case VGPUTextureFormat_RGB10A2UNorm:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    case VGPUTextureFormat_RG11B10Float:
        return DXGI_FORMAT_R11G11B10_FLOAT;
    case VGPUTextureFormat_RG32Float:
        return DXGI_FORMAT_R32G32_FLOAT;
    case VGPUTextureFormat_RG32UInt:
        return DXGI_FORMAT_R32G32_UINT;
    case VGPUTextureFormat_RG32SInt:
        return DXGI_FORMAT_R32G32_SINT;
    case VGPUTextureFormat_RGBA16UInt:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case VGPUTextureFormat_RGBA16SInt:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case VGPUTextureFormat_RGBA16Float:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case VGPUTextureFormat_RGBA32Float:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case VGPUTextureFormat_RGBA32UInt:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case VGPUTextureFormat_RGBA32SInt:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case VGPUTextureFormat_Depth32Float:
        return DXGI_FORMAT_D32_FLOAT;
    case VGPUTextureFormat_Depth24Plus:
        return DXGI_FORMAT_D32_FLOAT;
    case VGPUTextureFormat_Depth24PlusStencil8:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case VGPUTextureFormat_BC1RGBAUNorm:
        return DXGI_FORMAT_BC1_UNORM;
    case VGPUTextureFormat_BC1RGBAUNormSrgb:
        return DXGI_FORMAT_BC1_UNORM_SRGB;
    case VGPUTextureFormat_BC2RGBAUNorm:
        return DXGI_FORMAT_BC2_UNORM;
    case VGPUTextureFormat_BC2RGBAUNormSrgb:
        return DXGI_FORMAT_BC2_UNORM_SRGB;
    case VGPUTextureFormat_BC3RGBAUNorm:
        return DXGI_FORMAT_BC3_UNORM;
    case VGPUTextureFormat_BC3RGBAUNormSrgb:
        return DXGI_FORMAT_BC3_UNORM_SRGB;
    case VGPUTextureFormat_BC4RUNorm:
        return DXGI_FORMAT_BC4_UNORM;
    case VGPUTextureFormat_BC4RSNorm:
        return DXGI_FORMAT_BC4_SNORM;
    case VGPUTextureFormat_BC5RGUNorm:
        return DXGI_FORMAT_BC5_UNORM;
    case VGPUTextureFormat_BC5RGSNorm:
        return DXGI_FORMAT_BC5_SNORM;
    case VGPUTextureFormat_BC6HRGBUFloat:
        return DXGI_FORMAT_BC6H_UF16;
    case VGPUTextureFormat_BC6HRGBSFloat:
        return DXGI_FORMAT_BC6H_SF16;
    case VGPUTextureFormat_BC7RGBAUNorm:
        return DXGI_FORMAT_BC7_UNORM;
    case VGPUTextureFormat_BC7RGBAUNormSrgb:
        return DXGI_FORMAT_BC7_UNORM_SRGB;
    default:
        VGPU_UNREACHABLE();
    }
}

static inline DXGI_FORMAT _vgpuGetTypelessFormatFromDepthFormat(VGPUPixelFormat format)
{
    switch (format)
    {
        //case VGPUPixelFormat_D16Unorm:
        //    return DXGI_FORMAT_R16_TYPELESS;
    case VGPUTextureFormat_Depth24Plus:
    case VGPUTextureFormat_Depth24PlusStencil8:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case VGPUTextureFormat_Depth32Float:
        return DXGI_FORMAT_R32_TYPELESS;
    default:
        VGPU_ASSERT(vgpuIsDepthFormat(format) == false);
        return _vgpuGetDXGIFormat(format);
    }
}


static inline UINT _vgpuGetSyncInterval(VGPUPresentMode mode)
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
}

static inline DXGI_FORMAT vgpu_d3d_swapchain_format(VGPUPixelFormat format)
{
    switch (format)
    {
    case VGPUTextureFormat_RGBA16Float:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;

    case VGPUTextureFormat_BGRA8UNorm:
    case VGPUTextureFormat_BGRA8UNormSrgb:
        return DXGI_FORMAT_B8G8R8A8_UNORM;

    case VGPUTextureFormat_RGBA8UNorm:
    case VGPUTextureFormat_RGBA8UNormSrgb:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case VGPUTextureFormat_RGB10A2UNorm:
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
    VGPUPixelFormat format,
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

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = vgpu_d3d_swapchain_format(format);
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = image_count;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Scaling = scaling;
    swapChainDesc.SwapEffect = swapEffect;
    swapChainDesc.Flags = flags;

    IDXGISwapChain1* result = NULL;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = !fullscreen;

    // Create a SwapChain from a Win32 window.
    VHR(dxgiFactory->CreateSwapChainForHwnd(
        deviceOrCommandQueue,
        window,
        &swapChainDesc,
        &fsSwapChainDesc,
        NULL,
        &result
    ));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
    VHR(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
    VHR(dxgiFactory->CreateSwapChainForCoreWindow(
        deviceOrCommandQueue,
        window,
        &swapChainDesc,
        NULL,
        &result
    ));
#endif

    return result;
}

#endif /* defined(VGPU_DRIVER_D3D11) || defined(VGPU_DRIVER_D3D12) */
