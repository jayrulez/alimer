//
// Copyright (c) 2019 Amer Koleci.
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

#include "gpu_backend.h"
#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#define VHR(hr) if (FAILED(hr)) { VGPU_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

static DXGI_FORMAT d3d_GetFormat(AGPUPixelFormat format) {
    static DXGI_FORMAT formats[AGPUPixelFormat_Count] = {
        DXGI_FORMAT_UNKNOWN,
        // 8-bit pixel formats
        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8_SNORM,
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8_SINT,
        // 16-bit pixel formats
        //DXGI_FORMAT_R16_UNORM,
        //DXGI_FORMAT_R16_SNORM,
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SINT,
        DXGI_FORMAT_R16_FLOAT,
        DXGI_FORMAT_R8G8_UNORM,
        DXGI_FORMAT_R8G8_SNORM,
        DXGI_FORMAT_R8G8_UINT,
        DXGI_FORMAT_R8G8_SINT,

        // 32-bit pixel formats
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_R32_FLOAT,

        //DXGI_FORMAT_R16G16_UNORM,
        //DXGI_FORMAT_R16G16_SNORM,
        DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_R16G16_FLOAT,

        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_SNORM,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SINT,

        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,

        // Packed 32-Bit Pixel formats
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R11G11B10_FLOAT,

        // 64-Bit Pixel Formats
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32_SINT,
        DXGI_FORMAT_R32G32_FLOAT,
        //DXGI_FORMAT_R16G16B16A16_UNORM,
        //DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,

        // 128-Bit Pixel Formats
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,

        // Depth-stencil formats
        DXGI_FORMAT_D16_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,

        // Compressed BC formats
        DXGI_FORMAT_BC1_UNORM,
        DXGI_FORMAT_BC1_UNORM_SRGB,
        DXGI_FORMAT_BC2_UNORM,
        DXGI_FORMAT_BC2_UNORM_SRGB,
        DXGI_FORMAT_BC3_UNORM,
        DXGI_FORMAT_BC3_UNORM_SRGB,
        DXGI_FORMAT_BC4_UNORM,
        DXGI_FORMAT_BC4_SNORM,
        DXGI_FORMAT_BC5_UNORM,
        DXGI_FORMAT_BC5_SNORM,
        DXGI_FORMAT_BC6H_UF16,
        DXGI_FORMAT_BC6H_SF16,
        DXGI_FORMAT_BC7_UNORM,
        DXGI_FORMAT_BC7_UNORM_SRGB,
    };
    return formats[format];
}

static DXGI_FORMAT d3d_GetTypelessFormat(AGPUPixelFormat format)
{
    switch (format)
    {
    case AGPUPixelFormat_Depth16Unorm:
        return DXGI_FORMAT_R16_TYPELESS;
    case AGPUPixelFormat_Depth32Float:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case AGPUPixelFormat_Depth24Plus:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case AGPUPixelFormat_Depth24PlusStencil8:
        return DXGI_FORMAT_R32_TYPELESS;
    default:
        VGPU_ASSERT(!agpuIsDepthFormat(format));
        return d3d_GetFormat(format);
    }
}

static DXGI_FORMAT d3d_GetTextureFormat(AGPUPixelFormat format, AGPUTextureUsageFlags usage)
{
    if (agpuIsDepthFormat(format) &&
        (usage & AGPUTextureUsage_Sampled | AGPUTextureUsage_Storage) != AGPUTextureUsage_None)
    {
        return d3d_GetTypelessFormat(format);
    }

    return d3d_GetFormat(format);
}

static DXGI_FORMAT d3d_GetSwapChainFormat(AGPUPixelFormat format) {
    switch (format)
    {
    case AGPUPixelFormat_Undefined:
    case AGPUPixelFormat_BGRA8Unorm:
    case AGPUPixelFormat_BGRA8UnormSrgb:
        return DXGI_FORMAT_B8G8R8A8_UNORM;

    case AGPUPixelFormat_RGBA8Unorm:
    case AGPUPixelFormat_RGBA8UnormSrgb:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case AGPUPixelFormat_RGBA16Float:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;

    case AGPUPixelFormat_RGB10A2Unorm:
        return DXGI_FORMAT_R10G10B10A2_UNORM;

    default:
        //vgpu_log_error_format("PixelFormat (%u) is not supported for creating swapchain buffer", (uint32_t)format);
        return DXGI_FORMAT_UNKNOWN;
    }
}


static DXGI_FORMAT d3d_GetVertexFormat(AGPUVertexFormat format) {
    switch (format)
    {
    case AGPUVertexFormat_UChar2:
        return DXGI_FORMAT_R8G8_UINT;
    case AGPUVertexFormat_UChar4:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case AGPUVertexFormat_Char2:
        return DXGI_FORMAT_R8G8_SINT;
    case AGPUVertexFormat_Char4:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case AGPUVertexFormat_UChar2Norm:
        return DXGI_FORMAT_R8G8_UNORM;
    case AGPUVertexFormat_UChar4Norm:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case AGPUVertexFormat_Char2Norm:
        return DXGI_FORMAT_R8G8_SNORM;
    case AGPUVertexFormat_Char4Norm:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case AGPUVertexFormat_UShort2:
        return DXGI_FORMAT_R16G16_UINT;
    case AGPUVertexFormat_UShort4:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case AGPUVertexFormat_Short2:
        return DXGI_FORMAT_R16G16_SINT;
    case AGPUVertexFormat_Short4:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case AGPUVertexFormat_UShort2Norm:
        return DXGI_FORMAT_R16G16_UNORM;
    case AGPUVertexFormat_UShort4Norm:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case AGPUVertexFormat_Short2Norm:
        return DXGI_FORMAT_R16G16_SNORM;
    case AGPUVertexFormat_Short4Norm:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
    case AGPUVertexFormat_Half2:
        return DXGI_FORMAT_R16G16_FLOAT;
    case AGPUVertexFormat_Half4:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case AGPUVertexFormat_Float:
        return DXGI_FORMAT_R32_FLOAT;
    case AGPUVertexFormat_Float2:
        return DXGI_FORMAT_R32G32_FLOAT;
    case AGPUVertexFormat_Float3:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case AGPUVertexFormat_Float4:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case AGPUVertexFormat_UInt:
        return DXGI_FORMAT_R32_UINT;
    case AGPUVertexFormat_UInt2:
        return DXGI_FORMAT_R32G32_UINT;
    case AGPUVertexFormat_UInt3:
        return DXGI_FORMAT_R32G32B32_UINT;
    case AGPUVertexFormat_UInt4:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case AGPUVertexFormat_Int:
        return DXGI_FORMAT_R32_SINT;
    case AGPUVertexFormat_Int2:
        return DXGI_FORMAT_R32G32_SINT;
    case AGPUVertexFormat_Int3:
        return DXGI_FORMAT_R32G32B32_SINT;
    case AGPUVertexFormat_Int4:
        return DXGI_FORMAT_R32G32B32A32_SINT;

    default:
        _VGPU_UNREACHABLE();
    }
}

static D3D_PRIMITIVE_TOPOLOGY d3d_GetPrimitiveTopology(AGPUPrimitiveTopology topology)
{
    switch (topology)
    {
    case AGPUPrimitiveTopology_PointList:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case AGPUPrimitiveTopology_LineList:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case AGPUPrimitiveTopology_LineStrip:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case AGPUPrimitiveTopology_TriangleList:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case AGPUPrimitiveTopology_TriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:
        _VGPU_UNREACHABLE();
    }
}

typedef enum AGPUFactoryCaps {
    AGPU_FACTORY_FLIP_PRESENT = (1 << 0),
    AGPU_FACTORY_TEARING = (1 << 1),
} AGPUFactoryCaps;

static inline IDXGISwapChain1* agpu_d3d_createSwapChain(
    IDXGIFactory2* dxgiFactory,
    IUnknown* deviceOrCommandQueue,
    uint32_t backBufferCount,
    uint32_t caps,
    const AGPUSwapChainDescriptor* info)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window = (HWND)info->nativeHandle;
    if (!IsWindow(window)) {
        agpuLog(AGPULogLevel_Error, "Invalid HWND handle");
        return NULL;
    }
#else
    IUnknown* window = (IUnknown*)info->nativeHandle;
#endif

    UINT flags = 0;

    if (caps & AGPU_FACTORY_TEARING)
    {
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    const DXGI_FORMAT dxgiFormat = d3d_GetSwapChainFormat(info->colorFormat);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
    DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if (!(caps & AGPU_FACTORY_FLIP_PRESENT))
    {
        swapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }
#else
    DXGI_SCALING scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
    DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = info->width;
    swapChainDesc.Height = info->height;
    swapChainDesc.Format = dxgiFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = backBufferCount;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = scaling;
    swapChainDesc.SwapEffect = swapEffect;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags = flags;

    IDXGISwapChain1* result = NULL;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = TRUE;

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

    IDXGIDevice3* dxgiDevice;
    VHR(IUnknown_QueryInterface(deviceOrCommandQueue, &agpu_IID_IDXGIDevice3, (void**)&dxgiDevice));
    VHR(IDXGIDevice3_SetMaximumFrameLatency(dxgiDevice, 1));
#endif

    return result;
}
