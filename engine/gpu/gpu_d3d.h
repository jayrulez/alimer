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

#include "gpu.h"
#include <dxgi.h>

static DXGI_FORMAT _vgpu_d3d_get_format(GPUTextureFormat format) {
    static DXGI_FORMAT formats[_GPU_PIXEL_FORMAT_COUNT] = {
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

static DXGI_FORMAT _vgpu_d3d_get_typeless_format(GPUTextureFormat format)
{
    switch (format)
    {
    case GPU_TEXTURE_FORMAT_DEPTH16_UNORM:
        return DXGI_FORMAT_R16_TYPELESS;
    case GPU_TEXTURE_FORMAT_DEPTH32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case GPU_TEXTURE_FORMAT_DEPTH24_PLUS:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case GPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
        return DXGI_FORMAT_R32_TYPELESS;
    default:
        assert(!vgpu_is_depth_format(format));
        return _vgpu_d3d_get_format(format);
    }
}

static DXGI_FORMAT _vgpu_d3d_get_texture_format(GPUTextureFormat format, GPUTextureUsage usage)
{
    if (vgpu_is_depth_format(format) &&
        (usage & GPU_TEXTURE_USAGE_SAMPLED | GPU_TEXTURE_USAGE_STORAGE) != GPU_TEXTURE_USAGE_NONE)
    {
        return _vgpu_d3d_get_typeless_format(format);
    }

    return _vgpu_d3d_get_format(format);
}

static DXGI_FORMAT _vgpu_d3d_swapchain_pixel_format(GPUTextureFormat format) {
    switch (format)
    {
    case GPU_TEXTURE_FORMAT_UNDEFINED:
    case GPU_TEXTURE_FORMAT_BGRA8_UNORM:
    case GPU_TEXTURE_FORMAT_BGRA8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM;

    case GPU_TEXTURE_FORMAT_RGBA8_UNORM:
    case GPU_TEXTURE_FORMAT_RGBA8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case GPU_TEXTURE_FORMAT_RGBA16_FLOAT:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;

    case GPU_TEXTURE_FORMAT_RGB10A2_UNORM:
        return DXGI_FORMAT_R10G10B10A2_UNORM;

    default:
        //vgpu_log_error_format("PixelFormat (%u) is not supported for creating swapchain buffer", (uint32_t)format);
        return DXGI_FORMAT_UNKNOWN;
    }
}

static DXGI_USAGE d3d_GetSwapChainBufferUsage(GPUTextureUsage textureUsage) {
    DXGI_USAGE usage = 0;
    if (textureUsage & GPU_TEXTURE_USAGE_SAMPLED) {
        usage |= DXGI_USAGE_SHADER_INPUT;
    }
    if (textureUsage & GPU_TEXTURE_USAGE_STORAGE) {
        usage |= DXGI_USAGE_UNORDERED_ACCESS;
    }
    if (textureUsage & GPU_TEXTURE_USAGE_OUTPUT_ATTACHMENT) {
        usage |= DXGI_USAGE_RENDER_TARGET_OUTPUT;
    }
    return usage;
}

static UINT d3d_GetSyncInterval(GPUPresentMode mode)
{
    switch (mode)
    {
    case GPUPresentMode_Immediate:
        return 0;

    case GPUPresentMode_Mailbox:
        return 2;

    case GPUPresentMode_Fifo:
    default:
        return 1;
    }
}

static D3D_PRIMITIVE_TOPOLOGY d3d_GetPrimitiveTopology(GPUPrimitiveTopology topology)
{
    switch (topology)
    {
    case GPUPrimitiveTopology_PointList:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case GPUPrimitiveTopology_LineList:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case GPUPrimitiveTopology_LineStrip:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case GPUPrimitiveTopology_TriangleList:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case GPUPrimitiveTopology_TriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:
        _VGPU_UNREACHABLE();
    }
}
