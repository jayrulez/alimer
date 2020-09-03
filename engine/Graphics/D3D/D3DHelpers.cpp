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

#include "D3DHelpers.h"

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
#endif

#ifdef ALIMER_ENABLE_ASSERT
#define CHK_ERRA(hrchk) case hrchk: wcscpy_s( desc, count, L#hrchk )
#define CHK_ERR(hrchk, strOut) case hrchk: wcscpy_s( desc, count, L##strOut )

    void WINAPI DXGetErrorDescriptionW(_In_ HRESULT hr, _Out_cap_(count) wchar_t* desc, _In_ size_t count)
    {
        if (!count)
            return;

        *desc = 0;

        // First try to see if FormatMessage knows this hr
        LPWSTR errorText = nullptr;

        DWORD result = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&errorText, 0, nullptr);

        if (result > 0 && errorText)
        {
            wcscpy_s(desc, count, errorText);

            if (errorText)
                LocalFree(errorText);

            return;
        }

        if (errorText)
            LocalFree(errorText);

        switch (hr)
        {
            // Commmented out codes are actually alises for other codes

            // -------------------------------------------------------------
            // dxgi.h error codes
            // -------------------------------------------------------------
            CHK_ERR(DXGI_STATUS_OCCLUDED, "The target window or output has been occluded. The application should suspend rendering operations if possible.");
            CHK_ERR(DXGI_STATUS_CLIPPED, "Target window is clipped.");
            CHK_ERR(DXGI_STATUS_NO_REDIRECTION, "");
            CHK_ERR(DXGI_STATUS_NO_DESKTOP_ACCESS, "No access to desktop.");
            CHK_ERR(DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, "");
            CHK_ERR(DXGI_STATUS_MODE_CHANGED, "Display mode has changed");
            CHK_ERR(DXGI_STATUS_MODE_CHANGE_IN_PROGRESS, "Display mode is changing");
            CHK_ERR(DXGI_ERROR_INVALID_CALL, "The application has made an erroneous API call that it had enough information to avoid. This error is intended to denote that the application should be altered to avoid the error. Use of the debug version of the DXGI.DLL will provide run-time debug output with further information.");
            CHK_ERR(DXGI_ERROR_NOT_FOUND, "The item requested was not found. For GetPrivateData calls, this means that the specified GUID had not been previously associated with the object.");
            CHK_ERR(DXGI_ERROR_MORE_DATA, "The specified size of the destination buffer is too small to hold the requested data.");
            CHK_ERR(DXGI_ERROR_UNSUPPORTED, "Unsupported.");
            CHK_ERR(DXGI_ERROR_DEVICE_REMOVED, "Hardware device removed.");
            CHK_ERR(DXGI_ERROR_DEVICE_HUNG, "Device hung due to badly formed commands.");
            CHK_ERR(DXGI_ERROR_DEVICE_RESET, "Device reset due to a badly formed commant.");
            CHK_ERR(DXGI_ERROR_WAS_STILL_DRAWING, "Was still drawing.");
            CHK_ERR(DXGI_ERROR_FRAME_STATISTICS_DISJOINT, "The requested functionality is not supported by the device or the driver.");
            CHK_ERR(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE, "The requested functionality is not supported by the device or the driver.");
            CHK_ERR(DXGI_ERROR_DRIVER_INTERNAL_ERROR, "An internal driver error occurred.");
            CHK_ERR(DXGI_ERROR_NONEXCLUSIVE, "The application attempted to perform an operation on an DXGI output that is only legal after the output has been claimed for exclusive owenership.");
            CHK_ERR(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "The requested functionality is not supported by the device or the driver.");
            CHK_ERR(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED, "Remote desktop client disconnected.");
            CHK_ERR(DXGI_ERROR_REMOTE_OUTOFMEMORY, "Remote desktop client is out of memory.");

            // -------------------------------------------------------------
            // d3d11.h error codes
            // -------------------------------------------------------------
            CHK_ERR(D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS, "There are too many unique state objects.");
            CHK_ERR(D3D11_ERROR_FILE_NOT_FOUND, "File not found");
            CHK_ERR(D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS, "Therea are too many unique view objects.");
            CHK_ERR(D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD, "Deferred context requires Map-Discard usage pattern");

            // -------------------------------------------------------------
            // WIC error codes
            // -------------------------------------------------------------
            CHK_ERR(WINCODEC_ERR_WRONGSTATE, "WIC object in incorrect state.");
            CHK_ERR(WINCODEC_ERR_VALUEOUTOFRANGE, "WIC Value out of range.");
            CHK_ERR(WINCODEC_ERR_UNKNOWNIMAGEFORMAT, "Encountered unexpected value or setting in WIC image format.");
            CHK_ERR(WINCODEC_ERR_UNSUPPORTEDVERSION, "Unsupported WINCODEC_SD_VERSION passed to WIC factory.");
            CHK_ERR(WINCODEC_ERR_NOTINITIALIZED, "WIC component not initialized.");
            CHK_ERR(WINCODEC_ERR_ALREADYLOCKED, "WIC bitmap object already locked.");
            CHK_ERR(WINCODEC_ERR_PROPERTYNOTFOUND, "WIC property not found.");
            CHK_ERR(WINCODEC_ERR_PROPERTYNOTSUPPORTED, "WIC property not supported.");
            CHK_ERR(WINCODEC_ERR_PROPERTYSIZE, "Invalid property size");
            CHK_ERRA(WINCODEC_ERR_CODECPRESENT); // not currently used by WIC
            CHK_ERRA(WINCODEC_ERR_CODECNOTHUMBNAIL); // not currently used by WIC
            CHK_ERR(WINCODEC_ERR_PALETTEUNAVAILABLE, "Required palette data not available.");
            CHK_ERR(WINCODEC_ERR_CODECTOOMANYSCANLINES, "More scanlines requested than are available in WIC bitmap.");
            CHK_ERR(WINCODEC_ERR_INTERNALERROR, "Unexpected internal error in WIC.");
            CHK_ERR(WINCODEC_ERR_SOURCERECTDOESNOTMATCHDIMENSIONS, "Source WIC rectangle does not match bitmap dimensions.");
            CHK_ERR(WINCODEC_ERR_COMPONENTNOTFOUND, "WIC component not found.");
            CHK_ERR(WINCODEC_ERR_IMAGESIZEOUTOFRANGE, "Image size beyond expected boundaries for WIC codec.");
            CHK_ERR(WINCODEC_ERR_TOOMUCHMETADATA, "Image metadata size beyond expected boundaries for WIC codec.");
            CHK_ERR(WINCODEC_ERR_BADIMAGE, "WIC image is corrupted.");
            CHK_ERR(WINCODEC_ERR_BADHEADER, "Invalid header found in WIC image.");
            CHK_ERR(WINCODEC_ERR_FRAMEMISSING, "Expected bitmap frame data not found in WIC image.");
            CHK_ERR(WINCODEC_ERR_BADMETADATAHEADER, "Invalid metadata header found in WIC image.");
            CHK_ERR(WINCODEC_ERR_BADSTREAMDATA, "Invalid stream data found in WIC image.");
            CHK_ERR(WINCODEC_ERR_STREAMWRITE, "WIC operation on write stream failed.");
            CHK_ERR(WINCODEC_ERR_STREAMREAD, "WIC operation on read stream failed.");
            CHK_ERR(WINCODEC_ERR_STREAMNOTAVAILABLE, "Required stream is not available.");
            CHK_ERR(WINCODEC_ERR_UNSUPPORTEDOPERATION, "This operation is not supported by WIC.");
            CHK_ERR(WINCODEC_ERR_INVALIDREGISTRATION, "Error occurred reading WIC codec registry keys.");
            CHK_ERR(WINCODEC_ERR_COMPONENTINITIALIZEFAILURE, "Failed initializing WIC codec.");
            CHK_ERR(WINCODEC_ERR_INSUFFICIENTBUFFER, "Not enough buffer space available for WIC operation.");
            CHK_ERR(WINCODEC_ERR_DUPLICATEMETADATAPRESENT, "Duplicate metadata detected in WIC image.");
            CHK_ERR(WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE, "Unexpected property type in WIC image.");
            CHK_ERR(WINCODEC_ERR_UNEXPECTEDSIZE, "Unexpected value size in WIC metadata.");
            CHK_ERR(WINCODEC_ERR_INVALIDQUERYREQUEST, "Invalid metadata query.");
            CHK_ERR(WINCODEC_ERR_UNEXPECTEDMETADATATYPE, "Unexpected metadata type encountered in WIC image.");
            CHK_ERR(WINCODEC_ERR_REQUESTONLYVALIDATMETADATAROOT, "Operation only valid on meatadata root.");
            CHK_ERR(WINCODEC_ERR_INVALIDQUERYCHARACTER, "Invalid character in WIC metadata query.");
            CHK_ERR(WINCODEC_ERR_WIN32ERROR, "General Win32 error encountered during WIC operation.");
            CHK_ERR(WINCODEC_ERR_INVALIDPROGRESSIVELEVEL, "Invalid level for progressive WIC image decode.");
        }
    }

#undef CHK_ERR
#undef CHK_ERRA
#endif

    const DxgiFormatDesc kDxgiFormatDesc[] =
    {
        {PixelFormat::Invalid,                      DXGI_FORMAT_UNKNOWN},
        // 8-bit pixel formats
        {PixelFormat::R8Unorm,                      DXGI_FORMAT_R8_UNORM},
        {PixelFormat::R8Snorm,                      DXGI_FORMAT_R8_SNORM},
        {PixelFormat::R8Uint,                       DXGI_FORMAT_R8_UINT},
        {PixelFormat::R8Sint,                       DXGI_FORMAT_R8_SINT},
        // 16-bit pixel formats
        {PixelFormat::R16Unorm,                     DXGI_FORMAT_R16_UNORM},
        {PixelFormat::R16Snorm,                     DXGI_FORMAT_R16_SNORM},
        {PixelFormat::R16Uint,                      DXGI_FORMAT_R16_UINT},
        {PixelFormat::R16Sint,                      DXGI_FORMAT_R16_SINT},
        {PixelFormat::R16Float,                     DXGI_FORMAT_R16_FLOAT},
        {PixelFormat::RG8Unorm,                     DXGI_FORMAT_R8G8_UNORM},
        {PixelFormat::RG8Snorm,                     DXGI_FORMAT_R8G8_SNORM},
        {PixelFormat::RG8Uint,                      DXGI_FORMAT_R8G8_UINT},
        {PixelFormat::RG8Sint,                      DXGI_FORMAT_R8G8_SINT},
        // 32-bit pixel formats
        {PixelFormat::R32Uint,                      DXGI_FORMAT_R32_UINT},
        {PixelFormat::R32Sint,                      DXGI_FORMAT_R32_SINT},
        {PixelFormat::R32Float,                     DXGI_FORMAT_R32_FLOAT},
        {PixelFormat::RG16Unorm,                    DXGI_FORMAT_R16G16_UNORM},
        {PixelFormat::RG16Snorm,                    DXGI_FORMAT_R16G16_SNORM},
        {PixelFormat::RG16Uint,                     DXGI_FORMAT_R16G16_UINT},
        {PixelFormat::RG16Sint,                     DXGI_FORMAT_R16G16_SINT},
        {PixelFormat::RG16Float,                    DXGI_FORMAT_R16G16_FLOAT},
        {PixelFormat::RGBA8Unorm,                   DXGI_FORMAT_R8G8B8A8_UNORM},
        {PixelFormat::RGBA8UnormSrgb,               DXGI_FORMAT_R8G8B8A8_UNORM_SRGB},
        {PixelFormat::RGBA8Snorm,                   DXGI_FORMAT_R8G8B8A8_SNORM},
        {PixelFormat::RGBA8Uint,                    DXGI_FORMAT_R8G8B8A8_UINT},
        {PixelFormat::RGBA8Sint,                    DXGI_FORMAT_R8G8B8A8_SINT},
        {PixelFormat::BGRA8Unorm,                   DXGI_FORMAT_B8G8R8A8_UNORM},
        {PixelFormat::BGRA8UnormSrgb,               DXGI_FORMAT_B8G8R8A8_UNORM_SRGB},
        // Packed 32-Bit Pixel formats
        {PixelFormat::RGB10A2Unorm,                 DXGI_FORMAT_R10G10B10A2_UNORM},
        {PixelFormat::RG11B10Float,                 DXGI_FORMAT_R11G11B10_FLOAT},
        // 64-Bit Pixel Formats
        {PixelFormat::RG32Uint,                     DXGI_FORMAT_R32G32_UINT},
        {PixelFormat::RG32Sint,                     DXGI_FORMAT_R32G32_SINT},
        {PixelFormat::RG32Float,                    DXGI_FORMAT_R32G32_FLOAT},
        {PixelFormat::RGBA16Unorm,                  DXGI_FORMAT_R16G16B16A16_UNORM},
        {PixelFormat::RGBA16Snorm,                  DXGI_FORMAT_R16G16B16A16_SNORM},
        {PixelFormat::RGBA16Uint,                   DXGI_FORMAT_R16G16B16A16_UINT},
        {PixelFormat::RGBA16Sint,                   DXGI_FORMAT_R16G16B16A16_SINT},
        {PixelFormat::RGBA16Float,                  DXGI_FORMAT_R16G16B16A16_FLOAT},
        // 128-Bit Pixel Formats
        {PixelFormat::RGBA32Uint,                   DXGI_FORMAT_R32G32B32A32_UINT},
        {PixelFormat::RGBA32Sint,                   DXGI_FORMAT_R32G32B32A32_SINT},
        {PixelFormat::RGBA32Float,                  DXGI_FORMAT_R32G32B32A32_FLOAT},
        // Depth-stencil formats
        {PixelFormat::Depth16Unorm,                 DXGI_FORMAT_D16_UNORM},
        {PixelFormat::Depth32Float,                 DXGI_FORMAT_D32_FLOAT},
        {PixelFormat::Depth24UnormStencil8,         DXGI_FORMAT_D24_UNORM_S8_UINT},
        // Compressed BC formats
        {PixelFormat::BC1RGBAUnorm,                 DXGI_FORMAT_BC1_UNORM},
        {PixelFormat::BC1RGBAUnormSrgb,             DXGI_FORMAT_BC1_UNORM_SRGB},
        {PixelFormat::BC2RGBAUnorm,                 DXGI_FORMAT_BC2_UNORM},
        {PixelFormat::BC2RGBAUnormSrgb,             DXGI_FORMAT_BC2_UNORM_SRGB},
        {PixelFormat::BC3RGBAUnorm,                 DXGI_FORMAT_BC3_UNORM},
        {PixelFormat::BC3RGBAUnormSrgb,             DXGI_FORMAT_BC3_UNORM_SRGB},
        {PixelFormat::BC4RUnorm,                    DXGI_FORMAT_BC4_UNORM},
        {PixelFormat::BC4RSnorm,                    DXGI_FORMAT_BC4_SNORM},
        {PixelFormat::BC5RGUnorm,                   DXGI_FORMAT_BC5_UNORM},
        {PixelFormat::BC5RGSnorm,                   DXGI_FORMAT_BC5_SNORM},
        {PixelFormat::BC6HRGBUfloat,                DXGI_FORMAT_BC6H_UF16},
        {PixelFormat::BC6HRGBSfloat,                DXGI_FORMAT_BC6H_SF16},
        {PixelFormat::BC7RGBAUnorm,                 DXGI_FORMAT_BC7_UNORM},
        {PixelFormat::BC7RGBAUnormSrgb,             DXGI_FORMAT_BC7_UNORM_SRGB},
    };
    static_assert((unsigned)PixelFormat::Count == ALIMER_STATIC_ARRAY_SIZE(kDxgiFormatDesc), "Invalid PixelFormat size");

    void DXGISetObjectName(IDXGIObject* obj, const eastl::string& name)
    {
#ifdef _DEBUG
        if (obj != nullptr)
        {
            if (!name.empty())
            {
                obj->SetPrivateData(g_D3DDebugObjectName, static_cast<UINT>(name.length()), name.c_str());
            }
            else
            {
                obj->SetPrivateData(g_D3DDebugObjectName, 0, nullptr);
            }
        }
#else
        ALIMER_UNUSED(obj);
        ALIMER_UNUSED(name);
#endif
    }

    ComPtr<IDXGISwapChain1> DXGICreateSwapChain(IDXGIFactory2* dxgiFactory, DXGIFactoryCaps caps,
        IUnknown* deviceOrCommandQueue,
        void* window,
        uint32_t backBufferWidth, uint32_t backBufferHeight, DXGI_FORMAT backBufferFormat,
        uint32_t backBufferCount,
        bool fullscreen)
    {
        UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        if (any(caps & DXGIFactoryCaps::Tearing))
        {
            flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        const DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
        DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        if (!any(caps & DXGIFactoryCaps::FlipPresent))
        {
            swapEffect = DXGI_SWAP_EFFECT_DISCARD;
        }
#else
        const DXGI_SCALING scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        const DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = scaling;
        swapChainDesc.SwapEffect = swapEffect;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = flags;

        ComPtr<IDXGISwapChain1> swapChain;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = !fullscreen;

        // Create a SwapChain from a Win32 window.
        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            deviceOrCommandQueue,
            (HWND)window,
            &swapChainDesc,
            &fsSwapChainDesc,
            NULL,
            swapChain.GetAddressOf()
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(dxgiFactory->MakeWindowAssociation((HWND)window, DXGI_MWA_NO_ALT_ENTER));
#else
        VHR(IDXGIFactory2_CreateSwapChainForCoreWindow(
            dxgi_factory,
            deviceOrCommandQueue,
            window,
            &swapchain_desc,
            NULL,
            &result
        ));
#endif

        return swapChain;
    }
}
