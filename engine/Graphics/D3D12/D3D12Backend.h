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
// The implementation is based on WickedEngine graphics code, MIT license
// (https://github.com/turanszkij/WickedEngine/blob/master/LICENSE.md)

#pragma once

#include "AlimerConfig.h"
#include "Graphics/Types.h"

#if ALIMER_PLATFORM_WINDOWS
    #include "PlatformIncl.h"
#else
    #define NOMINMAX
#endif

#include <d3d12.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <unordered_map>

#if defined(NTDDI_WIN10_RS2)
    #include <dxgi1_6.h>
#else
    #include <dxgi1_5.h>
#endif

#ifdef _DEBUG
    #include <dxgidebug.h>
#endif

#include <wrl/client.h>

// Forward declare memory allocator classes
namespace D3D12MA
{
    class Allocator;
    class Allocation;
};

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    // Functions from dxgi.dll
    using PFN_CREATE_DXGI_FACTORY1 = HRESULT(WINAPI*)(REFIID riid, _COM_Outptr_ void** ppFactory);
    using PFN_CREATE_DXGI_FACTORY2 = HRESULT(WINAPI*)(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);
    using PFN_DXGI_GET_DEBUG_INTERFACE1 = HRESULT(WINAPI*)(UINT Flags, REFIID riid, _COM_Outptr_ void** pDebug);

    extern PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
    extern PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    extern PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
#endif

    class D3D12GraphicsDevice;

    // Type alias for ComPtr template.
    template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    template <typename T> void SafeRelease(T& resource)
    {
        if (resource)
        {
            resource->Release();
            resource = nullptr;
        }
    }

#ifdef _DEBUG
    // Declare Guids to avoid linking with "dxguid.lib"
    static constexpr GUID DXGI_DEBUG_ALL = {
        0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8}};
    static constexpr GUID DXGI_DEBUG_DXGI = {
        0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a}};
    static constexpr GUID g_D3DDebugObjectName = {
        0x429b8c22, 0x9188, 0x4b0c, {0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00}};
#endif

    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept
            : result(hr)
        {
        }

        const char* what() const override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }

    constexpr D3D_PRIMITIVE_TOPOLOGY D3DPrimitiveTopology(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PrimitiveTopology::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LineList:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::TriangleList:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case PrimitiveTopology::PatchList:
            return D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        default:
            ALIMER_UNREACHABLE();
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        }
    }

    constexpr DXGI_FORMAT D3DConvertVertexFormat(VertexFormat format)
    {
        switch (format)
        {
        case VertexFormat::UChar2:
            return DXGI_FORMAT_R8G8_UINT;
        case VertexFormat::UChar4:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case VertexFormat::Char2:
            return DXGI_FORMAT_R8G8_SINT;
        case VertexFormat::Char4:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case VertexFormat::UChar2Norm:
            return DXGI_FORMAT_R8G8_UNORM;
        case VertexFormat::UChar4Norm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case VertexFormat::Char2Norm:
            return DXGI_FORMAT_R8G8_SNORM;
        case VertexFormat::Char4Norm:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case VertexFormat::UShort2:
            return DXGI_FORMAT_R16G16_UINT;
        case VertexFormat::UShort4:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case VertexFormat::Short2:
            return DXGI_FORMAT_R16G16_SINT;
        case VertexFormat::Short4:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case VertexFormat::UShort2Norm:
            return DXGI_FORMAT_R16G16_UNORM;
        case VertexFormat::UShort4Norm:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case VertexFormat::Short2Norm:
            return DXGI_FORMAT_R16G16_SNORM;
        case VertexFormat::Short4Norm:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case VertexFormat::Half2:
            return DXGI_FORMAT_R16G16_FLOAT;
        case VertexFormat::Half4:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case VertexFormat::Float:
            return DXGI_FORMAT_R32_FLOAT;
        case VertexFormat::Float2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case VertexFormat::Float3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case VertexFormat::Float4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case VertexFormat::UInt:
            return DXGI_FORMAT_R32_UINT;
        case VertexFormat::UInt2:
            return DXGI_FORMAT_R32G32_UINT;
        case VertexFormat::UInt3:
            return DXGI_FORMAT_R32G32B32_UINT;
        case VertexFormat::UInt4:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case VertexFormat::Int:
            return DXGI_FORMAT_R32_SINT;
        case VertexFormat::Int2:
            return DXGI_FORMAT_R32G32_SINT;
        case VertexFormat::Int3:
            return DXGI_FORMAT_R32G32B32_SINT;
        case VertexFormat::Int4:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        default:
            ALIMER_UNREACHABLE();
        }
    }

    constexpr DXGI_FORMAT PixelFormatToDXGIFormat(PixelFormat value)
    {
        switch (value)
        {
        // 8-bit formats
        case PixelFormat::R8Unorm:
            return DXGI_FORMAT_R8_UNORM;
        case PixelFormat::R8Snorm:
            return DXGI_FORMAT_R8_SNORM;
        case PixelFormat::R8Uint:
            return DXGI_FORMAT_R8_UINT;
        case PixelFormat::R8Sint:
            return DXGI_FORMAT_R8_SINT;
        // 16-bit formats
        case PixelFormat::R16Unorm:
            return DXGI_FORMAT_R16_UNORM;
        case PixelFormat::R16Snorm:
            return DXGI_FORMAT_R16_SNORM;
        case PixelFormat::R16Uint:
            return DXGI_FORMAT_R16_UINT;
        case PixelFormat::R16Sint:
            return DXGI_FORMAT_R16_SINT;
        case PixelFormat::R16Float:
            return DXGI_FORMAT_R16_FLOAT;
        case PixelFormat::RG8Unorm:
            return DXGI_FORMAT_R8G8_UNORM;
        case PixelFormat::RG8Snorm:
            return DXGI_FORMAT_R8G8_SNORM;
        case PixelFormat::RG8Uint:
            return DXGI_FORMAT_R8G8_UINT;
        case PixelFormat::RG8Sint:
            return DXGI_FORMAT_R8G8_SINT;
            // 32-bit formats
        case PixelFormat::R32Uint:
            return DXGI_FORMAT_R32_UINT;
        case PixelFormat::R32Sint:
            return DXGI_FORMAT_R32_SINT;
        case PixelFormat::R32Float:
            return DXGI_FORMAT_R32_FLOAT;
        case PixelFormat::RG16Unorm:
            return DXGI_FORMAT_R16G16_UNORM;
        case PixelFormat::RG16Snorm:
            return DXGI_FORMAT_R16G16_SNORM;
        case PixelFormat::RG16Uint:
            return DXGI_FORMAT_R16G16_UINT;
        case PixelFormat::RG16Sint:
            return DXGI_FORMAT_R16G16_SINT;
        case PixelFormat::RG16Float:
            return DXGI_FORMAT_R16G16_FLOAT;
        case PixelFormat::RGBA8Unorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::RGBA8UnormSrgb:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case PixelFormat::RGBA8Snorm:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case PixelFormat::RGBA8Uint:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case PixelFormat::RGBA8Sint:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case PixelFormat::BGRA8Unorm:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case PixelFormat::BGRA8UnormSrgb:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            // Packed 32-Bit formats
        case PixelFormat::RGB10A2Unorm:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case PixelFormat::RG11B10Float:
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case PixelFormat::RGB9E5Float:
            return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
            // 64-Bit formats
        case PixelFormat::RG32Uint:
            return DXGI_FORMAT_R32G32_UINT;
        case PixelFormat::RG32Sint:
            return DXGI_FORMAT_R32G32_SINT;
        case PixelFormat::RG32Float:
            return DXGI_FORMAT_R32G32_FLOAT;
        case PixelFormat::RGBA16Unorm:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case PixelFormat::RGBA16Snorm:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case PixelFormat::RGBA16Uint:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case PixelFormat::RGBA16Sint:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case PixelFormat::RGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
            // 128-Bit formats
        case PixelFormat::RGBA32Uint:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case PixelFormat::RGBA32Sint:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case PixelFormat::RGBA32Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
            // Depth-stencil formats
        case PixelFormat::Depth16Unorm:
            return DXGI_FORMAT_D16_UNORM;
        case PixelFormat::Depth32Float:
            return DXGI_FORMAT_D32_FLOAT;
        case PixelFormat::Depth24UnormStencil8:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case PixelFormat::Depth32FloatStencil8:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            // Compressed BC formats
        case PixelFormat::BC1RGBAUnorm:
            return DXGI_FORMAT_BC1_UNORM;
        case PixelFormat::BC1RGBAUnormSrgb:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case PixelFormat::BC2RGBAUnorm:
            return DXGI_FORMAT_BC2_UNORM;
        case PixelFormat::BC2RGBAUnormSrgb:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case PixelFormat::BC3RGBAUnorm:
            return DXGI_FORMAT_BC3_UNORM;
        case PixelFormat::BC3RGBAUnormSrgb:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case PixelFormat::BC4RSnorm:
            return DXGI_FORMAT_BC4_SNORM;
        case PixelFormat::BC4RUnorm:
            return DXGI_FORMAT_BC4_UNORM;
        case PixelFormat::BC5RGSnorm:
            return DXGI_FORMAT_BC5_SNORM;
        case PixelFormat::BC5RGUnorm:
            return DXGI_FORMAT_BC5_UNORM;
        case PixelFormat::BC6HRGBFloat:
            return DXGI_FORMAT_BC6H_SF16;
        case PixelFormat::BC6HRGBUfloat:
            return DXGI_FORMAT_BC6H_UF16;
        case PixelFormat::BC7RGBAUnorm:
            return DXGI_FORMAT_BC7_UNORM;
        case PixelFormat::BC7RGBAUnormSrgb:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        default:
            ALIMER_UNREACHABLE();
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    constexpr DXGI_FORMAT GetTypelessFormatFromDepthFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::Depth16Unorm:
            return DXGI_FORMAT_R16_TYPELESS;
        case PixelFormat::Depth32Float:
            return DXGI_FORMAT_R32_TYPELESS;
        case PixelFormat::Depth24UnormStencil8:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case PixelFormat::Depth32FloatStencil8:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

        default:
            ALIMER_ASSERT(IsDepthFormat(format) == false);
            return PixelFormatToDXGIFormat(format);
        }
    }

    constexpr PixelFormat PixelFormatFromDXGIFormat(DXGI_FORMAT value)
    {
        switch (value)
        {
        case DXGI_FORMAT_UNKNOWN:
            return PixelFormat::Undefined;
            // 8-bit formats
        case DXGI_FORMAT_R8_UNORM:
            return PixelFormat::R8Unorm;
        case DXGI_FORMAT_R8_SNORM:
            return PixelFormat::R8Snorm;
        case DXGI_FORMAT_R8_UINT:
            return PixelFormat::R8Uint;
        case DXGI_FORMAT_R8_SINT:
            return PixelFormat::R8Sint;
            // 16-bit formats
        case DXGI_FORMAT_R16_UNORM:
            return PixelFormat::R16Unorm;
        case DXGI_FORMAT_R16_SNORM:
            return PixelFormat::R16Snorm;
        case DXGI_FORMAT_R16_UINT:
            return PixelFormat::R16Uint;
        case DXGI_FORMAT_R16_SINT:
            return PixelFormat::R16Sint;
        case DXGI_FORMAT_R16_FLOAT:
            return PixelFormat::R16Float;
        case DXGI_FORMAT_R8G8_UNORM:
            return PixelFormat::RG8Unorm;
        case DXGI_FORMAT_R8G8_SNORM:
            return PixelFormat::RG8Snorm;
        case DXGI_FORMAT_R8G8_UINT:
            return PixelFormat::RG8Uint;
        case DXGI_FORMAT_R8G8_SINT:
            return PixelFormat::RG8Sint;
            // 32-bit formats
        case DXGI_FORMAT_R32_UINT:
            return PixelFormat::R32Uint;
        case DXGI_FORMAT_R32_SINT:
            return PixelFormat::R32Sint;
        case DXGI_FORMAT_R32_FLOAT:
            return PixelFormat::R32Float;
        case DXGI_FORMAT_R16G16_UNORM:
            return PixelFormat::RG16Unorm;
        case DXGI_FORMAT_R16G16_SNORM:
            return PixelFormat::RG16Snorm;
        case DXGI_FORMAT_R16G16_UINT:
            return PixelFormat::RG16Uint;
        case DXGI_FORMAT_R16G16_SINT:
            return PixelFormat::RG16Sint;
        case DXGI_FORMAT_R16G16_FLOAT:
            return PixelFormat::RG16Float;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return PixelFormat::RGBA8Unorm;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return PixelFormat::RGBA8UnormSrgb;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return PixelFormat::RGBA8Snorm;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return PixelFormat::RGBA8Uint;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return PixelFormat::RGBA8Sint;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return PixelFormat::BGRA8Unorm;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return PixelFormat::BGRA8UnormSrgb;
            // Packed 32-Bit formats
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return PixelFormat::RGB10A2Unorm;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return PixelFormat::RG11B10Float;
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return PixelFormat::RGB9E5Float;
            // 64-Bit formats
        case DXGI_FORMAT_R32G32_UINT:
            return PixelFormat::RG32Uint;
        case DXGI_FORMAT_R32G32_SINT:
            return PixelFormat::RG32Sint;
        case DXGI_FORMAT_R32G32_FLOAT:
            return PixelFormat::RG32Float;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return PixelFormat::RGBA16Unorm;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return PixelFormat::RGBA16Snorm;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return PixelFormat::RGBA16Uint;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return PixelFormat::RGBA16Sint;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return PixelFormat::RGBA16Float;
            // 128-Bit formats
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return PixelFormat::RGBA32Uint;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return PixelFormat::RGBA32Sint;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return PixelFormat::RGBA32Float;
            // Depth-stencil formats
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_TYPELESS:
            return PixelFormat::Depth16Unorm;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_TYPELESS:
            return PixelFormat::Depth32Float;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
            return PixelFormat::Depth24UnormStencil8;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return PixelFormat::Depth32FloatStencil8;
            // Compressed BC formats
        case DXGI_FORMAT_BC1_UNORM:
            return PixelFormat::BC1RGBAUnorm;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return PixelFormat::BC1RGBAUnormSrgb;
        case DXGI_FORMAT_BC2_UNORM:
            return PixelFormat::BC2RGBAUnorm;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return PixelFormat::BC2RGBAUnormSrgb;
        case DXGI_FORMAT_BC3_UNORM:
            return PixelFormat::BC3RGBAUnorm;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return PixelFormat::BC3RGBAUnormSrgb;
        case DXGI_FORMAT_BC4_SNORM:
            return PixelFormat::BC4RSnorm;
        case DXGI_FORMAT_BC4_UNORM:
            return PixelFormat::BC4RUnorm;
        case DXGI_FORMAT_BC5_SNORM:
            return PixelFormat::BC5RGSnorm;
        case DXGI_FORMAT_BC5_UNORM:
            return PixelFormat::BC5RGUnorm;
        case DXGI_FORMAT_BC6H_SF16:
            return PixelFormat::BC6HRGBFloat;
        case DXGI_FORMAT_BC6H_UF16:
            return PixelFormat::BC6HRGBUfloat;
        case DXGI_FORMAT_BC7_UNORM:
            return PixelFormat::BC7RGBAUnorm;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return PixelFormat::BC7RGBAUnormSrgb;

        default:
            ALIMER_UNREACHABLE();
            return PixelFormat::Undefined;
        }
    }

    constexpr D3D12_RESOURCE_STATES _ConvertImageLayout(IMAGE_LAYOUT value)
    {
        switch (value)
        {
        case IMAGE_LAYOUT_UNDEFINED:
        case IMAGE_LAYOUT_GENERAL:
            return D3D12_RESOURCE_STATE_COMMON;
        case IMAGE_LAYOUT_RENDERTARGET:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case IMAGE_LAYOUT_DEPTHSTENCIL:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case IMAGE_LAYOUT_DEPTHSTENCIL_READONLY:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case IMAGE_LAYOUT_SHADER_RESOURCE:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case IMAGE_LAYOUT_UNORDERED_ACCESS:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case IMAGE_LAYOUT_COPY_SRC:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case IMAGE_LAYOUT_COPY_DST:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case IMAGE_LAYOUT_SHADING_RATE_SOURCE:
            return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
        }

        return D3D12_RESOURCE_STATE_COMMON;
    }
}
