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
// The implementation is based on WickedEngine graphics code, MIT license (https://github.com/turanszkij/WickedEngine/blob/master/LICENSE.md)

#pragma once

#include "AlimerConfig.h"
#include "RHI/RHITypes.h"

#if ALIMER_PLATFORM_WINDOWS
#   include "PlatformIncl.h"
#else
#   define NOMINMAX
#endif

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#ifdef _DEBUG
#   include <dxgidebug.h>
#endif

#include <wrl/client.h> 

namespace Alimer
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

    // Type alias for ComPtr template.
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    template <typename T>
    void SafeRelease(T& resource)
    {
        if (resource)
        {
            resource->Release();
            resource = nullptr;
        }
    }

#ifdef _DEBUG
    // Declare Guids to avoid linking with "dxguid.lib"
    static constexpr GUID DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8 } };
    static constexpr GUID DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a } };
    static constexpr GUID g_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
#endif

    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

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
}
