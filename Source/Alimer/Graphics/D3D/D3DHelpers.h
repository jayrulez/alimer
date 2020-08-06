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

#include "config.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "Graphics/Types.h"

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

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <wrl/client.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#if !defined(ALIMER_D3D12) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, _COM_Outptr_ void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

namespace alimer
{
#if !defined(ALIMER_D3D12) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    extern PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
#endif

    template<typename T> void SafeRelease(T*& resource)
    {
        if (resource != nullptr) {
            resource->Release();
            resource = nullptr;
        }
    }

#ifdef _DEBUG
    // Declare Guids to avoid linking with "dxguid.lib"
    static constexpr GUID g_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8 } };
    static constexpr GUID g_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a } };
    static constexpr GUID g_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
#endif

#ifdef ALIMER_ENABLE_ASSERT
    void WINAPI DXGetErrorDescriptionW(_In_ HRESULT hr, _Out_cap_(count) wchar_t* desc, _In_ size_t count);

    inline eastl::wstring GetDXErrorString(HRESULT hr)
    {
        const uint32_t errStringSize = 1024;
        wchar_t errorString[errStringSize];
        DXGetErrorDescriptionW(hr, errorString, errStringSize);

        eastl::wstring message = L"DirectX Error: ";
        message += errorString;
        return message;
    }

    inline eastl::string GetDXErrorStringAnsi(HRESULT hr)
    {
        eastl::wstring errorString = GetDXErrorString(hr);

        eastl::string message;
        for (eastl_size_t i = 0; i < errorString.length(); ++i)
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

    static inline eastl::string ToUtf8(const eastl::wstring& str)
    {
        eastl::vector<char> char_buffer;
        auto ret = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0, nullptr, nullptr);
        if (ret < 0)
            return "";
        char_buffer.resize(ret);
        WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), char_buffer.data(), static_cast<int>(char_buffer.size()), nullptr, nullptr);
        return eastl::string(char_buffer.data(), char_buffer.size());
    }

    static inline eastl::wstring ToUtf16(const eastl::string& str)
    {
        eastl::vector<wchar_t> wchar_buffer;
        auto ret = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
        if (ret < 0)
            return L"";
        wchar_buffer.resize(ret);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), wchar_buffer.data(), static_cast<int>(wchar_buffer.size()));
        return eastl::wstring(wchar_buffer.data(), wchar_buffer.size());
    }

    struct DxgiFormatDesc
    {
        PixelFormat format;
        DXGI_FORMAT dxgiFormat;
    };

    extern const DxgiFormatDesc kDxgiFormatDesc[];

    static inline DXGI_FORMAT ToDXGIFormat(PixelFormat format)
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

    enum class DXGIFactoryCaps : uint8_t
    {
        None = 0,
        FlipPresent = (1 << 0),
        Tearing = (1 << 1),
        HDR = (1 << 2)
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(DXGIFactoryCaps, uint8_t);

    static inline IDXGISwapChain1* DXGICreateSwapchain(
        IDXGIFactory2* dxgiFactory,
        DXGIFactoryCaps factoryCaps,
        IUnknown* deviceOrCommandQueue,
        void* windowHandle,
        uint32_t width, uint32_t height,
        PixelFormat colorFormat,
        uint32_t bufferCount,
        bool isFullscreen)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND window = (HWND)windowHandle;
        if (!IsWindow(window)) {
            LOGE("Invalid HWND handle");
            return nullptr;
        }
#else
        IUnknown* window = (IUnknown*)windowHandle;
#endif

        UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        if ((factoryCaps & DXGIFactoryCaps::Tearing) != DXGIFactoryCaps::None)
        {
            flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
        DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        if ((factoryCaps & DXGIFactoryCaps::FlipPresent) == DXGIFactoryCaps::None)
        {
            swapEffect = DXGI_SWAP_EFFECT_DISCARD;
        }
#else
        DXGI_SCALING scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = width;
        swapchainDesc.Height = height;
        swapchainDesc.Format = ToDXGIFormat(SRGBToLinearFormat(colorFormat));
        swapchainDesc.Stereo = false;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.BufferCount = bufferCount;
        swapchainDesc.Scaling = scaling;
        swapchainDesc.SwapEffect = swapEffect;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapchainDesc.Flags = flags;

        IDXGISwapChain1* result = NULL;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapchainDesc = {};
        fsSwapchainDesc.Windowed = !isFullscreen;

        // Create a SwapChain from a Win32 window.
        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            deviceOrCommandQueue,
            window,
            &swapchainDesc,
            &fsSwapchainDesc,
            nullptr,
            &result
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
        ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
            deviceOrCommandQueue,
            window,
            &swapchainDesc,
            nullptr,
            &result
        ));
#endif

        return result;
    }

    void DXGISetObjectName(IDXGIObject* obj, const String& name);
}
