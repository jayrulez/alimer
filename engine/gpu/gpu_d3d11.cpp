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

#if defined(GPU_D3D11_BACKEND) 

#include "gpu_backend.h"

#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#define WIN32_LEAN_AND_MEAN
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

// Declare debug guids to avoid linking with "dxguid.lib"
static const GUID agpu_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
static const GUID agpu_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

#define VHR(hr) if (FAILED(hr)) { VGPU_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

struct agpu_swapchain_d3d11 {
    enum { MAX_COUNT = 16 };

    uint32_t width;
    uint32_t height;

    GPUTextureFormat colorFormat;
    GPUColor clear_color;
    IDXGISwapChain1* handle;

    agpu_texture backbufferTexture;
    GPUTextureFormat depthStencilFormat;
    agpu_texture depthStencilTexture;
    VGPURenderPass renderPass;
};

struct agpu_buffer_d3d11 {
    enum { MAX_COUNT = 4096 };

    ID3D11Buffer* handle;
};

struct agpu_texture_d3d11 {
    enum { MAX_COUNT = 4096 };

    union
    {
        ID3D11Resource* resource;
        ID3D11Texture1D* tex1d;
        ID3D11Texture2D* tex2d;
        ID3D11Texture3D* tex3d;
    } handle;
    DXGI_FORMAT dxgi_format;
    GPUTextureLayout layout;
};

typedef struct VGPUSamplerD3D11 {
    ID3D11SamplerState* handle;
} VGPUSamplerD3D11;

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
    bool flip_present_supported;
    bool tearing_supported;

    UINT sync_interval;
    UINT present_flags;

    ID3D11Device1* d3d_device;
    ID3D11DeviceContext1* d3d_context;
    ID3DUserDefinedAnnotation* d3d_annotation;
    D3D_FEATURE_LEVEL feature_level;

    GPUDeviceCapabilities caps;

    Pool<agpu_texture_d3d11, agpu_texture_d3d11::MAX_COUNT> textures;
    Pool<agpu_buffer_d3d11, agpu_buffer_d3d11::MAX_COUNT> buffers;

    agpu_swapchain_d3d11 swapchains[agpu_swapchain_d3d11::MAX_COUNT];

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
        nullptr,
        D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
        nullptr,
        D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
        nullptr,                    // Any feature level will do.
        0,
        D3D11_SDK_VERSION,
        nullptr,                    // No need to keep the D3D device reference.
        nullptr,                    // No need to know the feature level.
        nullptr                     // No need to keep the D3D device context reference.
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
        handle->SetPrivateData(g_WKPDID_D3DDebugObjectName, 0, NULL);
    }
#endif /* defined(_DEBUG) */
}

/* Conversion functions */
static agpu_texture_usage_flags _agpu_d3d11_get_texture_usage(UINT bind_flags) {
    agpu_texture_usage_flags usage = 0;
    if (bind_flags & D3D11_BIND_SHADER_RESOURCE) {
        usage |= AGPU_TEXTURE_USAGE_SAMPLED;
    }
    if (bind_flags & D3D11_BIND_UNORDERED_ACCESS) {
        usage |= AGPU_TEXTURE_USAGE_STORAGE;
    }
    if (bind_flags & D3D11_BIND_RENDER_TARGET ||
        bind_flags & D3D11_BIND_DEPTH_STENCIL) {
        usage |= AGPU_TEXTURE_USAGE_OUTPUT_ATTACHMENT;
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

static D3D11_COMPARISON_FUNC d3d11_GetComparisonFunc(GPUCompareFunction function)
{
    switch (function)
    {
    case GPUCompareFunction_Never:
        return D3D11_COMPARISON_NEVER;
    case GPUCompareFunction_Less:
        return D3D11_COMPARISON_LESS;
    case GPUCompareFunction_LessEqual:
        return D3D11_COMPARISON_LESS_EQUAL;
    case GPUCompareFunction_Greater:
        return D3D11_COMPARISON_GREATER;
    case GPUCompareFunction_GreaterEqual:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case GPUCompareFunction_Equal:
        return D3D11_COMPARISON_EQUAL;
    case GPUCompareFunction_NotEqual:
        return D3D11_COMPARISON_NOT_EQUAL;
    case GPUCompareFunction_Always:
        return D3D11_COMPARISON_ALWAYS;

    default:
        _VGPU_UNREACHABLE();
    }
}

static void agpu_d3d11_createSwapChain(GPURendererD3D11* renderer, agpu_swapchain_d3d11* swapchain, const agpu_swapchain_info* info)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window = (HWND)info->native_handle;
    if (!IsWindow(window)) {
        return;
    }
#else
    IUnknown* window = (IUnknown*)info->native_handle;
#endif

    const uint32_t sample_count = 1u;
    const DXGI_FORMAT back_buffer_dxgi_format = d3d_GetSwapChainFormat(swapchain->colorFormat);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = info->width;
    swapChainDesc.Height = info->height;
    swapChainDesc.Format = back_buffer_dxgi_format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = AGPU_NUM_INFLIGHT_FRAMES;
    swapChainDesc.SampleDesc.Count = sample_count;
    swapChainDesc.SampleDesc.Quality = sample_count > 1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;

    if (!renderer->sync_interval && renderer->tearing_supported)
    {
        swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }


#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    if (!renderer->flip_present_supported)
    {
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = TRUE;

    // Create a SwapChain from a Win32 window.
    VHR(renderer->factory->CreateSwapChainForHwnd(
        renderer->d3d_device,
        window,
        &swapChainDesc,
        &fsSwapChainDesc,
        nullptr,
        &swapchain->handle
    ));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
    VHR(renderer->factory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));
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

    IDXGISwapChain3* handle;
    VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain->handle)));
    VHR(swapChain.handle->SetRotation(DXGI_MODE_ROTATION_IDENTITY));
    SAFE_RELEASE(tempSwapChain);
#endif

    /*ID3D11Texture2D* render_target;
    VHR(IDXGISwapChain1_GetBuffer(swapchain->handle, 0, &vgpu_IID_ID3D11Texture2D, (void**)&render_target));

    D3D11_TEXTURE2D_DESC d3d_texture_desc;
    ID3D11Texture2D_GetDesc(render_target, &d3d_texture_desc);

    const vgpu_texture_desc texture_desc = {
        .type = VGPU_TEXTURE_TYPE_2D,
        .usage = _vgpu_d3d11_get_texture_usage(d3d_texture_desc.BindFlags),
        .width = d3d_texture_desc.Width,
        .height = d3d_texture_desc.Height,
        .layers = d3d_texture_desc.ArraySize,
        .format = VGPUTextureFormat_BGRA8Unorm,
        .mip_levels = d3d_texture_desc.MipLevels,
        .sample_count = d3d_texture_desc.SampleDesc.Count,
        .external_handle = (void*)render_target
    };
    swapchain->backbufferTexture = vgpu_create_texture(&texture_desc);

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

    swapchain->renderPass = vgpuCreateRenderPass(&pass_desc);
    ID3D11Texture2D_Release(render_target);*/
}


static void _agpu_d3d11_destroy_swapchain(GPURendererD3D11* renderer, agpu_swapchain_d3d11* swapchain)
{
    //agpu_destroy_texture(renderer->gpuDevice, swapchain->backbufferTexture);
    SAFE_RELEASE(swapchain->handle);
}


static bool d3d11_supported(void) {
    if (d3d11.available_initialized) {
        return d3d11.available;
    }
    d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d11.dxgi_handle = LoadLibraryW(L"dxgi.dll");
    if (d3d11.dxgi_handle == nullptr) {
        return false;
    }

    d3d11.CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgi_handle, "CreateDXGIFactory1");
    d3d11.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgi_handle, "CreateDXGIFactory2");
    if (d3d11.CreateDXGIFactory2 == nullptr)
    {
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

    renderer->flip_present_supported = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    // Disable FLIP if not on a supporting OS
    {
        IDXGIFactory4* factory4;
        HRESULT hr = renderer->factory->QueryInterface(IID_PPV_ARGS(&factory4));
        if (FAILED(hr))
        {
            renderer->flip_present_supported = false;
#ifdef _DEBUG
            OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
        }
        SAFE_RELEASE(factory4);
    }
#endif

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = renderer->factory->QueryInterface(IID_PPV_ARGS(&factory5));
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            renderer->tearing_supported = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            renderer->tearing_supported = true;
        }

        SAFE_RELEASE(factory5);
    }

    return true;
}

static IDXGIAdapter1* d3d11_GetAdapter(GPURendererD3D11* renderer, GPUPowerPreference powerPreference)
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
        if (powerPreference == GPUPowerPreference_LowPower) {
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

    if (renderer->d3d_device)
    {
        /* Destroy swap chains.*/
        for (auto& swapchain : renderer->swapchains)
        {
            if (!swapchain.handle) {
                continue;
            }

            _agpu_d3d11_destroy_swapchain(renderer, &swapchain);
        }

        SAFE_RELEASE(renderer->d3d_context);
        SAFE_RELEASE(renderer->d3d_annotation);

        ULONG ref_count = renderer->d3d_device->Release();
#if !defined(NDEBUG)
        if (ref_count > 0)
        {
            gpuLog(GPULogLevel_Error, "Direct3D11: There are %d unreleased references left on the device", ref_count);

            ID3D11Debug* d3d_debug = nullptr;
            if (SUCCEEDED(renderer->d3d_device->QueryInterface(IID_PPV_ARGS(&d3d_debug))))
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
        dxgi_debug->ReportLiveObjects(agpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
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

    for (auto& swapchain : renderer->swapchains)
    {
        if (!swapchain.handle) {
            continue;
        }

        hr = swapchain.handle->Present(renderer->sync_interval, renderer->present_flags);

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

static GPUDeviceCapabilities d3d11_query_caps(gpu_renderer* driver_data)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driver_data;
    return renderer->caps;
}

static GPUTextureFormat d3d11_getDefaultDepthFormat(gpu_renderer* driver_data)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driver_data;

    UINT dxgi_fmt_caps = 0;
    HRESULT hr = renderer->d3d_device->CheckFormatSupport(DXGI_FORMAT_D32_FLOAT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return GPU_TEXTURE_FORMAT_DEPTH32_FLOAT;
    }

    hr = renderer->d3d_device->CheckFormatSupport(DXGI_FORMAT_D16_UNORM, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));
    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return GPU_TEXTURE_FORMAT_DEPTH16_UNORM;
    }

    return GPU_TEXTURE_FORMAT_UNDEFINED;
}

static GPUTextureFormat d3d11_getDefaultDepthStencilFormat(gpu_renderer* driver_data)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driver_data;

    UINT dxgi_fmt_caps = 0;
    HRESULT hr = renderer->d3d_device->CheckFormatSupport(DXGI_FORMAT_D24_UNORM_S8_UINT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return GPU_TEXTURE_FORMAT_DEPTH24_PLUS;
    }

    hr = renderer->d3d_device->CheckFormatSupport(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return GPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    }

    return GPU_TEXTURE_FORMAT_UNDEFINED;
}

/* Texture */
static agpu_texture d3d11_createTexture(gpu_renderer* driverData, const agpu_texture_info* info)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driverData;
    return { 0 };
}

static void d3d11_destroyTexture(gpu_renderer* driverData, agpu_texture handle)
{
    GPURendererD3D11* renderer = (GPURendererD3D11*)driverData;
}



static GPUDevice d3d11_createDevice(const agpu_device_info* info) {
    HRESULT hr = S_OK;

    GPURendererD3D11* renderer = (GPURendererD3D11*)VGPU_MALLOC(sizeof(GPURendererD3D11));
    memset(renderer, 0, sizeof(GPURendererD3D11));

    /* Create dxgi factory first. */
    const bool validation = ((info->flags & AGPU_DEVICE_FLAGS_DEBUG) || (info->flags & AGPU_DEVICE_FLAGS_GPU_VALIDATION));
    agpu_d3d11_create_factory(renderer, validation);

    renderer->sync_interval = (info->flags & AGPU_DEVICE_FLAGS_VSYNC) ? 1u : 0u;
    if (renderer->sync_interval == 0)
    {
        renderer->present_flags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    IDXGIAdapter1* adapter = d3d11_GetAdapter(renderer, info->power_preference);

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
                nullptr,
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
            gpuLog(GPULogLevel_Error, "No Direct3D hardware device found");
            _VGPU_UNREACHABLE();
        }
#else
        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            hr = vgpuD3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                nullptr,
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
        ID3D11Debug* d3d_debug;
        if (SUCCEEDED(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3d_debug))))
        {
            ID3D11InfoQueue* d3d_info_queue;
            if (SUCCEEDED(d3d_debug->QueryInterface(IID_PPV_ARGS(&d3d_info_queue))))
            {
#ifdef _DEBUG
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
                D3D11_MESSAGE_ID hide[] =
                {
                    D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                };
                D3D11_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                d3d_info_queue->AddStorageFilterEntries(&filter);
            }
            SAFE_RELEASE(d3d_info_queue);
            SAFE_RELEASE(d3d_debug);
        }
#endif

        VHR(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&renderer->d3d_device)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&renderer->d3d_context)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&renderer->d3d_annotation)));
        SAFE_RELEASE(temp_d3d_context);
        SAFE_RELEASE(temp_d3d_device);
    }

    // Init features and limits.
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(adapter->GetDesc1(&adapter_desc));

        renderer->caps.backend = AGPU_BACKEND_TYPE_D3D11;
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
        for (int fmt = (GPU_TEXTURE_FORMAT_UNDEFINED + 1); fmt < _GPUTextureFormat_Count; fmt++)
        {
            DXGI_FORMAT dxgi_fmt = d3d_GetFormat((GPUTextureFormat)fmt);
            if (dxgi_fmt != DXGI_FORMAT_UNKNOWN)
            {
                HRESULT hr = renderer->d3d_device->CheckFormatSupport(dxgi_fmt, &dxgi_fmt_caps);
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
    if (info->swapchain_info != nullptr) {
        agpu_d3d11_createSwapChain(renderer, &renderer->swapchains[0], info->swapchain_info);
    }

    return device;
}

gpu_driver d3d11_driver = {
    d3d11_supported,
    d3d11_createDevice
};

#undef VHR
#undef SAFE_RELEASE

#endif /* defined(GPU_D3D11_BACKEND) */
