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

#pragma once

#include "AlimerConfig.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "Graphics/Types.h"

#if ALIMER_PLATFORM_WINDOWS
#include "Platform/Win32/WindowsPlatform.h"
#else
#define NOMINMAX
#endif

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <wrl/client.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

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

    template<typename T> void SafeRelease(T*& resource)
    {
        if (resource != nullptr) {
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

#ifdef ALIMER_ENABLE_ASSERT
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
        for (size_t i = 0; i < errorString.length(); ++i)
        {
            message.append(1, static_cast<char>(errorString[i]));
        }

        return message;
    }
#endif

#ifndef ALIMER_ENABLE_ASSERT
#define ThrowIfFailed(hr) if (FAILED(hr)) { ALIMER_ASSERT_FAIL("Failure with HRESULT of %08X", static_cast<unsigned int>(hr)); }
#else
#define ThrowIfFailed(x)                                                           \
    do                                                                      \
    {                                                                       \
        HRESULT hr_ = x;                                                    \
        ALIMER_ASSERT_MSG(SUCCEEDED(hr_), GetDXErrorStringAnsi(hr_).c_str());      \
    }                                                                       \
    while(0)
#endif

    struct DxgiFormatDesc
    {
        PixelFormat format;
        DXGI_FORMAT dxgiFormat;
    };

    extern const DxgiFormatDesc kDxgiFormatDesc[];

    static constexpr DXGI_FORMAT ToDXGIFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kDxgiFormatDesc[(uint32_t)format].format == format);
        return kDxgiFormatDesc[(uint32_t)format].dxgiFormat;
    }

    static inline DXGI_FORMAT GetTypelessFormatFromDepthFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::Depth16Unorm:
            return DXGI_FORMAT_R16_TYPELESS;
        case PixelFormat::Depth24UnormStencil8:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case PixelFormat::Depth32Float:
            return DXGI_FORMAT_R32_TYPELESS;
        default:
            ALIMER_ASSERT(IsDepthFormat(format) == false);
            return ToDXGIFormat(format);
        }
    }

    static inline DXGI_FORMAT ToDXGIFormatWitUsage(PixelFormat format, TextureUsage usage)
    {
        // If depth and either ua or sr, set to typeless
        if (IsDepthStencilFormat(format)
            && ((usage & (TextureUsage::Sampled | TextureUsage::Storage)) != TextureUsage::None))
        {
            return GetTypelessFormatFromDepthFormat(format);
        }

        return ToDXGIFormat(format);
    }

    enum class DXGIFactoryCaps : uint8
    {
        None = 0,
        FlipPresent = (1 << 0),
        HDR = (1 << 1),
        Tearing = (1 << 2)
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(DXGIFactoryCaps, uint8);

    void DXGISetObjectName(IDXGIObject* obj, const std::string& name);
}
