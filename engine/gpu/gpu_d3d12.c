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

#if defined(GPU_D3D12_BACKEND)

#include "gpu_backend.h"

#ifndef CINTERFACE
#   define CINTERFACE
#endif
#ifndef COBJMACROS
#   define COBJMACROS
#endif
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
#include <d3d12.h>
#include "gpu_d3d.h"
#include <stdio.h>

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#include <d3dcompiler.h>

/* DXGI guids */
static const GUID vgpu_IID_IDXGIAdapter1 = { 0x29038f61, 0x3839, 0x4626, {0x91,0xfd,0x08,0x68,0x79,0x01,0x1a,0x05} };
static const GUID IID_IDXGIFactory4 = { 0x1bc6ea02, 0xef36, 0x464f, {0xbf,0x0c,0x21,0xca,0x39,0xe5,0x16,0x8a} };
static const GUID IID_IDXGIFactory5 = { 0x7632e1f5, 0xee65, 0x4dca, {0x87,0xfd,0x84,0xcd,0x75,0xf8,0x83,0x8d} };
static const GUID IID_IDXGIFactory6 = { 0xc1b6694f, 0xff09, 0x44a9, {0xb0,0x3c,0x77,0x90,0x0a,0x0a,0x1d,0x17} };
static const GUID IID_IDXGISwapChain3 = { 0x94d99bdb, 0xf1f8, 0x4ab0, {0xb2, 0x36, 0x7d, 0xa0, 0x17, 0x0e, 0xda, 0xb1 } };

#ifdef _DEBUG
#include <dxgidebug.h>

static const GUID IID_IDXGIInfoQueue = { 0xD67441C7, 0x672A, 0x476f, {0x9E, 0x82, 0xCD, 0x55, 0xB4, 0x49, 0x49, 0xCE} };
static const GUID IID_IDXGIDebug1 = { 0xc5a05f0c,0x16f2,0x4adf, {0x9f,0x4d,0xa8,0xc4,0xd5,0x8a,0xc5,0x50 } };

// Declare debug guids to avoid linking with "dxguid.lib"
static const GUID vgpu_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
static const GUID vgpu_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

static const GUID IID_ID3D12Device = { 0x189819f1, 0x1db6, 0x4b57, {0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7 } };
static const GUID IID_ID3D12Device1 = { 0x77acce80, 0x638e, 0x4e65, {0x88, 0x95, 0xc1, 0xf2, 0x33, 0x86, 0x86, 0x3e } };
static const GUID IID_ID3D12Device2 = { 0x30baa41e, 0xb15b, 0x475c, {0xa0, 0xbb, 0x1a, 0xf5, 0xc5, 0xb6, 0x43, 0x28 } };
static const GUID IID_ID3D12Device3 = { 0x81dadc15, 0x2bad, 0x4392, {0x93, 0xc5, 0x10, 0x13, 0x45, 0xc4, 0xaa, 0x98 } };
static const GUID IID_ID3D12CommandQueue = { 0x0ec870a6, 0x5d7e, 0x4c22, {0x8c, 0xfc, 0x5b, 0xaa, 0xe0, 0x76, 0x16, 0xed } };
static const GUID IID_ID3D12Resource = { 0x696442be, 0xa72e, 0x4059, {0xbc, 0x79, 0x5b, 0x5c, 0x98, 0x04, 0x0f, 0xad }};

#ifdef _DEBUG
static const GUID IID_ID3D12Debug = { 0x344488b7, 0x6846, 0x474b, {0xb9, 0x89, 0xf0, 0x27, 0x44, 0x82, 0x45, 0xe0} };
static const GUID IID_ID3D12Debug1 = { 0xaffaa4ca, 0x63fe, 0x4d8e, { 0xb8, 0xad, 0x15, 0x90, 0x00, 0xaf, 0x43, 0x04} };
static const GUID IID_ID3D12DebugDevice = { 0x3febd6dd, 0x4973, 0x4787, {0x81, 0x94, 0xe4, 0x5f, 0x9e, 0x28, 0x92, 0x3e} };
#endif

#define VHR(hr) if (FAILED(hr)) { VGPU_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->lpVtbl->Release(obj); (obj) = nullptr; }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

typedef struct GPUSwapchainD3D12 {
    IDXGISwapChain3* handle;
    uint32_t backbufferCount;
    UINT syncInterval;
    UINT presentFlags;

    vgpu_texture backbufferTexture;
    GPUTextureFormat depthStencilFormat;
    vgpu_texture depthStencilTexture;
    VGPURenderPass renderPass;
} GPUSwapChainD3D12;

typedef struct {
    ID3D12Resource* handle;
} GPUBufferD3D12;

typedef struct {
    ID3D12Resource* handle;
    DXGI_FORMAT dxgi_format;
    VGPUTextureLayout layout;
    GPUTextureDescriptor desc;
} VGPUTextureD3D11;

typedef struct {
    uint32_t dummy;
} GPUSamplerD3D11;

typedef struct {
    uint32_t                width;
    uint32_t                height;
    uint32_t                color_attachment_count;
    //ID3D11RenderTargetView* color_rtvs[GPU_MAX_COLOR_ATTACHMENTS];
    //ID3D11DepthStencilView* dsv;
    GPUColor                clear_colors[GPU_MAX_COLOR_ATTACHMENTS];
} VGPURenderPassD3D11;

typedef struct {
    //ID3D11VertexShader* vertex;
   // ID3D11PixelShader* fragment;
    void* vs_blob;
    SIZE_T  vs_blob_length;
} vgpu_shader_d3d11;

typedef struct {
    vgpu_shader_d3d11* shader;
    //ID3D11InputLayout* input_layout;
} vgpu_pipeline_d3d11;

typedef struct {
    ID3D12Device* device;
    ID3D12CommandQueue* graphicsQueue;

    D3D_FEATURE_LEVEL featureLevel;
    D3D_ROOT_SIGNATURE_VERSION  rootSignatureVersion;

    GPUDeviceCapabilities caps;
} GPURendererD3D12;

typedef struct {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window;
#else
    IUnknown* window;
#endif
} GPUBackendSurfaceD3D12;

static struct {
    bool                    available_initialized;
    bool                    available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HMODULE dxgi_handle;
    HMODULE d3d12_handle;

    // DXGI functions
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    // D3D12 functions.
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;

    HINSTANCE d3dcompiler_dll;
    bool d3dcompiler_dll_load_failed;
    pD3DCompile D3DCompile;
#endif

    DWORD factoryFlags;
    IDXGIFactory4* factory;
    bool tearingSupported;

} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define gpuCreateDXGIFactory2 d3d12.CreateDXGIFactory2
#   define gpuDXGIGetDebugInterface1 d3d12.DXGIGetDebugInterface1
#   define gpuD3D12CreateDevice d3d12.D3D12CreateDevice
#   define gpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#else
#   define gpuCreateDXGIFactory2 CreateDXGIFactory2
#   define gpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define gpuD3D12CreateDevice D3D12CreateDevice
#   define gpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#endif

/* Helper functions */
static void _vgpu_d3d12_set_name(ID3D12Object* handle, const char* name)
{
#if defined(_DEBUG)
    if (name)
    {
        // Convert to wchar_t
        size_t len = strlen(name);

        // Workaround for Windows 1903 bug with short strings
        size_t new_len = max(len, 4ULL);

        wchar_t* str = (wchar_t*)alloca((new_len + 1) * sizeof(wchar_t));
        size_t i = 0;
        for (; i < len; ++i)
            str[i] = name[i];
        for (; i < new_len; ++i)
            str[i] = ' ';
        str[i] = 0;
        ID3D12Object_SetName(handle, str);
    }
#endif /* defined(_DEBUG) */
}

/* Conversion functions */
static D3D12_COMPARISON_FUNC get_d3d11_comparison_func(vgpu_compare_function function)
{
    switch (function)
    {
    case VGPU_COMPARE_FUNCTION_NEVER:
        return D3D12_COMPARISON_FUNC_NEVER;
    case VGPU_COMPARE_FUNCTION_LESS:
        return D3D12_COMPARISON_FUNC_LESS;
    case VGPU_COMPARE_FUNCTION_LESS_EQUAL:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case VGPU_COMPARE_FUNCTION_GREATER:
        return D3D12_COMPARISON_FUNC_GREATER;
    case VGPU_COMPARE_FUNCTION_GREATER_EQUAL:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case VGPU_COMPARE_FUNCTION_EQUAL:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case VGPU_COMPARE_FUNCTION_NOT_EQUAL:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case VGPU_COMPARE_FUNCTION_ALWAYS:
        return D3D12_COMPARISON_FUNC_ALWAYS;

    default:
        _VGPU_UNREACHABLE();
    }
}

/* Driver function */

static bool d3d12_supported(void) {
    if (d3d12.available_initialized) {
        return d3d12.available;
    }
    d3d12.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d12.dxgi_handle = LoadLibraryW(L"dxgi.dll");
    if (d3d12.dxgi_handle == nullptr) {
        return false;
    }

    d3d12.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d12.dxgi_handle, "CreateDXGIFactory2");
    if (d3d12.CreateDXGIFactory2 == nullptr)
    {
        return false;
    }

    d3d12.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d12.dxgi_handle, "DXGIGetDebugInterface1");

    d3d12.d3d12_handle = LoadLibraryW(L"d3d12.dll");
    if (d3d12.d3d12_handle == nullptr) {
        return false;
    }

    d3d12.D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12.d3d12_handle, "D3D12CreateDevice");
    if (d3d12.D3D12CreateDevice == nullptr) {
        return false;
    }

    d3d12.D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12.d3d12_handle, "D3D12GetDebugInterface");
#endif

    d3d12.available = true;
    return true;
}

/* Device functions */
static void d3d12_destroyDevice(GPUDevice device)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)device->renderer;
    if (!renderer->device)
        return;

    ID3D12CommandQueue_Release(renderer->graphicsQueue);

    //SAFE_RELEASE(renderer->device3);
    //SAFE_RELEASE(renderer->device2);
    //SAFE_RELEASE(renderer->device1);

#if !defined(NDEBUG)
    ULONG ref_count = ID3D12Device_Release(renderer->device);
    if (ref_count > 0)
    {
        gpuLog(GPU_LOG_LEVEL_ERROR, "Direct3D12: There are %d unreleased references left on the device", ref_count);

        ID3D12DebugDevice* d3d_debug = nullptr;
        if (SUCCEEDED(ID3D12Device_QueryInterface(renderer->device, &IID_ID3D12DebugDevice, (void**)&d3d_debug)))
        {
            ID3D12DebugDevice_ReportLiveDeviceObjects(d3d_debug, D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
            ID3D12DebugDevice_Release(d3d_debug);
        }
    }
#else
    ID3D12Device_Release(renderer->device);
#endif

    VGPU_FREE(renderer);
    VGPU_FREE(device);
}

static GPUDeviceCapabilities d3d12_query_caps(gpu_renderer* driverData)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    return renderer->caps;
}

static VGPURenderPass d3d12_get_default_render_pass(gpu_renderer* driverData) {
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    return nullptr;
}

GPUTextureFormat d3d12_getPreferredSwapChainFormat(gpu_renderer* driverData, GPUSurface surface)
{
    _VGPU_UNUSED(driverData);
    _VGPU_UNUSED(surface);
    return GPU_TEXTURE_FORMAT_BGRA8_UNORM_SRGB;
}

static GPUTextureFormat d3d12_getDefaultDepthFormat(gpu_renderer* driverData)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;

    D3D12_FEATURE_DATA_FORMAT_SUPPORT data = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .Support1 = D3D12_FORMAT_SUPPORT1_NONE,
        .Support2 = D3D12_FORMAT_SUPPORT2_NONE,
    };

    if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(renderer->device, D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        if ((data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        {
            return GPU_TEXTURE_FORMAT_DEPTH32_FLOAT;
        }
    }

    data.Format = DXGI_FORMAT_D16_UNORM;
    if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(renderer->device, D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        if ((data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        {
            return GPU_TEXTURE_FORMAT_DEPTH16_UNORM;
        }
    }

    return GPU_TEXTURE_FORMAT_UNDEFINED;
}

static GPUTextureFormat d3d12_getDefaultDepthStencilFormat(gpu_renderer* driverData)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;

    D3D12_FEATURE_DATA_FORMAT_SUPPORT data = {
       .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
       .Support1 = D3D12_FORMAT_SUPPORT1_NONE,
       .Support2 = D3D12_FORMAT_SUPPORT2_NONE,
    };

    if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(renderer->device, D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        if ((data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        {
            return GPU_TEXTURE_FORMAT_DEPTH24_PLUS;
        }
    }

    data.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(renderer->device, D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        if ((data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        {
            return GPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
        }
    }

    return GPU_TEXTURE_FORMAT_UNDEFINED;
}

static void d3d12_wait_idle(gpu_renderer* driver_data) {
    GPURendererD3D12* renderer = (GPURendererD3D12*)driver_data;
}

static void d3d12_begin_frame(gpu_renderer* driver_data) {
    GPURendererD3D12* renderer = (GPURendererD3D12*)driver_data;
}

static void d3d12_end_frame(gpu_renderer* driver_data) {
    GPURendererD3D12* renderer = (GPURendererD3D12*)driver_data;
    HRESULT hr = S_OK;

    /*for (uint32_t i = 0; i < _VGPU_MAX_SWAPCHAINS; i++)
    {
        if (!renderer->swapchains[i].handle)
            continue;

        hr = IDXGISwapChain1_Present(
            renderer->swapchains[i].handle,
            renderer->swapchains[i].sync_interval,
            renderer->swapchains[i].present_flags);

        if (hr == DXGI_ERROR_DEVICE_REMOVED
            || hr == DXGI_ERROR_DEVICE_HUNG
            || hr == DXGI_ERROR_DEVICE_RESET
            || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
            || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            //HandleDeviceLost();
            return;
        }
    }*/
}

static GPUSwapChain d3d12_createSwapChain(gpu_renderer* driverData, GPUSurface surface, const GPUSwapChainDescriptor* desc)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    GPUBackendSurfaceD3D12* backendSurface = (GPUBackendSurfaceD3D12*)surface->d3d12;

    const DXGI_FORMAT back_buffer_dxgi_format = _vgpu_d3d_swapchain_pixel_format(desc->format);

    DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc = {
            .Width = desc->width,
            .Height = desc->height,
            .Format = back_buffer_dxgi_format,
            .BufferUsage = d3d_GetSwapChainBufferUsage(desc->usage),
            .BufferCount = desc->presentMode == GPU_PRESENT_MODE_FIFO ? 3u : 2u,
            .SampleDesc = {
                .Count = 1,
                .Quality = 0
            },
            .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    };

    if (desc->presentMode == GPU_PRESENT_MODE_IMMEDIATE && d3d12.tearingSupported)
    {
        dxgiSwapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    dxgiSwapChainDesc.Scaling = DXGI_SCALING_STRETCH;

    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgi_swap_chain_fullscreen_desc = {
        .Windowed = true
    };

    // Create a SwapChain from a Win32 window.
    VHR(IDXGIFactory2_CreateSwapChainForHwnd(
        d3d12.factory,
        (IUnknown*)renderer->graphicsQueue,
        backendSurface->window,
        &dxgiSwapChainDesc,
        &dxgi_swap_chain_fullscreen_desc,
        nullptr,
        &tempSwapChain
    ));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
    VHR(IDXGIFactory2_MakeWindowAssociation(d3d12.factory, backendSurface->window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));
#else
    swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

    // Create a swap chain for the window.
    IDXGISwapChain1* tempSwapChain;
    VHR(d3d11.dxgiFactory->CreateSwapChainForCoreWindow(
        d3d11.device,
        swapChain->window,
        &swapChainDesc,
        nullptr,
        &tempSwapChain
    ));


    VHR(swapChain.handle->SetRotation(DXGI_MODE_ROTATION_IDENTITY));

#endif

    GPUSwapChainD3D12* result = _VGPU_ALLOC_HANDLE(GPUSwapChainD3D12);
    VHR(IDXGISwapChain1_QueryInterface(tempSwapChain, &IID_IDXGISwapChain3, (void**)&result->handle));
    SAFE_RELEASE(tempSwapChain);

    result->backbufferCount = dxgiSwapChainDesc.BufferCount;
    for (uint32_t i = 0; i < result->backbufferCount; i++)
    {
        //ID3D12Resource* backBuffer = NULL;
        //VHR(IDXGISwapChain3_GetBuffer(result->handle, i, &IID_ID3D12Resource, (void**)&backBuffer));
    }

    return (GPUSwapChain)result;
}

static void d3d12_destroySwapChain(gpu_renderer* driverData, GPUSwapChain handle) {
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    GPUSwapChainD3D12* swapChain = (GPUSwapChainD3D12*)handle;
    IDXGISwapChain3_Release(swapChain->handle);
    VGPU_FREE(swapChain);
}

#if TODO_D3D11
/* Buffer */
static vgpu_buffer d3d11_create_buffer(VGPURenderer* driver_data, const vgpu_buffer_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;

    D3D11_BUFFER_DESC d3d11_desc;
    d3d11_desc.ByteWidth = (UINT)desc->size;
    d3d11_desc.Usage = D3D11_USAGE_DEFAULT;
    d3d11_desc.BindFlags = _vgpu_d3d11_get_bind_flags(desc->usage);
    d3d11_desc.CPUAccessFlags = 0;
    d3d11_desc.MiscFlags = 0;
    d3d11_desc.StructureByteStride = 0;

    if ((desc->usage & VGPU_BUFFER_USAGE_INDIRECT))
    {
        d3d11_desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    }

    if ((desc->usage & VGPU_BUFFER_USAGE_DYNAMIC))
    {
        d3d11_desc.Usage = D3D11_USAGE_DYNAMIC;
        d3d11_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else if ((desc->usage & VGPU_BUFFER_USAGE_STAGING))
    {
        d3d11_desc.Usage = D3D11_USAGE_STAGING;
        d3d11_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    }

    D3D11_SUBRESOURCE_DATA* initialDataPtr = 0;
    D3D11_SUBRESOURCE_DATA initData;
    if (desc->content != nullptr) {
        initData.pSysMem = desc->content;
        initData.SysMemPitch = 0;
        initData.SysMemSlicePitch = 0;
        initialDataPtr = &initData;
    }

    ID3D11Buffer* handle;
    HRESULT hr = ID3D11Device1_CreateBuffer(renderer->d3d_device, &d3d11_desc, initialDataPtr, &handle);
    if (FAILED(hr)) {
        return nullptr;
    }

    VGPUBufferD3D11* result = _VGPU_ALLOC_HANDLE(VGPUBufferD3D11);
    result->handle = handle;
    return (vgpu_buffer)result;
}

static void d3d11_destroy_buffer(VGPURenderer* driver_data, vgpu_buffer handle) {
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    VGPUBufferD3D11* buffer = (VGPUBufferD3D11*)handle;
    ID3D11Buffer_Release(buffer->handle);
    VGPU_FREE(buffer);
}

/* Texture */
static vgpu_texture d3d11_create_texture(VGPURenderer* driverData, const vgpu_texture_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;

    VGPUTextureD3D11* texture = _VGPU_ALLOC_HANDLE(VGPUTextureD3D11);
    texture->dxgi_format = _vgpu_d3d_get_texture_format(desc->format, desc->usage);
    if (desc->external_handle != nullptr)
    {
        texture->handle.resource = (ID3D11Resource*)desc->external_handle;
        ID3D11Resource_AddRef(texture->handle.resource);
    }
    else
    {
        HRESULT hr = S_OK;

        if (desc->type == VGPU_TEXTURE_TYPE_3D)
        {
        }
        else
        {
            const uint32_t multiplier = (desc->type == VGPU_TEXTURE_TYPE_CUBE) ? 6 : 1;

            const D3D11_TEXTURE2D_DESC d3d11_desc = {
                .Width = desc->width,
                .Height = desc->height,
                .MipLevels = desc->mip_levels,
                .ArraySize = desc->layers * multiplier,
                .Format = texture->dxgi_format,
                .SampleDesc = {
                    .Count = desc->sample_count,
                    .Quality = desc->sample_count > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0
                },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = (desc->type == VGPU_TEXTURE_TYPE_CUBE) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0
            };
            hr = ID3D11Device1_CreateTexture2D(renderer->d3d_device, &d3d11_desc, NULL, &texture->handle.tex2d);
        }

        if (FAILED(hr)) {
            VGPU_FREE(texture);
            return nullptr;
        }
    }

    memcpy(&(texture->desc), desc, sizeof(*desc));
    return (vgpu_texture)texture;
}

static void d3d11_destroy_texture(VGPURenderer* driverData, vgpu_texture handle)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    VGPUTextureD3D11* texture = (VGPUTextureD3D11*)handle;
    ID3D11Resource_Release(texture->handle.resource);
    VGPU_FREE(texture);
}

static vgpu_texture_desc d3d11_query_texture_desc(vgpu_texture handle)
{
    VGPUTextureD3D11* texture = (VGPUTextureD3D11*)handle;
    return texture->desc;
}

/* Sampler */
D3D11_FILTER_TYPE get_d3d11_filterType(vgpu_filter filter)
{
    switch (filter)
    {
    case VGPU_FILTER_NEAREST:
        return D3D11_FILTER_TYPE_POINT;
    case VGPU_FILTER_LINEAR:
        return D3D11_FILTER_TYPE_LINEAR;
    default:
        VGPU_UNREACHABLE();
        return (D3D11_FILTER_TYPE)-1;
    }
}

D3D11_FILTER get_d3d11_filter(vgpu_filter minFilter, vgpu_filter magFilter, vgpu_filter mipFilter, bool isComparison, bool isAnisotropic)
{
    D3D11_FILTER filter;
    D3D11_FILTER_REDUCTION_TYPE reduction = isComparison ? D3D11_FILTER_REDUCTION_TYPE_COMPARISON : D3D11_FILTER_REDUCTION_TYPE_STANDARD;

    if (isAnisotropic)
    {
        filter = D3D11_ENCODE_ANISOTROPIC_FILTER(reduction);
    }
    else
    {
        D3D11_FILTER_TYPE dxMin = get_d3d11_filterType(minFilter);
        D3D11_FILTER_TYPE dxMag = get_d3d11_filterType(magFilter);
        D3D11_FILTER_TYPE dxMip = get_d3d11_filterType(mipFilter);
        filter = D3D11_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, reduction);
    }

    return filter;
};


static D3D11_TEXTURE_ADDRESS_MODE get_d3d11_addressMode(vgpu_address_mode mode)
{
    switch (mode)
    {
    default:
    case VGPU_ADDRESS_MODE_CLAMP_TO_EDGE:
        return D3D11_TEXTURE_ADDRESS_CLAMP;

    case VGPU_ADDRESS_MODE_REPEAT:
        return D3D11_TEXTURE_ADDRESS_WRAP;

    case VGPU_ADDRESS_MODE_MIRROR_REPEAT:
        return D3D11_TEXTURE_ADDRESS_MIRROR;

    case VGPU_ADDRESS_MODE_CLAMP_TO_BORDER:
        return D3D11_TEXTURE_ADDRESS_BORDER;
    }
}

static vgpu_sampler d3d11_samplerCreate(VGPURenderer* driver_data, const vgpu_sampler_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;

    const bool isComparison = desc->compare != VGPU_COMPARE_FUNCTION_UNDEFINED;

    D3D11_SAMPLER_DESC sampler_desc = {
        .Filter = get_d3d11_filter(desc->min_filter, desc->mag_filter, desc->mipmap_filter, isComparison, desc->max_anisotropy > 1),
        .AddressU = get_d3d11_addressMode(desc->address_mode_u),
        .AddressV = get_d3d11_addressMode(desc->address_mode_v),
        .AddressW = get_d3d11_addressMode(desc->address_mode_w),
        .MipLODBias = 0.0f,
        .MaxAnisotropy = desc->max_anisotropy,
        .MinLOD = desc->lod_min_clamp,
        .MaxLOD = desc->lod_max_clamp,
    };

    if (isComparison) {
        sampler_desc.ComparisonFunc = get_d3d11_comparison_func(desc->compare);
    }
    else {
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    }

    switch (desc->border_color) {
    case VGPU_BORDER_COLOR_TRANSPARENT_BLACK:
        sampler_desc.BorderColor[0] = 0.0f;
        sampler_desc.BorderColor[1] = 0.0f;
        sampler_desc.BorderColor[2] = 0.0f;
        sampler_desc.BorderColor[3] = 0.0f;
        break;
    case VGPU_BORDER_COLOR_OPAQUE_BLACK:
        sampler_desc.BorderColor[0] = 0.0f;
        sampler_desc.BorderColor[1] = 0.0f;
        sampler_desc.BorderColor[2] = 0.0f;
        sampler_desc.BorderColor[3] = 1.0f;
        break;
    case VGPU_BORDER_COLOR_OPAQUE_WHITE:
        sampler_desc.BorderColor[0] = 1.0f;
        sampler_desc.BorderColor[1] = 1.0f;
        sampler_desc.BorderColor[2] = 1.0f;
        sampler_desc.BorderColor[3] = 1.0f;
        break;
    }

    ID3D11SamplerState* handle;
    HRESULT hr = ID3D11Device1_CreateSamplerState(renderer->d3d_device, &sampler_desc, &handle);
    if (FAILED(hr)) {
        return nullptr;
    }

    VGPUSamplerD3D11* result = _VGPU_ALLOC_HANDLE(VGPUSamplerD3D11);
    result->handle = handle;
    return (vgpu_sampler)result;
}

static void d3d11_samplerDestroy(VGPURenderer* driver_data, vgpu_sampler handle)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    VGPUSamplerD3D11* sampler = (VGPUSamplerD3D11*)handle;
    ID3D11SamplerState_Release(sampler->handle);
    VGPU_FREE(sampler);
}

/* RenderPass */
static VGPURenderPass d3d11_renderPassCreate(VGPURenderer* driverData, const VGPURenderPassDescriptor* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    VGPURenderPassD3D11* renderPass = _VGPU_ALLOC_HANDLE(VGPURenderPassD3D11);
    renderPass->width = UINT32_MAX;
    renderPass->height = UINT32_MAX;

    for (uint32_t i = 0; i < VGPU_MAX_COLOR_ATTACHMENTS; i++) {
        if (!desc->color_attachments[i].texture) {
            continue;
        }

        uint32_t mip_level = desc->color_attachments[i].mip_level;
        VGPUTextureD3D11* d3d11_texture = (VGPUTextureD3D11*)desc->color_attachments[i].texture;
        renderPass->width = _vgpu_min(renderPass->width, _vgpu_max(1u, d3d11_texture->desc.width >> mip_level));
        renderPass->height = _vgpu_min(renderPass->height, _vgpu_max(1u, d3d11_texture->desc.height >> mip_level));

        HRESULT hr = ID3D11Device1_CreateRenderTargetView(
            renderer->d3d_device,
            d3d11_texture->handle.resource,
            nullptr,
            &renderPass->color_rtvs[renderPass->color_attachment_count]);
        VHR(hr);
        memcpy(&renderPass->clear_colors[i].r, &desc->color_attachments[i].clear_color, sizeof(vgpu_color));
        renderPass->color_attachment_count++;
    }

    return (VGPURenderPass)renderPass;
}

static void d3d11_renderPassDestroy(VGPURenderer* driverData, VGPURenderPass handle)
{
    _VGPU_UNUSED(driverData);
    VGPURenderPassD3D11* render_pass = (VGPURenderPassD3D11*)handle;
    for (uint32_t i = 0; i < render_pass->color_attachment_count; i++) {
        ID3D11RenderTargetView_Release(render_pass->color_rtvs[i]);
    }

    if (render_pass->dsv) {
        ID3D11DepthStencilView_Release(render_pass->dsv);
    }

    VGPU_FREE(render_pass);
}

static void d3d11_renderPassGetExtent(VGPURenderer* driverData, VGPURenderPass handle, uint32_t* width, uint32_t* height)
{
    _VGPU_UNUSED(driverData);
    VGPURenderPassD3D11* render_pass = (VGPURenderPassD3D11*)handle;
    if (width) {
        *width = render_pass->width;
    }

    if (height) {
        *height = render_pass->height;
    }
}

static void d3d11_render_pass_set_color_clear_value(VGPURenderPass handle, uint32_t attachment_index, const float colorRGBA[4])
{
    VGPURenderPassD3D11* render_pass = (VGPURenderPassD3D11*)handle;
    VGPU_ASSERT(attachment_index < render_pass->color_attachment_count);

    render_pass->clear_colors[attachment_index].r = colorRGBA[0];
    render_pass->clear_colors[attachment_index].g = colorRGBA[1];
    render_pass->clear_colors[attachment_index].b = colorRGBA[2];
    render_pass->clear_colors[attachment_index].a = colorRGBA[3];
}

static void d3d11_render_pass_set_depth_stencil_clear_value(VGPURenderPass handle, float depth, uint8_t stencil)
{

}

/* Shader */
static bool _vgpu_d3d11_load_d3dcompiler_dll(void)
{
    /* on UWP, don't do anything (not tested) */
#if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
    return true;
#else
    /* load DLL on demand */
    if ((0 == d3d11.d3dcompiler_dll) && !d3d11.d3dcompiler_dll_load_failed)
    {
        d3d11.d3dcompiler_dll = LoadLibraryW(L"d3dcompiler_47.dll");
        if (0 == d3d11.d3dcompiler_dll) {
            /* don't attempt to load missing DLL in the future */
            vgpu_log_error("failed to load d3dcompiler_47.dll!");
            d3d11.d3dcompiler_dll_load_failed = true;
            return false;
        }

        /* look up function pointers */
        d3d11.D3DCompile = (pD3DCompile)GetProcAddress(d3d11.d3dcompiler_dll, "D3DCompile");
        VGPU_ASSERT(d3d11.D3DCompile);
    }

    return 0 != d3d11.d3dcompiler_dll;
#endif

}
static ID3DBlob* _vgpu_d3d11_compile_shader(const vgpu_shader_stage_desc* stage_desc, const char* target)
{
    if (!_vgpu_d3d11_load_d3dcompiler_dll()) {
        return NULL;
    }

    ID3DBlob* output = NULL;
    ID3DBlob* errors_or_warnings = NULL;
    HRESULT hr = _vgpu_d3d11_D3DCompile(
        stage_desc->source,             /* pSrcData */
        strlen(stage_desc->source),     /* SrcDataSize */
        NULL,                           /* pSourceName */
        NULL,                           /* pDefines */
        NULL,                           /* pInclude */
        stage_desc->entry_point ? stage_desc->entry_point : "main",     /* pEntryPoint */
        target,     /* pTarget (vs_5_0 or ps_5_0) */
        D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,   /* Flags1 */
        0,          /* Flags2 */
        &output,    /* ppCode */
        &errors_or_warnings);   /* ppErrorMsgs */
    if (errors_or_warnings) {
        vgpu_log_error((LPCSTR)ID3D10Blob_GetBufferPointer(errors_or_warnings));
        ID3D10Blob_Release(errors_or_warnings); errors_or_warnings = NULL;
    }
    if (FAILED(hr)) {
        /* just in case, usually output is NULL here */
        if (output) {
            ID3D10Blob_Release(output);
            output = NULL;
        }
    }
    return output;
}

static vgpu_shader d3d11_create_shader(VGPURenderer* driver_data, const vgpu_shader_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    HRESULT hr;

    const void* vs_ptr = 0, * fs_ptr = 0;
    SIZE_T vs_length = 0, fs_length = 0;

    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* fs_blob = nullptr;
    if (desc->vertex.byte_code && desc->fragment.byte_code)
    {
    }
    else
    {
        /* compile from shader source code */
        vs_blob = _vgpu_d3d11_compile_shader(&desc->vertex, "vs_5_0");
        fs_blob = _vgpu_d3d11_compile_shader(&desc->fragment, "ps_5_0");
        if (vs_blob && fs_blob) {
            vs_ptr = ID3D10Blob_GetBufferPointer(vs_blob);
            vs_length = ID3D10Blob_GetBufferSize(vs_blob);
            fs_ptr = ID3D10Blob_GetBufferPointer(fs_blob);
            fs_length = ID3D10Blob_GetBufferSize(fs_blob);
        }
    }

    vgpu_shader_d3d11* shader = _VGPU_ALLOC_HANDLE(vgpu_shader_d3d11);

    if (vs_ptr && fs_ptr && (vs_length > 0) && (fs_length > 0))
    {
        /* create the D3D vertex- and pixel-shader objects */
        hr = ID3D11Device_CreateVertexShader(renderer->d3d_device, vs_ptr, vs_length, NULL, &shader->vertex);
        VGPU_ASSERT(SUCCEEDED(hr) && shader->vertex);
        hr = ID3D11Device_CreatePixelShader(renderer->d3d_device, fs_ptr, fs_length, NULL, &shader->fragment);
        VGPU_ASSERT(SUCCEEDED(hr) && shader->fragment);

        /* need to store the vertex shader byte code, this is needed later in sg_create_pipeline */
        shader->vs_blob_length = vs_length;
        shader->vs_blob = VGPU_MALLOC(vs_length);
        VGPU_ASSERT(shader->vs_blob);
        memcpy(shader->vs_blob, vs_ptr, vs_length);

    }
    if (vs_blob) {
        ID3D10Blob_Release(vs_blob); vs_blob = 0;
    }
    if (fs_blob) {
        ID3D10Blob_Release(fs_blob); fs_blob = 0;
    }

    return (vgpu_shader)shader;
}

static void d3d11_destroy_shader(VGPURenderer* driver_data, vgpu_shader handle)
{
    vgpu_shader_d3d11* shader = (vgpu_shader_d3d11*)handle;
    VGPU_FREE(shader);
}

/* Pipeline */
static vgpu_pipeline d3d11_create_render_pipeline(VGPURenderer* driver_data, const vgpu_render_pipeline_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    vgpu_pipeline_d3d11* pipeline = _VGPU_ALLOC_HANDLE(vgpu_pipeline_d3d11);
    pipeline->shader = (vgpu_shader_d3d11*)desc->shader;

    HRESULT hr;
    D3D11_INPUT_ELEMENT_DESC d3d11_comps[VGPU_MAX_VERTEX_ATTRIBUTES];
    memset(d3d11_comps, 0, sizeof(d3d11_comps));
    uint32_t attr_count = 0;
    for (uint32_t i = 0; i < VGPU_MAX_VERTEX_ATTRIBUTES; i++)
    {
        /*const VGpuVertexAttributeDescriptor* attrDesc = &desc->vertex_state.attributes[i];
        if (attrDesc->format == VGPU_VERTEX_FORMAT_INVALID) {
            continue;
        }

        assert((attrDesc->bufferIndex >= 0) && (attrDesc->bufferIndex < VGPU_MAX_VERTEX_BUFFER_BINDINGS));
        const VGpuVertexBufferLayoutDescriptor* layoutDesc = &desc->vertex_state.layouts[attrDesc->bufferIndex];

        D3D11_INPUT_ELEMENT_DESC* d3d11ElementDesc = &d3d11InputElements[numElements++];
        d3d11ElementDesc->SemanticName = "ATTRIBUTE";
        d3d11ElementDesc->SemanticIndex = i;
        d3d11ElementDesc->Format = _vgpu_d3d_vertex_format(attrDesc->format);
        d3d11ElementDesc->InputSlot = attrDesc->bufferIndex;
        d3d11ElementDesc->AlignedByteOffset = attrDesc->offset;
        if (layoutDesc->inputRate == VGPU_VERTEX_INPUT_RATE_INSTANCE) {
            d3d11ElementDesc->InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
            d3d11ElementDesc->InstanceDataStepRate = 1u;
        }
        else {
            d3d11ElementDesc->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            d3d11ElementDesc->InstanceDataStepRate = 0u;
        }*/
    }

    hr = ID3D11Device1_CreateInputLayout(renderer->d3d_device,
        d3d11_comps,
        attr_count,
        pipeline->shader->vs_blob,
        pipeline->shader->vs_blob_length,
        &pipeline->input_layout);
    VGPU_ASSERT(SUCCEEDED(hr) && pipeline->input_layout);

    return (vgpu_pipeline)pipeline;
}

static vgpu_pipeline d3d11_create_compute_pipeline(VGPURenderer* driver_data, const VgpuComputePipelineDescriptor* desc)
{
    return nullptr;
}

static void d3d11_destroy_pipeline(VGPURenderer* driver_data, vgpu_pipeline handle)
{
    vgpu_pipeline_d3d11* pipeline = (vgpu_pipeline_d3d11*)handle;
    VGPU_FREE(pipeline);
}

static void d3d11_cmdBeginRenderPass(VGPURenderer* driverData, VGPURenderPass handle)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    VGPURenderPassD3D11* render_pass = (VGPURenderPassD3D11*)handle;

    ID3D11DeviceContext1_OMSetRenderTargets(
        renderer->d3d_context,
        render_pass->color_attachment_count, render_pass->color_rtvs,
        render_pass->dsv);

    /* Apply viewport and scissor rect to the render pass */
    const D3D11_VIEWPORT viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = (FLOAT)render_pass->width,
        .Height = (FLOAT)render_pass->height,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f
    };

    const D3D11_RECT scissor_rect = {
        .left = 0,
        .top = 0,
        .right = (LONG)render_pass->width,
        .bottom = (LONG)render_pass->height,
    };

    ID3D11DeviceContext_RSSetViewports(renderer->d3d_context, 1, &viewport);
    ID3D11DeviceContext_RSSetScissorRects(renderer->d3d_context, 1, &scissor_rect);

    /* perform load action */
    for (uint32_t i = 0; i < render_pass->color_attachment_count; i++)
    {
        ID3D11DeviceContext_ClearRenderTargetView(
            renderer->d3d_context, render_pass->color_rtvs[i],
            &render_pass->clear_colors[i].r);
    }
}

static void d3d11_cmdEndRenderPass(VGPURenderer* driverData) {

}

#endif // TODO_D3D11

static bool d3d12_init(const GPUInitConfig* config)
{
    if (!d3d12_supported()) {
        return false;
    }

    HRESULT hr = S_OK;
#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    if ((config->flags & GPUDebugFlags_Debug) ||
        (config->flags & GPUDebugFlags_GPUBasedValidation))
    {
        ID3D12Debug* d3dDebug;
        ID3D12Debug1* d3dDebug1;
        hr = gpuD3D12GetDebugInterface(&IID_ID3D12Debug, (void**)&d3dDebug);
        if (SUCCEEDED(hr))
        {
            ID3D12Debug_EnableDebugLayer(d3dDebug);

            hr = ID3D12Debug_QueryInterface(d3dDebug, &IID_ID3D12Debug1, (void**)&d3dDebug1);
            if (SUCCEEDED(hr))
            {
                if (config->flags & GPUDebugFlags_GPUBasedValidation)
                {
                    ID3D12Debug1_SetEnableGPUBasedValidation(d3dDebug1, TRUE);
                    ID3D12Debug1_SetEnableSynchronizedCommandQueueValidation(d3dDebug1, TRUE);
                }
                else
                {
                    ID3D12Debug1_SetEnableGPUBasedValidation(d3dDebug1, FALSE);
                }
            }
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        SAFE_RELEASE(d3dDebug1);
        SAFE_RELEASE(d3dDebug);

        IDXGIInfoQueue* dxgiInfoQueue;
        if (SUCCEEDED(gpuDXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (void**)&dxgiInfoQueue)))
        {
            d3d12.factoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgiInfoQueue, vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgiInfoQueue, vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgiInfoQueue, vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(DXGI_INFO_QUEUE_FILTER));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;

            IDXGIInfoQueue_AddStorageFilterEntries(dxgiInfoQueue, vgpu_DXGI_DEBUG_DXGI, &filter);
            IDXGIInfoQueue_Release(dxgiInfoQueue);
        }
    }
#endif

    SAFE_RELEASE(d3d12.factory);
    hr = gpuCreateDXGIFactory2(d3d12.factoryFlags, &IID_IDXGIFactory4, (void**)&d3d12.factory);
    if (FAILED(hr)) {
        return false;
    }

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = IDXGIFactory2_QueryInterface(d3d12.factory, &IID_IDXGIFactory5, (void**)&factory5);
        if (SUCCEEDED(hr))
        {
            hr = IDXGIFactory5_CheckFeatureSupport(factory5, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            d3d12.tearingSupported = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            d3d12.tearingSupported = true;
        }
        SAFE_RELEASE(factory5);
    }

    return true;
}

static void d3d12_shutdown(void) {
    if (!d3d12.factory) {
        return;
    }

    SAFE_RELEASE(d3d12.factory);

#ifdef _DEBUG
    IDXGIDebug1* dxgi_debug;
    if (SUCCEEDED(gpuDXGIGetDebugInterface1(0, &IID_IDXGIDebug1, (void**)&dxgi_debug)))
    {
        IDXGIDebug1_ReportLiveObjects(dxgi_debug, vgpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    }
    SAFE_RELEASE(dxgi_debug);
#endif
}

static IDXGIAdapter1* gpuD3D12GetAdapter(GPUPowerPreference powerPreference)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    if (powerPreference != GPU_POWER_PREFERENCE_DEFAULT)
    {
        IDXGIFactory6* dxgi_factory6;
        HRESULT hr = IDXGIFactory2_QueryInterface(d3d12.factory, &IID_IDXGIFactory6, (void**)&dxgi_factory6);
        if (SUCCEEDED(hr))
        {
            // By default prefer high performance
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (powerPreference == GPU_POWER_PREFERENCE_LOW_POWER) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (uint32_t i = 0;
                DXGI_ERROR_NOT_FOUND != IDXGIFactory6_EnumAdapterByGpuPreference(dxgi_factory6, i, gpuPreference, &vgpu_IID_IDXGIAdapter1, (void**)&adapter); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(IDXGIAdapter1_GetDesc1(adapter, &desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    IDXGIAdapter1_Release(adapter);
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(gpuD3D12CreateDevice((IUnknown*)adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, NULL)))
                {
                    break;
                }
            }

            IDXGIFactory6_Release(dxgi_factory6);
        }
    }

#endif
    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != IDXGIFactory2_EnumAdapters1(d3d12.factory, i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(IDXGIAdapter1_GetDesc1(adapter, &desc));

            // Don't select the Basic Render Driver adapter.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                IDXGIAdapter1_Release(adapter);

                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(gpuD3D12CreateDevice((IUnknown*)adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, NULL)))
            {
                break;
            }
        }
    }

    return adapter;
}

static gpu_backend_surface* d3d12_create_surface_from_windows_hwnd(void* hinstance, void* hwnd) {
    _VGPU_UNUSED(hinstance);
    HWND handle = (HWND)hwnd;
    if (!IsWindow(handle)) {
        return NULL;
    }

    GPUBackendSurfaceD3D12* surface = _VGPU_ALLOC_HANDLE(GPUBackendSurfaceD3D12);
    surface->window = handle;
    return (gpu_backend_surface*)surface;
}

static GPUDevice d3d12_createDevice(const GPUDeviceDescriptor* desc)
{
    HRESULT hr = S_OK;
    IDXGIAdapter1* adapter = gpuD3D12GetAdapter(desc->powerPreference);

    GPURendererD3D12* renderer = VGPU_MALLOC(sizeof(GPURendererD3D12));
    memset(renderer, 0, sizeof(GPURendererD3D12));

    hr = gpuD3D12CreateDevice((IUnknown*)adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, &renderer->device);
    if (FAILED(hr))
    {
        return NULL;
    }

    /* Create command queue's */
    {
        const D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0
        };

        VHR(ID3D12Device_CreateCommandQueue(renderer->device, &queueDesc, &IID_ID3D12CommandQueue, (void**)&renderer->graphicsQueue));
    }

    //ID3D12Device_QueryInterface(d3dDevice, &IID_ID3D12Device1, (void**)&renderer->device1);
    //ID3D12Device_QueryInterface(d3dDevice, &IID_ID3D12Device2, (void**)&renderer->device2);
    //ID3D12Device_QueryInterface(d3dDevice, &IID_ID3D12Device3, (void**)&renderer->device3);

    // Init features and limits.
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(IDXGIAdapter1_GetDesc1(adapter, &adapter_desc));

        renderer->caps.backend = GPU_BACKEND_D3D12;
        renderer->caps.vendor_id = adapter_desc.VendorId;
        renderer->caps.device_id = adapter_desc.DeviceId;

        char description[_countof(adapter_desc.Description)];
        wcstombs(description, adapter_desc.Description, _countof(adapter_desc.Description));
        memcpy(renderer->caps.adapter_name, description, _countof(adapter_desc.Description));

        renderer->caps.features.independentBlend = true;
        renderer->caps.features.computeShader = true;
        renderer->caps.features.geometryShader = true;
        renderer->caps.features.tessellationShader = true;
        renderer->caps.features.multiViewport = true;
        renderer->caps.features.indexUint32 = true;
        renderer->caps.features.multiDrawIndirect = true;
        renderer->caps.features.fillModeNonSolid = true;
        renderer->caps.features.samplerAnisotropy = true;
        renderer->caps.features.textureCompressionETC2 = false;
        renderer->caps.features.textureCompressionASTC_LDR = false;
        renderer->caps.features.textureCompressionBC = true;
        renderer->caps.features.textureCubeArray = true;

        // Determine maximum supported feature level for this device
        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
        {
            _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
        };

        HRESULT hr = ID3D12Device_CheckFeatureSupport(renderer->device, D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
        if (SUCCEEDED(hr))
        {
            renderer->featureLevel = featLevels.MaxSupportedFeatureLevel;
        }
        else
        {
            renderer->featureLevel = D3D_FEATURE_LEVEL_11_0;
        }

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        renderer->rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {
            .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1
        };
        if (FAILED(ID3D12Device_CheckFeatureSupport(renderer->device, D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            renderer->rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options;
        if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(renderer->device, D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options)))
            && options.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            renderer->caps.features.raytracing = true;
        }
        else {
            renderer->caps.features.raytracing = false;
        }

        // Limits
        renderer->caps.limits.max_vertex_attributes = GPU_MAX_VERTEX_ATTRIBUTES;
        renderer->caps.limits.max_vertex_bindings = GPU_MAX_VERTEX_ATTRIBUTES;
        renderer->caps.limits.max_vertex_attribute_offset = GPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        renderer->caps.limits.max_vertex_binding_stride = GPU_MAX_VERTEX_BUFFER_STRIDE;

        renderer->caps.limits.max_texture_size_1d = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        renderer->caps.limits.max_texture_size_2d = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        renderer->caps.limits.max_texture_size_3d = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        renderer->caps.limits.max_texture_size_cube = D3D12_REQ_TEXTURECUBE_DIMENSION;
        renderer->caps.limits.max_texture_array_layers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        renderer->caps.limits.max_color_attachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
        renderer->caps.limits.max_uniform_buffer_size = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        renderer->caps.limits.min_uniform_buffer_offset_alignment = 256u;
        renderer->caps.limits.max_storage_buffer_size = UINT32_MAX;
        renderer->caps.limits.min_storage_buffer_offset_alignment = 16;
        renderer->caps.limits.max_sampler_anisotropy = D3D12_MAX_MAXANISOTROPY;
        renderer->caps.limits.max_viewports = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        renderer->caps.limits.max_viewport_width = D3D12_VIEWPORT_BOUNDS_MAX;
        renderer->caps.limits.max_viewport_height = D3D12_VIEWPORT_BOUNDS_MAX;
        renderer->caps.limits.max_tessellation_patch_size = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        renderer->caps.limits.point_size_range_min = 1.0f;
        renderer->caps.limits.point_size_range_max = 1.0f;
        renderer->caps.limits.line_width_range_min = 1.0f;
        renderer->caps.limits.line_width_range_max = 1.0f;
        renderer->caps.limits.max_compute_shared_memory_size = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        renderer->caps.limits.max_compute_work_group_count_x = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_count_y = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_count_z = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_invocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        renderer->caps.limits.max_compute_work_group_size_x = D3D12_CS_THREAD_GROUP_MAX_X;
        renderer->caps.limits.max_compute_work_group_size_y = D3D12_CS_THREAD_GROUP_MAX_Y;
        renderer->caps.limits.max_compute_work_group_size_z = D3D12_CS_THREAD_GROUP_MAX_Z;
    }

    IDXGIAdapter1_Release(adapter);

    /* Reference gpu_device and renderer together. */
    GPUDeviceImpl* device = (GPUDeviceImpl*)VGPU_MALLOC(sizeof(GPUDeviceImpl));
    device->renderer = (gpu_renderer*)renderer;
    ASSIGN_DRIVER(d3d12);
    return device;
}

gpu_driver d3d12_driver = {
    d3d12_supported,
    d3d12_init,
    d3d12_shutdown,
    d3d12_create_surface_from_windows_hwnd,
    d3d12_createDevice
};

#endif /* defined(GPU_D3D11_BACKEND) */
