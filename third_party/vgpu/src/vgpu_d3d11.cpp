//
// Copyright (c) 2019-2020 Amer Koleci.
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

#if defined(VGPU_BACKEND_D3D11) 
#include "vgpu_backend.h"

#ifndef NOMINMAX
#   define NOMINMAX
#endif
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#ifndef D3D11_NO_HELPERS
#   define D3D11_NO_HELPERS
#endif
#include <windows.h>
#include <d3d11_1.h>
#include "vgpu_d3d.h"
#include <stdio.h>

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

#define VHR(hr) if (FAILED(hr)) { VGPU_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(obj); (obj) = nullptr; }

static struct {
    bool                    available_initialized;
    bool                    available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    bool can_use_new_features;
    HMODULE dxgi_handle;
    HMODULE d3d11_handle;

    // DXGI functions
    PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    // D3D11 functions.
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif


    vgpu_caps caps;

} d3d11;

/* Renderer functions */
static bool d3d11_init(const vgpu_config* config)
{
    if (!vgpu_d3d11_supported()) {
        return false;
    }

    return true;
}

static void d3d11_destroy(void)
{
}

static vgpu_backend_type d3d11_get_backend(void) {
    return VGPU_BACKEND_TYPE_DIRECT3D11;
}

static vgpu_caps d3d11_get_caps(void)
{
    return d3d11.caps;
}

static VGPUTextureFormat d3d11_GetDefaultDepthFormat(void)
{
    return VGPUTextureFormat_Depth32Float;
}

static VGPUTextureFormat d3d11_GetDefaultDepthStencilFormat(void)
{
    return VGPUTextureFormat_Depth24Plus;
}

static void d3d11_begin_frame(void)
{
}

static void d3d11_end_frame(void)
{
}

static VGPUTexture d3d11_create_texture(const VGPUTextureDescriptor* desc)
{
    return vgpu::kInvalidTexture;
}

static void d3d11_destroy_texture(VGPUTexture handle)
{
}

/* Driver functions */
bool vgpu_d3d11_supported(void) {
    if (d3d11.available_initialized) {
        return d3d11.available;
    }
    d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d11.dxgi_handle = LoadLibraryW(L"dxgi.dll");
    if (d3d11.dxgi_handle == nullptr) {
        return false;
    }

    d3d11.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgi_handle, "CreateDXGIFactory2");
    if (d3d11.CreateDXGIFactory2 == nullptr)
    {
        d3d11.CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgi_handle, "CreateDXGIFactory1");
        if (d3d11.CreateDXGIFactory1 == nullptr)
        {
            return false;
        }

        return false;
    }
    else
    {
        d3d11.can_use_new_features = true;
        d3d11.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgi_handle, "DXGIGetDebugInterface1");
    }

    d3d11.d3d11_handle = LoadLibraryW(L"d3d11.dll");
    if (d3d11.d3d11_handle == nullptr) {
        return false;
    }

    d3d11.D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.d3d11_handle, "D3D11CreateDevice");
    if (d3d11.D3D11CreateDevice == nullptr) {
        return false;
    }

#endif

    return true;
}

vgpu_renderer* vgpu_d3d11_create_device(void) {
    static vgpu_renderer renderer = { nullptr };
    ASSIGN_DRIVER(d3d11);
    return &renderer;
}

#undef VHR
#undef SAFE_RELEASE

#else

#include "vgpu/vgpu.h"

bool vgpu_d3d11_supported(void) {
    return false;
}

VGPUDevice vgpu_d3d11_create_device(void) {
    return nullptr;
}

#endif /* defined(VGPU_BACKEND_D3D11) */
