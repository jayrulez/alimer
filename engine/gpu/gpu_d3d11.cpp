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

#if defined(GPU_D3D11_BACKEND) && TODO_D3D

#include "gpu_backend.h"

#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#include <windows.h>
#include <d3d11_1.h>
#include "gpu_d3d.h"
#include <stdio.h>

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#include <d3dcompiler.h>

#ifdef _DEBUG
#include <dxgidebug.h>
DEFINE_ENUM_FLAG_OPERATORS(DXGI_DEBUG_RLO_FLAGS);

static const GUID agpu_IID_IDXGIInfoQueue = { 0xD67441C7, 0x672A, 0x476f, {0x9E, 0x82, 0xCD, 0x55, 0xB4, 0x49, 0x49, 0xCE} };
static const GUID agpu_IID_IDXGIDebug = { 0x119E7452, 0xDE9E, 0x40fe, {0x88, 0x06, 0x88, 0xF9, 0x0C, 0x12, 0xB4, 0x41 } };
static const GUID agpu_IID_IDXGIDebug1 = { 0xc5a05f0c,0x16f2,0x4adf, {0x9f,0x4d,0xa8,0xc4,0xd5,0x8a,0xc5,0x50 } };

// Declare debug guids to avoid linking with "dxguid.lib"
static const GUID agpu_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
static const GUID agpu_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

#ifdef _DEBUG
static const GUID agpu_IID_ID3D11Debug = { 0x79cf2233, 0x7536, 0x4948, {0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60} };
static const GUID agpu_IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, {0xab,0x82,0xe9,0x7e,0xc7,0x43,0x26,0xf6} };
#endif

/* D3D11 guids */
static const GUID agpu_IID_ID3D11BlendState1 = { 0xcc86fabe, 0xda55, 0x401d, {0x85, 0xe7, 0xe3, 0xc9, 0xde, 0x28, 0x77, 0xe9} };
static const GUID agpu_IID_ID3D11RasterizerState1 = { 0x1217d7a6, 0x5039, 0x418c, {0xb0, 0x42, 0x9c, 0xbe, 0x25, 0x6a, 0xfd, 0x6e} };
static const GUID agpu_IID_ID3DDeviceContextState = { 0x5c1e0d8a, 0x7c23, 0x48f9, {0x8c, 0x59, 0xa9, 0x29, 0x58, 0xce, 0xff, 0x11} };
static const GUID agpu_IID_ID3D11DeviceContext1 = { 0xbb2c6faa, 0xb5fb, 0x4082, {0x8e, 0x6b, 0x38, 0x8b, 0x8c, 0xfa, 0x90, 0xe1} };
static const GUID agpu_IID_ID3D11Device1 = { 0xa04bfb29, 0x08ef, 0x43d6, {0xa4, 0x9c, 0xa9, 0xbd, 0xbd, 0xcb, 0xe6, 0x86} };
static const GUID agpu_IID_ID3DUserDefinedAnnotation = { 0xb2daad8b, 0x03d4, 0x4dbf, {0x95, 0xeb, 0x32, 0xab, 0x4b, 0x63, 0xd0, 0xab} };
static const GUID agpu_IID_ID3D11Texture2D = { 0x6f15aaf2,0xd208,0x4e89, {0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c } };

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

typedef struct AGPUSwapChainD3D11 {
    enum { MAX_COUNT = 16 };

    uint32_t width;
    uint32_t height;

    GPUColor clear_color;
    IDXGISwapChain1* handle;

    TextureHandle backbufferTexture;
    //AGPUPixelFormat depthStencilFormat;
    //TextureHandle depthStencilTexture;
    //VGPURenderPass renderPass;
} AGPUSwapChainD3D11;

struct AGPUTextureD3D11 {
    enum { MAX_COUNT = 1024 };

    ID3D11Resource* handle;
    DXGI_FORMAT dxgi_format;
    GPUTextureLayout layout;
};

struct AGPUBufferD3D11 {
    enum { MAX_COUNT = 1024 };

    ID3D11Buffer* handle;
};

struct VGPUSamplerD3D11 {
    enum { MAX_COUNT = 256 };

    ID3D11SamplerState* handle;
};

typedef struct {
    uint32_t                width;
    uint32_t                height;
    uint32_t                color_attachment_count;
    ID3D11RenderTargetView* color_rtvs[AGPU_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* dsv;
    GPUColor                clear_colors[AGPU_MAX_COLOR_ATTACHMENTS];
} VGPURenderPassD3D11;

typedef struct {
    ID3D11VertexShader* vertex;
    ID3D11PixelShader* fragment;
    void* vs_blob;
    SIZE_T  vs_blob_length;
} vgpu_shader_d3d11;

typedef struct {
    vgpu_shader_d3d11* shader;
    ID3D11InputLayout* input_layout;
} vgpu_pipeline_d3d11;

typedef struct GPURendererD3D11 {
    IDXGIFactory2* factory;
    bool flipPresentSupported;
    bool tearingSupported;

    UINT syncInterval;
    UINT presentFlags;

    ID3D11Device1* device;
    ID3D11DeviceContext1* d3d_context;
    ID3DUserDefinedAnnotation* d3d_annotation;
    D3D_FEATURE_LEVEL feature_level;

    AGPUDeviceCapabilities caps;

    Pool<AGPUTextureD3D11, AGPUTextureD3D11::MAX_COUNT> textures;
    Pool<AGPUBufferD3D11, AGPUBufferD3D11::MAX_COUNT> buffers;

    AGPUSwapChainD3D11 swapchains[AGPUSwapChainD3D11::MAX_COUNT];

    /* Backend device. */
    GPUDevice gpuDevice;
} GPURendererD3D11;

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

    HINSTANCE d3dcompiler_dll;
    bool d3dcompiler_dll_load_failed;
    pD3DCompile D3DCompile;
#endif

} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define vgpuCreateDXGIFactory1 d3d11.CreateDXGIFactory1
#   define vgpuCreateDXGIFactory2 d3d11.CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 d3d11.DXGIGetDebugInterface1
#   define vgpuD3D11CreateDevice d3d11.D3D11CreateDevice
#   define _vgpu_d3d11_D3DCompile d3d11.D3DCompile
#else
#   define vgpuCreateDXGIFactory2 CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define vgpuD3D11CreateDevice D3D11CreateDevice
#   define _vgpu_d3d11_D3DCompile D3DCompile
#endif

#if defined(_DEBUG)
static const GUID g_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };

static bool vgpu_sdk_layers_available() {
    HRESULT hr = vgpuD3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
        NULL,
        D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
        NULL,                    // Any feature level will do.
        0,
        D3D11_SDK_VERSION,
        NULL,                    // No need to keep the D3D device reference.
        NULL,                    // No need to know the feature level.
        NULL                     // No need to keep the D3D device context reference.
    );

    return SUCCEEDED(hr);
}
#endif

/* Helper functions */
static void _vgpu_d3d11_set_name(ID3D11DeviceChild* handle, const char* name)
{
#if defined(_DEBUG)
    if (name)
    {
        const size_t length = strlen(name);
        handle->SetPrivateData(g_WKPDID_D3DDebugObjectName, (UINT)length, name);
    }
    else
    {
        handle->SetPrivateData(g_WKPDID_D3DDebugObjectName, 0, nullptr);
    }
#endif /* defined(_DEBUG) */
}

/* Conversion functions */
static AGPUTextureUsageFlags _agpu_d3d11_get_texture_usage(UINT bind_flags) {
    AGPUTextureUsageFlags usage = 0;
    if (bind_flags & D3D11_BIND_SHADER_RESOURCE) {
        usage |= AGPUTextureUsage_Sampled;
    }
    if (bind_flags & D3D11_BIND_UNORDERED_ACCESS) {
        usage |= AGPUTextureUsage_Storage;
    }
    if (bind_flags & D3D11_BIND_RENDER_TARGET ||
        bind_flags & D3D11_BIND_DEPTH_STENCIL) {
        usage |= AGPUTextureUsage_OutputAttachment;
    }

    return usage;
}

static UINT _agpu_d3d11_get_bind_flags(gpu_buffer_usage usage)
{
    /* Excluse constant buffer */
    if (usage & GPU_BUFFER_USAGE_UNIFORM)
    {
        return D3D11_BIND_CONSTANT_BUFFER;
    }

    UINT bind_flags = 0;
    if (usage & GPU_BUFFER_USAGE_VERTEX)
    {
        bind_flags |= D3D11_BIND_VERTEX_BUFFER;
    }
    if (usage & GPU_BUFFER_USAGE_INDEX)
    {
        bind_flags |= D3D11_BIND_INDEX_BUFFER;
    }

    if (usage & GPU_BUFFER_USAGE_STORAGE)
    {
        bind_flags |= D3D11_BIND_SHADER_RESOURCE;
        bind_flags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    return bind_flags;
}

static D3D11_COMPARISON_FUNC d3d11_GetComparisonFunc(AGPUCompareFunction function)
{
    switch (function)
    {
    case AGPUCompareFunction_Never:
        return D3D11_COMPARISON_NEVER;
    case AGPUCompareFunction_Less:
        return D3D11_COMPARISON_LESS;
    case AGPUCompareFunction_LessEqual:
        return D3D11_COMPARISON_LESS_EQUAL;
    case AGPUCompareFunction_Greater:
        return D3D11_COMPARISON_GREATER;
    case AGPUCompareFunction_GreaterEqual:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case AGPUCompareFunction_Equal:
        return D3D11_COMPARISON_EQUAL;
    case AGPUCompareFunction_NotEqual:
        return D3D11_COMPARISON_NOT_EQUAL;
    case AGPUCompareFunction_Always:
        return D3D11_COMPARISON_ALWAYS;

    default:
        _VGPU_UNREACHABLE();
    }
}

static void agpu_d3d11_createSwapChain(GPURendererD3D11* renderer, AGPUSwapChainD3D11* swapchain, const AGPUSwapChainDescriptor* info)
{
    uint32_t caps = 0;

    if (renderer->flipPresentSupported) {
        caps |= AGPU_FACTORY_FLIP_PRESENT;
    }

    if (renderer->tearingSupported) {
        caps |= AGPU_FACTORY_TEARING;
    }

    swapchain->handle = agpu_d3d_createSwapChain(renderer->factory, (IUnknown*)renderer->device, AGPU_NUM_INFLIGHT_FRAMES, caps, info);

    ID3D11Texture2D* render_target;
    VHR(swapchain->handle->GetBuffer(0, IID_PPV_ARGS(&render_target)));

    D3D11_TEXTURE2D_DESC d3d_texture_desc;
    render_target->GetDesc(&d3d_texture_desc);

    AGPUTextureDescriptor texture_desc = {};
    texture_desc.type = AGPUTextureType_2D;
    texture_desc.format = AGPUPixelFormat_BGRA8Unorm;
    texture_desc.usage = _agpu_d3d11_get_texture_usage(d3d_texture_desc.BindFlags);
    texture_desc.size = { d3d_texture_desc.Width, d3d_texture_desc.Height, d3d_texture_desc.ArraySize };
    texture_desc.mipLevelCount = d3d_texture_desc.MipLevels;
    texture_desc.sampleCount = d3d_texture_desc.SampleDesc.Count;
    texture_desc.externalHandle = (void*)render_target;
    //swapchain->backbufferTexture = agpuCreateTexture(&texture_desc);

    /*
    if (swapchain->depthStencilFormat != VGPU_PIXELFORMAT_UNDEFINED)
    {
        const vgpu_texture_desc depth_texture_desc = {
            .type = VGPU_TEXTURE_TYPE_2D,
            .usage = VGPU_TEXTURE_USAGE_RENDERTARGET,
            .width = d3d_texture_desc.Width,
            .height = d3d_texture_desc.Height,
            .layers = 1u,
            .format = swapchain->depthStencilFormat,
            .mip_levels = 1u,
            .sample_count = 1u
        };

        swapchain->depthStencilTexture = vgpu_create_texture(&depth_texture_desc);
    }

    VGPURenderPassDescriptor pass_desc = {
        .color_attachments[0].texture = swapchain->backbufferTexture,
        .color_attachments[0].clear_color = swapchain->clear_color
    };

    if (swapchain->depthStencilFormat != VGPU_PIXELFORMAT_UNDEFINED)
    {
        pass_desc.depth_stencil_attachment.texture = swapchain->depthStencilTexture;
    }

    swapchain->renderPass = vgpuCreateRenderPass(&pass_desc);*/
    render_target->Release();
}


static void _agpu_d3d11_destroy_swapchain(GPURendererD3D11* renderer, AGPUSwapChainD3D11* swapchain)
{
    //agpuDestroyTexture(swapchain->backbufferTexture);
    SAFE_RELEASE(swapchain->handle);
}


static bool d3d11_supported(void) {
    if (d3d11.available_initialized) {
        return d3d11.available;
    }
    d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d11.dxgi_handle = LoadLibraryW(L"dxgi.dll");
    if (d3d11.dxgi_handle == NULL) {
        return false;
    }

    d3d11.CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgi_handle, "CreateDXGIFactory1");
    d3d11.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgi_handle, "CreateDXGIFactory2");
    if (d3d11.CreateDXGIFactory2 == NULL)
    {
        if (d3d11.CreateDXGIFactory1 == NULL)
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
    if (d3d11.d3d11_handle == NULL) {
        return false;
    }

    d3d11.D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.d3d11_handle, "D3D11CreateDevice");
    if (d3d11.D3D11CreateDevice == NULL) {
        return false;
    }
#endif

    d3d11.available = true;
    return true;
}

static bool agpu_d3d11_create_factory(GPURendererD3D11* renderer, bool validation)
{
    if (!d3d11_supported()) {
        return false;
    }

    SAFE_RELEASE(renderer->factory);

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    bool debugDXGI = false;

    if (validation)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.can_use_new_features)
#endif
        {
            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                debugDXGI = true;

                hr = vgpuCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&renderer->factory));
                if (FAILED(hr)) {
                    return false;
                }

                VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
                VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
                VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
                };

                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(agpu_DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }
    }

    if (!debugDXGI)
#endif
    {
        hr = vgpuCreateDXGIFactory1(IID_PPV_ARGS(&renderer->factory));
        if (FAILED(hr)) {
            return false;
        }
    }

    // Disable FLIP if not on a supporting OS
    renderer->flipPresentSupported = true;
    {
        IDXGIFactory4* factory4;
        HRESULT hr = renderer->factory->QueryInterface(IID_PPV_ARGS(&factory4));
        if (FAILED(hr))
        {
            renderer->flipPresentSupported = false;
        }
        factory4->Release();
    }

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = renderer->factory->QueryInterface(IID_PPV_ARGS(&factory5));
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            factory5->Release();
        }

        if (FAILED(hr) || !allowTearing)
        {
            renderer->tearingSupported = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            renderer->tearingSupported = true;
        }
    }

    return true;
}

static IDXGIAdapter1* d3d11_GetAdapter(GPURendererD3D11* renderer, AGPUPowerPreference powerPreference)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6;
    HRESULT hr = renderer->factory->QueryInterface(IID_PPV_ARGS(&factory6));
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        if (powerPreference == AGPUPowerPreference_LowPower) {
            gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
        }

        for (uint32_t i = 0;
            DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(&adapter)); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter->Release();

                // Don't select the Basic Render Driver adapter.
                continue;
            }

            break;
        }

        factory6->Release();
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != renderer->factory->EnumAdapters1(i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(adapter->GetDesc1(&desc));

            // Don't select the Basic Render Driver adapter.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter->Release();

                continue;
            }

            break;
        }
    }

    return adapter;
}

static void d3d11_destroyDevice(GPUDevice device)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)device->renderer;

    if (renderer->device)
    {
        /* Destroy swap chains.*/
        for (uint32_t i = 0; i < _countof(renderer->swapchains); i++)
        {
            if (!renderer->swapchains[i].handle) {
                continue;
            }

            _agpu_d3d11_destroy_swapchain(renderer, &renderer->swapchains[i]);
        }

        SAFE_RELEASE(renderer->d3d_context);
        SAFE_RELEASE(renderer->d3d_annotation);

        ULONG ref_count = renderer->device->Release();
#if !defined(NDEBUG)
        if (ref_count > 0)
        {
            agpuLog(AGPULogLevel_Error, "Direct3D11: There are %d unreleased references left on the device", ref_count);

            ID3D11Debug* d3d_debug = NULL;
            if (SUCCEEDED(renderer->device->QueryInterface(IID_PPV_ARGS(&d3d_debug))))
            {
                d3d_debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
                d3d_debug->Release();
            }
        }
#else
        (void)ref_count; // avoid warning
#endif
    }

    SAFE_RELEASE(renderer->factory);

#ifdef _DEBUG
    IDXGIDebug* dxgi_debug;
    if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))))
    {
        dxgi_debug->ReportLiveObjects(agpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        dxgi_debug->Release();
    }
#endif

    VGPU_FREE(renderer);
    VGPU_FREE(device);
}

static void d3d11_waitForGPU(gpu_renderer* driver_data)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driver_data;
    renderer->d3d_context->Flush();
}

static void d3d11_beginFrame(gpu_renderer* driver_data) {
}

static void d3d11_presentFrame(gpu_renderer* driver_data) {
    GPURendererD3D11* renderer = (GPURendererD3D11*)driver_data;
    HRESULT hr = S_OK;

    for (uint32_t i = 0; i < _countof(renderer->swapchains); i++)
    {
        if (!renderer->swapchains[i].handle) {
            continue;
        }

        hr = renderer->swapchains[i].handle->Present(renderer->syncInterval, renderer->presentFlags);

        if (hr == DXGI_ERROR_DEVICE_REMOVED
            || hr == DXGI_ERROR_DEVICE_HUNG
            || hr == DXGI_ERROR_DEVICE_RESET
            || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
            || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            //HandleDeviceLost();
            return;
        }
    }
}

static AGPUDeviceCapabilities d3d11_query_caps(gpu_renderer* driver_data)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driver_data;
    return renderer->caps;
}

static AGPUPixelFormat d3d11_getDefaultDepthFormat(gpu_renderer* driver_data)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driver_data;

    UINT dxgi_fmt_caps = 0;
    HRESULT hr = renderer->device->CheckFormatSupport(DXGI_FORMAT_D32_FLOAT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return AGPUPixelFormat_Depth32Float;
    }

    hr = renderer->device->CheckFormatSupport(DXGI_FORMAT_D16_UNORM, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));
    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return AGPUPixelFormat_Depth16Unorm;
    }

    return AGPUPixelFormat_Undefined;
}

static AGPUPixelFormat d3d11_getDefaultDepthStencilFormat(gpu_renderer* driver_data)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driver_data;

    UINT dxgi_fmt_caps = 0;
    HRESULT hr = renderer->device->CheckFormatSupport(DXGI_FORMAT_D24_UNORM_S8_UINT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return AGPUPixelFormat_Depth24Plus;
    }

    hr = renderer->device->CheckFormatSupport(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return AGPUPixelFormat_Depth24PlusStencil8;
    }

    return AGPUPixelFormat_Undefined;
}

/* Texture */
static TextureHandle d3d11_createTexture(gpu_renderer* driverData, const AGPUTextureDescriptor* desc)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driverData;

    if (renderer->buffers.isFull()) {
        return kInvalidTextureHandle;
    }

    const int id = renderer->textures.alloc();
    AGPUTextureD3D11& texture = renderer->textures[id];

    HRESULT hr = S_OK;
    texture.dxgi_format = d3d_GetTextureFormat(desc->format, desc->usage);
    if (desc->externalHandle != NULL)
    {
        texture.handle = (ID3D11Resource*)desc->externalHandle;
        texture.handle->AddRef();
    }
    else
    {
        if (desc->type == AGPUTextureType_3D)
        {
        }
        else
        {
            const uint32_t multiplier = (desc->type == AGPUTextureType_Cube) ? 6 : 1;

            D3D11_TEXTURE2D_DESC d3d11_desc = {};
            d3d11_desc.Width = desc->size.width;
            d3d11_desc.Height = desc->size.height;
            d3d11_desc.MipLevels = desc->mipLevelCount;
            d3d11_desc.ArraySize = desc->size.depth * multiplier;
            d3d11_desc.Format = texture.dxgi_format;
            d3d11_desc.SampleDesc.Count = desc->sampleCount;
            d3d11_desc.SampleDesc.Quality = desc->sampleCount > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
            d3d11_desc.Usage = D3D11_USAGE_DEFAULT;
            d3d11_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            d3d11_desc.CPUAccessFlags = 0;
            d3d11_desc.MiscFlags = (desc->type == AGPUTextureType_Cube) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
            hr = renderer->device->CreateTexture2D(&d3d11_desc, NULL, (ID3D11Texture2D**)&texture.handle);
        }
    }

    if (FAILED(hr)) {
        renderer->textures.dealloc(id);
        return kInvalidTextureHandle;
    }

    return { (uint32_t)id };
}

static void d3d11_destroyTexture(gpu_renderer* driverData, TextureHandle handle)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driverData;
    AGPUTextureD3D11& texture = renderer->textures[handle.id];
    texture.handle->Release();
    renderer->textures.dealloc(handle.id);
}

/* Buffer */
static BufferHandle d3d11_createBuffer(gpu_renderer* driverData, const AGPUBufferDescriptor* desc)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driverData;

    if (renderer->buffers.isFull()) {
        return kInvalidBufferHandle;
    }

    const int id = renderer->buffers.alloc();
    AGPUBufferD3D11& buffer = renderer->buffers[id];

    HRESULT hr = S_OK;

    if (desc->externalHandle != NULL)
    {
        buffer.handle = (ID3D11Buffer*)desc->externalHandle;
        buffer.handle->AddRef();
    }
    else
    {
        D3D11_BUFFER_DESC d3d11_desc = {};
        d3d11_desc.ByteWidth = desc->size;
        //d3d11_desc.Usage = 0;
        d3d11_desc.BindFlags = 0;
        d3d11_desc.CPUAccessFlags = 0;
        d3d11_desc.MiscFlags = 0;
        d3d11_desc.StructureByteStride = 0;
        hr = renderer->device->CreateBuffer(&d3d11_desc, NULL, &buffer.handle);
    }

    if (FAILED(hr)) {
        renderer->buffers.dealloc(id);
        return kInvalidBufferHandle;
    }

    return { (uint32_t)id };
}

static void d3d11_destroyBuffer(gpu_renderer* driverData, BufferHandle handle)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driverData;
    AGPUBufferD3D11& buffer = renderer->buffers[handle.id];
    buffer.handle->Release();
    renderer->buffers.dealloc(handle.id);
}

static GPUDevice d3d11_createDevice(const agpu_device_info* info) {
    HRESULT hr = S_OK;

    GPURendererD3D11* renderer = (GPURendererD3D11*)VGPU_MALLOC(sizeof(GPURendererD3D11));
    memset(renderer, 0, sizeof(GPURendererD3D11));

    /* Create dxgi factory first. */
    const bool validation = ((info->flags & AGPU_DEVICE_FLAGS_DEBUG) || (info->flags & AGPU_DEVICE_FLAGS_GPU_VALIDATION));
    agpu_d3d11_create_factory(renderer, validation);

    renderer->syncInterval = (info->flags & AGPU_DEVICE_FLAGS_VSYNC) ? 1u : 0u;
    if (renderer->syncInterval == 0 && renderer->tearingSupported)
    {
        renderer->presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    IDXGIAdapter1* adapter = d3d11_GetAdapter(renderer, info->powerPreference);

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (validation && vgpu_sdk_layers_available())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }
#endif

        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        // Create the Direct3D 11 API device object and a corresponding context.
        ID3D11Device* temp_d3d_device;
        ID3D11DeviceContext* temp_d3d_context;

        HRESULT hr = E_FAIL;
        if (adapter)
        {
            hr = vgpuD3D11CreateDevice(
                (IDXGIAdapter*)adapter,
                D3D_DRIVER_TYPE_UNKNOWN,
                NULL,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &renderer->feature_level,
                &temp_d3d_context
            );
        }
#if defined(NDEBUG)
        else
        {
            agpuLog(AGPULogLevel_Error, "No Direct3D hardware device found");
            _VGPU_UNREACHABLE();
        }
#else
        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            hr = vgpuD3D11CreateDevice(
                NULL,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                NULL,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &renderer->feature_level,
                &temp_d3d_context
            );

            if (SUCCEEDED(hr))
            {
                OutputDebugStringA("Direct3D Adapter - WARP\n");
            }
        }
#endif

        if (FAILED(hr)) {
            //gpu_device_destroy(device);
            return false;
        }

#ifndef NDEBUG
        ID3D11Debug* d3dDebug;
        if (SUCCEEDED(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3dDebug))))
        {
            ID3D11InfoQueue* d3dInfoQueue;
            if (SUCCEEDED(d3dDebug->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
            {
#ifdef _DEBUG
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
#endif
                D3D11_MESSAGE_ID hide[] =
                {
                    D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                };

                D3D11_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                d3dInfoQueue->AddStorageFilterEntries(&filter);
                d3dInfoQueue->Release();
            }
            d3dDebug->Release();
        }
#endif

        VHR(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&renderer->device)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&renderer->d3d_context)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&renderer->d3d_annotation)));
        SAFE_RELEASE(temp_d3d_context);
        SAFE_RELEASE(temp_d3d_device);
    }

    // Init features and limits.
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(adapter->GetDesc1(&adapter_desc));

        renderer->caps.backend = AGPUBackendType_D3D11;
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
        renderer->caps.features.raytracing = false;

        // Limits
        renderer->caps.limits.max_vertex_attributes = AGPU_MAX_VERTEX_ATTRIBUTES;
        renderer->caps.limits.max_vertex_bindings = AGPU_MAX_VERTEX_ATTRIBUTES;
        renderer->caps.limits.max_vertex_attribute_offset = AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        renderer->caps.limits.max_vertex_binding_stride = AGPU_MAX_VERTEX_BUFFER_STRIDE;

        renderer->caps.limits.max_texture_size_1d = D3D11_REQ_TEXTURE1D_U_DIMENSION;
        renderer->caps.limits.max_texture_size_2d = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        renderer->caps.limits.max_texture_size_3d = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        renderer->caps.limits.max_texture_size_cube = D3D11_REQ_TEXTURECUBE_DIMENSION;
        renderer->caps.limits.max_texture_array_layers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        renderer->caps.limits.max_color_attachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
        renderer->caps.limits.max_uniform_buffer_size = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        renderer->caps.limits.min_uniform_buffer_offset_alignment = 256u;
        renderer->caps.limits.max_storage_buffer_size = UINT32_MAX;
        renderer->caps.limits.min_storage_buffer_offset_alignment = 16;
        renderer->caps.limits.max_sampler_anisotropy = D3D11_MAX_MAXANISOTROPY;
        renderer->caps.limits.max_viewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        renderer->caps.limits.max_viewport_width = D3D11_VIEWPORT_BOUNDS_MAX;
        renderer->caps.limits.max_viewport_height = D3D11_VIEWPORT_BOUNDS_MAX;
        renderer->caps.limits.max_tessellation_patch_size = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        renderer->caps.limits.point_size_range_min = 1.0f;
        renderer->caps.limits.point_size_range_max = 1.0f;
        renderer->caps.limits.line_width_range_min = 1.0f;
        renderer->caps.limits.line_width_range_max = 1.0f;
        renderer->caps.limits.max_compute_shared_memory_size = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        renderer->caps.limits.max_compute_work_group_count_x = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_count_y = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_count_z = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_invocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        renderer->caps.limits.max_compute_work_group_size_x = D3D11_CS_THREAD_GROUP_MAX_X;
        renderer->caps.limits.max_compute_work_group_size_y = D3D11_CS_THREAD_GROUP_MAX_Y;
        renderer->caps.limits.max_compute_work_group_size_z = D3D11_CS_THREAD_GROUP_MAX_Z;

        /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
        UINT dxgi_fmt_caps = 0;
        for (int fmt = (AGPUPixelFormat_Undefined + 1); fmt < AGPUPixelFormat_Count; fmt++)
        {
            DXGI_FORMAT dxgi_fmt = d3d_GetFormat((AGPUPixelFormat)fmt);
            if (dxgi_fmt != DXGI_FORMAT_UNKNOWN)
            {
                HRESULT hr = renderer->device->CheckFormatSupport(dxgi_fmt, &dxgi_fmt_caps);
                VGPU_ASSERT(SUCCEEDED(hr));
                /*sg_pixelformat_info* info = &_sg.formats[fmt];
                info->sample = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D);
                info->filter = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
                info->render = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
                info->blend = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE);
                info->msaa = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);
                info->depth = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
                if (info->depth) {
                    info->render = true;
                }*/
            }
        }
    }

    adapter->Release();

    /* Init pools */
    {
        renderer->textures.init();
        renderer->buffers.init();
    }

    /* Reference gpu_device and renderer together. */
    GPUDeviceImpl* device = (GPUDeviceImpl*)VGPU_MALLOC(sizeof(GPUDeviceImpl));
    device->renderer = (gpu_renderer*)renderer;
    renderer->gpuDevice = device;
    ASSIGN_DRIVER(d3d11);

    /* Create main swap chain if required */
    if (info->swapchain != NULL) {
        agpu_d3d11_createSwapChain(renderer, &renderer->swapchains[0], info->swapchain);
    }

    return device;
}

gpu_driver d3d11_driver = {
    d3d11_supported,
    d3d11_createDevice
};

#endif /* defined(GPU_D3D11_BACKEND) */
