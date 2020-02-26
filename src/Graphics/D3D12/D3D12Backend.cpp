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

#include "D3D12Backend.h"

#define  CHK_ERRA(hrchk) \
        case hrchk: \
             wcscpy_s( desc, count, L#hrchk )

#define  CHK_ERR(hrchk, strOut) \
        case hrchk: \
             wcscpy_s( desc, count, L##strOut )

namespace Alimer
{
#if ALIMER_ENABLE_ASSERT
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
            //        CHK_ERRA(WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT)
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
#endif
}

#undef CHK_ERRA
#undef CHK_ERR
