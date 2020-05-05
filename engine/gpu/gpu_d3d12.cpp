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

#if defined(GPU_D3D12_BACKEND) && defined(TODO_D3D12)

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
#include <d3d12.h>
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

struct agpu_swapchain_d3d12 {
    enum { MAX_COUNT = 16 };

    IDXGISwapChain3* handle;
    uint32_t backbufferCount;

    agpu_texture backbufferTextures[3];
    GPUTextureView backbufferTextureViews[3];
    uint32_t imageIndex;
};

struct agpu_buffer_d3d12 {
    enum { MAX_COUNT = 4096 };

    ID3D12Resource* handle;
};

struct agpu_texture_d3d12 {
    enum { MAX_COUNT = 4096 };

    ID3D12Resource* handle;
    DXGI_FORMAT dxgi_format;
    GPUTextureLayout layout;
};

typedef struct {
    uint32_t dummy;
} GPUSamplerD3D12;

typedef struct {
    uint32_t                width;
    uint32_t                height;
    uint32_t                color_attachment_count;
    //ID3D11RenderTargetView* color_rtvs[AGPU_MAX_COLOR_ATTACHMENTS];
    //ID3D11DepthStencilView* dsv;
    GPUColor                clear_colors[AGPU_MAX_COLOR_ATTACHMENTS];
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

typedef struct GPURendererD3D12 {
    DWORD factory_flags;
    IDXGIFactory4* factory;
    bool tearing_supported;

    UINT sync_interval;
    UINT present_flags;

    ID3D12Device* device;
    ID3D12CommandQueue* graphics_queue;
    bool shutting_down;

    ID3D12Fence* frameFence;
    HANDLE frameFenceEvent;
    uint64_t frameCount; // Total number of CPU frames completed (completed means all command buffers submitted to the GPU)
    uint64_t frameIndex;  // cpu_frame % AGPU_NUM_INFLIGHT_FRAMES

    D3D_FEATURE_LEVEL featureLevel;
    D3D_ROOT_SIGNATURE_VERSION  rootSignatureVersion;

    GPUDeviceCapabilities caps;

    Pool<agpu_texture_d3d12, agpu_texture_d3d12::MAX_COUNT> textures;
    Pool<agpu_buffer_d3d12, agpu_buffer_d3d12::MAX_COUNT> buffers;

    agpu_swapchain_d3d12 swapchains[agpu_swapchain_d3d12::MAX_COUNT];

    GPUDevice gpuDevice;
} GPURendererD3D12;

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
} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define agpuCreateDXGIFactory2 d3d12.CreateDXGIFactory2
#   define agpuDXGIGetDebugInterface1 d3d12.DXGIGetDebugInterface1
#   define agpuD3D12CreateDevice d3d12.D3D12CreateDevice
#   define agpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#else
#   define agpuCreateDXGIFactory2 CreateDXGIFactory2
#   define agpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define agpuD3D12CreateDevice D3D12CreateDevice
#   define agpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
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
        size_t new_len = _vgpu_max(len, 4ULL);

        wchar_t* str = (wchar_t*)alloca((new_len + 1) * sizeof(wchar_t));
        size_t i = 0;
        for (; i < len; ++i)
            str[i] = name[i];
        for (; i < new_len; ++i)
            str[i] = ' ';
        str[i] = 0;
        handle->SetName(str);
    }
#endif /* defined(_DEBUG) */
}

/* Conversion functions */
static D3D12_COMPARISON_FUNC d3d12_GetComparisonFunc(GPUCompareFunction function)
{
    switch (function)
    {
    case GPUCompareFunction_Never:
        return D3D12_COMPARISON_FUNC_NEVER;
    case GPUCompareFunction_Less:
        return D3D12_COMPARISON_FUNC_LESS;
    case GPUCompareFunction_LessEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case GPUCompareFunction_Greater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case GPUCompareFunction_GreaterEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case GPUCompareFunction_Equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case GPUCompareFunction_NotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case GPUCompareFunction_Always:
        return D3D12_COMPARISON_FUNC_ALWAYS;

    default:
        _VGPU_UNREACHABLE();
    }
}

/* SwapChain */
static void d3d12_createSwapChain(GPURendererD3D12* renderer, agpu_swapchain_d3d12* swapchain, const agpu_swapchain_info* info)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window = (HWND)info->native_handle;
    if (!IsWindow(window)) {
        return;
    }
#else
    IUnknown* window = (IUnknown*)info->native_handle;
#endif

    const DXGI_FORMAT dxgiFormat = d3d_GetSwapChainFormat(info->color_format);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = info->width;
    swapChainDesc.Height = info->height;
    swapChainDesc.Format = dxgiFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = AGPU_NUM_INFLIGHT_FRAMES;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;

    if (renderer->tearing_supported) {
        swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = TRUE;

    // Create a SwapChain from a Win32 window.
    VHR(renderer->factory->CreateSwapChainForHwnd(
        renderer->graphics_queue,
        window,
        &swapChainDesc,
        &fsSwapChainDesc,
        NULL,
        &tempSwapChain
    ));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
    VHR(renderer->factory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));
#else
    swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

    VHR(IDXGIFactory2_CreateSwapChainForCoreWindow(
        d3d12.factory,
        (IUnknown*)renderer->graphicsQueue->handle,
        backendSurface->window,
        &dxgiSwapChainDesc,
        NULL,
        &tempSwapChain
    ));

    VHR(IDXGISwapChain1_SetRotation(tempSwapChain, DXGI_MODE_ROTATION_IDENTITY));
#endif

    VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapchain->handle)));
    SAFE_RELEASE(tempSwapChain);

    swapchain->backbufferCount = swapChainDesc.BufferCount;
    for (uint32_t i = 0; i < swapchain->backbufferCount; i++)
    {
        ID3D12Resource* backBuffer = NULL;
        VHR(swapchain->handle->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        AGPUTextureDescriptor texture_info = {};
        texture_info.format = info->color_format;
        texture_info.size.width = info->width;
        texture_info.size.height = info->height;
        texture_info.size.depth = 1u;
        texture_info.miplevels = 1;
        texture_info.external_handle = backBuffer;

        swapchain->backbufferTextures[i] = agpuDeviceCreateTexture(renderer->gpuDevice, &texture_info);
        backBuffer->Release();
    }

    swapchain->imageIndex = swapchain->handle->GetCurrentBackBufferIndex();
}

static void _agpu_d3d12_destroy_swapchain(GPURendererD3D12* renderer, agpu_swapchain_d3d12* swapchain)
{
    for (uint32_t i = 0; i < swapchain->backbufferCount; i++)
    {
        agpu_destroy_texture(renderer->gpuDevice, swapchain->backbufferTextures[i]);
    }

    SAFE_RELEASE(swapchain->handle);
}

/* Texture */
static agpu_texture d3d12_createTexture(gpu_renderer* driverData, const AGPUTextureDescriptor* info)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;

    if (renderer->textures.isFull()) {
        return { (uint32_t)AGPU_INVALID_ID };
    }

    const int id = renderer->textures.alloc();
    agpu_texture_d3d12& texture = renderer->textures[id];

    texture.dxgi_format = d3d_GetTextureFormat(info->format, info->usage);
    if (info->external_handle != nullptr)
    {
        texture.handle = (ID3D12Resource*)info->external_handle;
        texture.handle->AddRef();
    }
    else
    {
        /*HRESULT hr = S_OK;

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
        }*/
    }

    return { (uint32_t)id };
}

static void d3d12_destroyTexture(gpu_renderer* driverData, agpu_texture handle)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    //GPUTextureD3D12* texture = (GPUTextureD3D12*)handle;
    //texture->handle->Release();
}

/* Device functions */
static void d3d12_destroyDevice(GPUDevice device)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)device->renderer;
    if (!renderer->device)
        return;

    agpu_wait_gpu(device);
    VGPU_ASSERT(renderer->frameCount == renderer->frameFence->GetCompletedValue());
    renderer->shutting_down = true;

    for (auto& swapchain : renderer->swapchains)
    {
        if (!swapchain.handle) {
            continue;
        }

        _agpu_d3d12_destroy_swapchain(renderer, &swapchain);
    }

    CloseHandle(renderer->frameFenceEvent);
    SAFE_RELEASE(renderer->frameFence);
    SAFE_RELEASE(renderer->graphics_queue);

    //SAFE_RELEASE(renderer->device3);
    //SAFE_RELEASE(renderer->device2);
    //SAFE_RELEASE(renderer->device1);

    ULONG ref_count = renderer->device->Release();
#if !defined(NDEBUG)
    if (ref_count > 0)
    {
        agpuLog(GPULogLevel_Error, "Direct3D12: There are %d unreleased references left on the device", ref_count);

        ID3D12DebugDevice* d3d_debug = nullptr;
        if (SUCCEEDED(renderer->device->QueryInterface(IID_PPV_ARGS(&d3d_debug))))
        {
            d3d_debug->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
            d3d_debug->Release();
        }
    }
#else
    (void)ref_count; // avoid warning
#endif

    SAFE_RELEASE(renderer->factory);

#ifdef _DEBUG
    IDXGIDebug* dxgi_debug;
    if (SUCCEEDED(agpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))))
    {
        dxgi_debug->ReportLiveObjects(agpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        dxgi_debug->Release();
    }
#endif

    VGPU_FREE(renderer);
    VGPU_FREE(device);
}

static void d3d12_beginFrame(gpu_renderer* driver_data)
{
}

static void d3d12_presentFrame(gpu_renderer* driver_data)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driver_data;

    // Signal the fence with the current frame number, so that we can check back on it
    renderer->graphics_queue->Signal(renderer->frameFence, ++renderer->frameCount);

    uint64_t GPUFrameCount = renderer->frameFence->GetCompletedValue();

    if ((renderer->frameCount - GPUFrameCount) >= AGPU_NUM_INFLIGHT_FRAMES)
    {
        renderer->frameFence->SetEventOnCompletion(GPUFrameCount + 1, renderer->frameFenceEvent);
        WaitForSingleObject(renderer->frameFenceEvent, INFINITE);
    }

    renderer->frameIndex = renderer->frameCount % AGPU_NUM_INFLIGHT_FRAMES;
}


static void d3d12_waitForGPU(gpu_renderer* driver_data)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driver_data;
    renderer->graphics_queue->Signal(renderer->frameFence, ++renderer->frameCount);
    renderer->frameFence->SetEventOnCompletion(renderer->frameCount, renderer->frameFenceEvent);
    WaitForSingleObject(renderer->frameFenceEvent, INFINITE);

    //m_ComputeQueue.WaitForIdle();
    //m_CopyQueue.WaitForIdle();
}


static GPUDeviceCapabilities d3d12_query_caps(gpu_renderer* driverData)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    return renderer->caps;
}

static GPUTextureFormat d3d12_getDefaultDepthFormat(gpu_renderer* driverData)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;

    D3D12_FEATURE_DATA_FORMAT_SUPPORT data = {};
    data.Format = DXGI_FORMAT_D32_FLOAT;

    if (SUCCEEDED(renderer->device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        if ((data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        {
            return GPU_TEXTURE_FORMAT_DEPTH32_FLOAT;
        }
    }

    data.Format = DXGI_FORMAT_D16_UNORM;
    if (SUCCEEDED(renderer->device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
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

    D3D12_FEATURE_DATA_FORMAT_SUPPORT data = {};
    data.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    if (SUCCEEDED(renderer->device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        if ((data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        {
            return GPU_TEXTURE_FORMAT_DEPTH24_PLUS;
        }
    }

    data.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    if (SUCCEEDED(renderer->device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        if ((data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        {
            return GPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
        }
    }

    return GPU_TEXTURE_FORMAT_UNDEFINED;
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

static IDXGIAdapter1* gpuD3D12GetAdapter(GPURendererD3D12* renderer, GPUPowerPreference powerPreference)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    if (powerPreference != GPUPowerPreference_Default)
    {
        IDXGIFactory6* dxgi_factory6;
        HRESULT hr = renderer->factory->QueryInterface(IID_PPV_ARGS(&dxgi_factory6));
        if (SUCCEEDED(hr))
        {
            // By default prefer high performance
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (powerPreference == GPUPowerPreference_LowPower) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (uint32_t i = 0;
                DXGI_ERROR_NOT_FOUND != dxgi_factory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(&adapter)); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    adapter->Release();
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(agpuD3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }

            dxgi_factory6->Release();
        }
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

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(agpuD3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    return adapter;
}

static GPUDevice d3d12_createDevice(const agpu_device_info* info)
{
    HRESULT hr = S_OK;

    GPURendererD3D12* renderer = (GPURendererD3D12*)VGPU_MALLOC(sizeof(GPURendererD3D12));
    memset(renderer, 0, sizeof(GPURendererD3D12));


#if defined(_DEBUG) || defined(PROFILE)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    if ((info->flags & AGPU_DEVICE_FLAGS_DEBUG) ||
        (info->flags & AGPU_DEVICE_FLAGS_GPU_VALIDATION))
    {
        ID3D12Debug* d3dDebug;
        ID3D12Debug1* d3dDebug1;
        hr = agpuD3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug));
        if (SUCCEEDED(hr))
        {
            d3dDebug->EnableDebugLayer();

            hr = d3dDebug->QueryInterface(IID_PPV_ARGS(&d3dDebug1));
            if (SUCCEEDED(hr))
            {
                if (info->flags & AGPU_DEVICE_FLAGS_GPU_VALIDATION)
                {
                    d3dDebug1->SetEnableGPUBasedValidation(TRUE);
                    d3dDebug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                }
                else
                {
                    d3dDebug1->SetEnableGPUBasedValidation(FALSE);
                }
                d3dDebug1->Release();
            }
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        SAFE_RELEASE(d3dDebug1);
        SAFE_RELEASE(d3dDebug);

        IDXGIInfoQueue* dxgiInfoQueue;
        if (SUCCEEDED(agpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
        {
            renderer->factory_flags = DXGI_CREATE_FACTORY_DEBUG;

            VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(agpu_DXGI_DEBUG_DXGI, &filter);
            dxgiInfoQueue->Release();
        }
    }
#endif

    hr = agpuCreateDXGIFactory2(renderer->factory_flags, IID_PPV_ARGS(&renderer->factory));
    if (FAILED(hr)) {
        return false;
    }

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

    renderer->sync_interval = (info->flags & AGPU_DEVICE_FLAGS_VSYNC) ? 1u : 0u;
    if (renderer->sync_interval == 0)
    {
        renderer->present_flags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    IDXGIAdapter1* adapter = gpuD3D12GetAdapter(renderer, info->power_preference);

    hr = agpuD3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&renderer->device));
    if (FAILED(hr))
    {
        return NULL;
    }

    /* Create command queue's */
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        VHR(renderer->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&renderer->graphics_queue)));
    }

    /* Create frame data. */
    {
        renderer->shutting_down = false;
        VHR(renderer->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&renderer->frameFence)));
        renderer->frameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        renderer->frameCount = 0;
        renderer->frameIndex = 0;
    }

    /* Init pools */
    {
        renderer->textures.init();
        renderer->buffers.init();
    }


    /* Reference gpu_device and renderer together. */
    GPUDeviceImpl* device = (GPUDeviceImpl*)VGPU_MALLOC(sizeof(GPUDeviceImpl));
    device->renderer = (gpu_renderer*)renderer;
    renderer->gpuDevice = device;
    ASSIGN_DRIVER(d3d12);

    /* Create main swapchain if required. */
    if (info->swapchain_info != nullptr) {
        d3d12_createSwapChain(renderer, &renderer->swapchains[0], info->swapchain_info);
    }

    // Init features and limits.
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(adapter->GetDesc1(&adapter_desc));

        renderer->caps.backend = AGPU_BACKEND_TYPE_D3D12;
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

        HRESULT hr = renderer->device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
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
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(renderer->device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            renderer->rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options;
        if (SUCCEEDED(renderer->device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options)))
            && options.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            renderer->caps.features.raytracing = true;
        }
        else {
            renderer->caps.features.raytracing = false;
        }

        // Limits
        renderer->caps.limits.max_vertex_attributes = AGPU_MAX_VERTEX_ATTRIBUTES;
        renderer->caps.limits.max_vertex_bindings = AGPU_MAX_VERTEX_ATTRIBUTES;
        renderer->caps.limits.max_vertex_attribute_offset = AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        renderer->caps.limits.max_vertex_binding_stride = AGPU_MAX_VERTEX_BUFFER_STRIDE;

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

    adapter->Release();
    return device;
}

gpu_driver d3d12_driver = {
    d3d12_supported,
    d3d12_createDevice
};

#endif /* defined(GPU_D3D12_BACKEND) */
