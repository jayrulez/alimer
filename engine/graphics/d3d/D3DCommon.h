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

#include "core/Assert.h"
#include "core/Log.h"
#include "graphics/PixelFormat.h"

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

#include <d3dcommon.h>
#include <dxgiformat.h>

#if defined(NTDDI_WIN10_RS2)
    #include <dxgi1_6.h>
#else
    #include <dxgi1_5.h>
#endif

#if defined(_DEBUG)
    #include <dxgidebug.h>
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE)(UINT flags, REFIID _riid, void** _debug);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_CREATE_DXGI_FACTORY CreateDXGIFactory1;
    extern PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    extern PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
#endif

#if defined(_DEBUG)
    // Declare debug guids to avoid linking with "dxguid.lib"
    static constexpr GUID g_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
    static constexpr GUID g_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

    void WINAPI DXGetErrorDescriptionW(_In_ HRESULT hr, _Out_cap_(count) wchar_t* desc, _In_ size_t count);

    template <typename T>
    void SafeRelease(T& resource)
    {
        if (resource)
        {
            resource->Release();
            resource = nullptr;
        }
    }

    inline std::wstring GetDXErrorString(HRESULT hr)
    {
        const uint32_t errStringSize = 1024;
        wchar_t errorString[errStringSize];
        DXGetErrorDescriptionW(hr, errorString, errStringSize);

        std::wstring message = L"DirectX Error: ";
        message += errorString;
        return message;
    }

    inline std::string GetDXErrorStringAnsi(HRESULT hr)
    {
        std::wstring errorString = GetDXErrorString(hr);

        std::string message;
        for (size_t i = 0; i < errorString.length(); ++i) {
            message.append(1, static_cast<char>(errorString[i]));
        }

        return message;
    }

    static inline DXGI_FORMAT ToDXGISwapChainFormat(PixelFormat format) {
        switch (format)
        {
        case PixelFormat::BGRA8UNorm:
        case PixelFormat::BGRA8UNormSrgb:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case PixelFormat::RGBA8UNorm:
        case PixelFormat::RGBA8UNormSrgb:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case PixelFormat::RGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case PixelFormat::RGB10A2UNorm:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        default:
            //ALIMER_LOGE("PixelFormat (%u) is not supported for creating swapchain buffer", (uint32_t)format);
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    static inline DXGI_FORMAT ToDXGIFormat(PixelFormat format)
    {
        static DXGI_FORMAT formats[(unsigned)PixelFormat::Count] = {
            DXGI_FORMAT_UNKNOWN,
            // 8-bit pixel formats
            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_R8_SNORM,
            DXGI_FORMAT_R8_UINT,
            DXGI_FORMAT_R8_SINT,

            // 16-bit pixel formats
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_R16_SNORM,
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
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R16G16_SNORM,
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
            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R16G16B16A16_SNORM,
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

            // Compressed PVRTC Pixel Formats
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,

            // Compressed ETC Pixel Formats
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,

            // Compressed ASTC Pixel Formats
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN
        };

        static_assert((unsigned)PixelFormat::Count == ALIMER_COUNT_OF(formats), "Invalid PixelFormat size");
        return formats[(unsigned)format];
    }

    static inline DXGI_FORMAT ToDXGITypelessDepthFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::Depth16UNorm:
            return DXGI_FORMAT_R16_TYPELESS;
        case PixelFormat::Depth32FloatStencil8:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        case PixelFormat::Depth24UNormStencil8:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case PixelFormat::Depth32Float:
            return DXGI_FORMAT_R32_TYPELESS;
        default:
            ALIMER_ASSERT(IsDepthFormat(format) == false);
            return ToDXGIFormat(format);
        }
    }

    static inline std::string D3DFeatureLevelToVersion(D3D_FEATURE_LEVEL featureLevel)
    {
        switch (featureLevel)
        {
        case D3D_FEATURE_LEVEL_12_1:    return "12.1";
        case D3D_FEATURE_LEVEL_12_0:    return "12.0";
        case D3D_FEATURE_LEVEL_11_1:    return "11.1";
        case D3D_FEATURE_LEVEL_11_0:    return "11.0";
        case D3D_FEATURE_LEVEL_10_1:    return "10.1";
        case D3D_FEATURE_LEVEL_10_0:    return "10.0";
        case D3D_FEATURE_LEVEL_9_3:     return "9.3";
        case D3D_FEATURE_LEVEL_9_2:     return "9.2";
        case D3D_FEATURE_LEVEL_9_1:     return "9.1";
        }

        return "";
    }

    /// Calculate size taking into account alignment. Alignment must be a power of 2
    static inline uint32_t alignTo(uint32_t size, uint32_t alignment) { return (size + alignment - 1) & ~(alignment - 1); }
    static inline uint64_t alignTo(uint64_t size, uint64_t alignment) { return (size + alignment - 1) & ~(alignment - 1); }
}

#if ALIMER_ENABLE_ASSERT

#define ThrowIfFailed(x)                                                    \
    do                                                                      \
    {                                                                       \
        HRESULT hr_ = x;                                                    \
        ALIMER_ASSERT_MSG(SUCCEEDED(hr_), alimer::GetDXErrorStringAnsi(hr_).c_str());      \
    }                                                                       \
    while(0)
#else

// Throws a DXException on failing HRESULT
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) {
        static char s_str[64] = {};
        sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(hr));
        ALIMER_LOGERROR(s_str);
    }
}

#endif
