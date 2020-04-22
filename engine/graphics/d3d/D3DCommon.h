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
#include "graphics/Types.h"
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
#   include <dxgidebug.h>

#   if !defined(_XBOX_ONE) || !defined(_TITLE)
#   pragma comment(lib,"dxguid.lib")
#   endif

#endif

#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

namespace alimer
{
    void WINAPI DXGetErrorDescriptionW(_In_ HRESULT hr, _Out_cap_(count) wchar_t* desc, _In_ size_t count);

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

    struct DxgiFormatDesc
    {
        PixelFormat format;
        DXGI_FORMAT dxgiFormat;
    };

    extern const DxgiFormatDesc kDxgiFormatDesc[];

    static inline DXGI_FORMAT ToDXGISwapChainFormat(PixelFormat format) {
        switch (format)
        {
        case PixelFormat::BGRA8Unorm:
        case PixelFormat::BGRA8UnormSrgb:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case PixelFormat::RGBA8Unorm:
        case PixelFormat::RGBA8UnormSrgb:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case PixelFormat::RGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case PixelFormat::RGB10A2Unorm:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        default:
            ALIMER_LOGE("PixelFormat (%u) is not supported for creating swapchain buffer", (uint32_t)format);
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    static inline DXGI_FORMAT ToDXGIFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kDxgiFormatDesc[(uint32_t)format].format == format);
        return kDxgiFormatDesc[(uint32_t)format].dxgiFormat;
    }

    static inline DXGI_FORMAT ToDXGITypelessDepthFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::D16Unorm:
            return DXGI_FORMAT_R16_TYPELESS;
        case PixelFormat::D32FloatS8X24:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        case PixelFormat::D24UnormS8:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case PixelFormat::D32Float:
            return DXGI_FORMAT_R32_TYPELESS;
        default:
            ALIMER_ASSERT(IsDepthFormat(format) == false);
            return ToDXGIFormat(format);
        }
    }

    static inline DXGI_FORMAT ToDXGIFormatWithUsage(PixelFormat format, TextureUsage usage)
    {
        if (IsDepthFormat(format) &&
            (any(usage & TextureUsage::Sampled | TextureUsage::Storage)))
        {
            return ToDXGITypelessDepthFormat(format);
        }

        return ToDXGIFormat(format);
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
