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

typedef struct GPUSwapchainD3D12 {
    IDXGISwapChain3* handle;
    uint32_t backbufferCount;
    UINT syncInterval;
    UINT presentFlags;

    GPUTexture backbufferTextures[3];
    GPUTextureView backbufferTextureViews[3];
    uint32_t imageIndex;
} GPUSwapChainD3D12;

typedef struct {
    ID3D12Resource* handle;
} GPUBufferD3D12;

typedef struct {
    ID3D12Resource* handle;
    DXGI_FORMAT dxgi_format;
    GPUTextureLayout layout;
    GPUTextureDescriptor desc;
} GPUTextureD3D12;

typedef struct {
    uint32_t dummy;
} GPUSamplerD3D12;

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
    ID3D12CommandQueue* handle;

    // Lifetime of these objects is managed by the descriptor cache
    ID3D12Fence* fence;
    uint64_t nextFenceValue;
    uint64_t lastCompletedFenceValue;
    HANDLE fenceEventHandle;

    CRITICAL_SECTION fenceMutex;
    CRITICAL_SECTION eventMutex;
} CommandQueueD3D12;

typedef struct {
    ID3D12Device* device;
    CommandQueueD3D12* graphicsQueue;

    D3D_FEATURE_LEVEL featureLevel;
    D3D_ROOT_SIGNATURE_VERSION  rootSignatureVersion;

    GPUDeviceCapabilities caps;

    GPUDevice gpuDevice;
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

/* CommandQueue*/
static CommandQueueD3D12* d3d12_CreateCommandQueue(GPURendererD3D12* renderer, D3D12_COMMAND_LIST_TYPE type)
{
    CommandQueueD3D12* result = (CommandQueueD3D12*)VGPU_MALLOC(sizeof(CommandQueueD3D12));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    VHR(renderer->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&result->handle)));

    result->nextFenceValue = (uint64_t)type << 56 | 1;
    result->lastCompletedFenceValue = (uint64_t)type << 56;

    VHR(renderer->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&result->fence)));
    result->fence->Signal(result->lastCompletedFenceValue);

    result->fenceEventHandle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
    VGPU_ASSERT(result->fenceEventHandle != NULL);

    /* Init mutexes */
    InitializeCriticalSection(&result->fenceMutex);
    InitializeCriticalSection(&result->eventMutex);

    return result;
}

static uint64_t d3d12_CommandQueueIncrementFence(CommandQueueD3D12* queue)
{
    {
        EnterCriticalSection(&queue->fenceMutex);
        //ID3D12CommandQueue_Signal(queue->handle, queue->fence, queue->nextFenceValue);
        LeaveCriticalSection(&queue->fenceMutex);
        return queue->nextFenceValue++;
    }
}

static uint64_t d3d12_CommandQueueIsFenceComplete(CommandQueueD3D12* queue, uint64_t fenceValue)
{
    if (fenceValue > queue->lastCompletedFenceValue)
        queue->lastCompletedFenceValue = _vgpu_max(queue->lastCompletedFenceValue, queue->fence->GetCompletedValue());

    return fenceValue <= queue->lastCompletedFenceValue;
}

static void d3d12_CommandQueueWaitForFence(CommandQueueD3D12* queue, uint64_t fenceValue)
{
    if (d3d12_CommandQueueIsFenceComplete(queue, fenceValue))
    {
        return;
    }

    // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
    // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
    // the fence can only have one event set on completion, then thread B has to wait for 
    // 100 before it knows 99 is ready.  Maybe insert sequential events?
    {
        EnterCriticalSection(&queue->eventMutex);

        //ID3D12Fence_SetEventOnCompletion(queue->fence, fenceValue, queue->fenceEventHandle);
        WaitForSingleObject(queue->fenceEventHandle, INFINITE);
        queue->lastCompletedFenceValue = fenceValue;
        LeaveCriticalSection(&queue->eventMutex);
    }
}

static void d3d12_WaitQueueIdle(CommandQueueD3D12* queue)
{
    uint64_t fenceValue = d3d12_CommandQueueIncrementFence(queue);
    d3d12_CommandQueueWaitForFence(queue, fenceValue);
}

static void d3d12_DestroyQueue(CommandQueueD3D12* queue)
{
    CloseHandle(queue->fenceEventHandle);
    //ID3D12Fence_Release(queue->fence);
    //ID3D12CommandQueue_Release(queue->handle);
    DeleteCriticalSection(&queue->fenceMutex);
    DeleteCriticalSection(&queue->eventMutex);
    VGPU_FREE(queue);
}



static void d3d12_waitIdle(gpu_renderer* driverData)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    d3d12_WaitQueueIdle(renderer->graphicsQueue);
    //m_ComputeQueue.WaitForIdle();
    //m_CopyQueue.WaitForIdle();
}


static GPUDeviceCapabilities d3d12_query_caps(gpu_renderer* driverData)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    return renderer->caps;
}

GPUTextureFormat d3d12_getPreferredSwapChainFormat(gpu_renderer* driverData, GPUSurface surface)
{
    _VGPU_UNUSED(driverData);
    _VGPU_UNUSED(surface);
    return GPUTextureFormat_BGRA8UnormSrgb;
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
    if (SUCCEEDED(renderer->device->CheckFeatureSupport( D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        if ((data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0)
        {
            return GPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
        }
    }

    return GPU_TEXTURE_FORMAT_UNDEFINED;
}

/* SwapChain */
static GPUTextureView d3d12_SwapChainGetCurrentTextureView(GPUBackendSwapChain* backend)
{
    GPUSwapChainD3D12* swapChain = (GPUSwapChainD3D12*)backend;
    swapChain->imageIndex = swapChain->handle->GetCurrentBackBufferIndex();

    return swapChain->backbufferTextureViews[swapChain->imageIndex];
}

static void d3d12_SwapChainPresent(GPUBackendSwapChain* backend)
{
    GPUSwapChainD3D12* swapChain = (GPUSwapChainD3D12*)backend;
    HRESULT hr = swapChain->handle->Present(swapChain->syncInterval, swapChain->presentFlags);

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

static GPUSwapChain d3d12_createSwapChain(gpu_renderer* driverData, GPUSurface surface, const GPUSwapChainDescriptor* desc)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    GPUBackendSurfaceD3D12* backendSurface = (GPUBackendSurfaceD3D12*)surface->d3d12;

    const DXGI_FORMAT dxgiFormat = d3d_GetSwapChainFormat(desc->format);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = desc->width;
    swapChainDesc.Height = desc->height;
    swapChainDesc.Format = dxgiFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = desc->presentMode == GPUPresentMode_Fifo ? 3u : 2u;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if (desc->presentMode == GPUPresentMode_Immediate && d3d12.tearingSupported)
    {
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = TRUE;

    // Create a SwapChain from a Win32 window.
    VHR(d3d12.factory->CreateSwapChainForHwnd(
        renderer->graphicsQueue->handle,
        backendSurface->window,
        &swapChainDesc,
        &fsSwapChainDesc,
        NULL,
        &tempSwapChain
    ));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
    VHR(d3d12.factory->MakeWindowAssociation(backendSurface->window, DXGI_MWA_NO_ALT_ENTER));
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

    GPUSwapChainD3D12* backend = _VGPU_ALLOC_HANDLE(GPUSwapChainD3D12);
    VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&backend->handle)));
    SAFE_RELEASE(tempSwapChain);

    backend->backbufferCount = swapChainDesc.BufferCount;
    for (uint32_t i = 0; i < backend->backbufferCount; i++)
    {
        ID3D12Resource* backBuffer = NULL;
        VHR(backend->handle->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        /*const GPUTextureDescriptor textureDesc = {
            .type = GPUTextureType_2D,
            .format = desc->format,
            .usage = desc->usage,
            .size = {desc->width, desc->height, 1u},
            .mipLevelCount = 1u,
            .sampleCount = GPUSampleCount_Count1,
            .externalHandle = backBuffer
        };

        backend->backbufferTextures[i] = gpuDeviceCreateTexture(renderer->gpuDevice, &textureDesc);
        ID3D12Resource_Release(backBuffer);*/
    }

    backend->syncInterval = d3d_GetSyncInterval(desc->presentMode);
    if (desc->presentMode == GPUPresentMode_Immediate)
    {
        if (d3d12.tearingSupported)
            backend->presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        else
            backend->presentFlags |= DXGI_PRESENT_RESTART;
    }

    backend->imageIndex = backend->handle->GetCurrentBackBufferIndex();

    GPUSwapChainImpl* result = (GPUSwapChainImpl*)VGPU_MALLOC(sizeof(GPUSwapChainImpl));
    result->backend = (GPUBackendSwapChain*)backend;
    result->getCurrentTextureView = d3d12_SwapChainGetCurrentTextureView;
    result->present = d3d12_SwapChainPresent;
    return result;
}

static void d3d12_destroySwapChain(gpu_renderer* driverData, GPUSwapChain handle) {
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    GPUSwapChainD3D12* swapChain = (GPUSwapChainD3D12*)handle->backend;
    for (uint32_t i = 0; i < swapChain->backbufferCount; i++)
    {
        gpuDeviceDestroyTexture(renderer->gpuDevice, swapChain->backbufferTextures[i]);
    }

    swapChain->handle->Release();
    VGPU_FREE(swapChain);
    VGPU_FREE(handle);
}

/* Texture */
static GPUTexture d3d12_createTexture(gpu_renderer* driverData, const GPUTextureDescriptor* desc)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    GPUTextureD3D12* texture = _VGPU_ALLOC_HANDLE(GPUTextureD3D12);
    texture->dxgi_format = d3d_GetTextureFormat(desc->format, desc->usage);
    if (desc->externalHandle != nullptr)
    {
        texture->handle = (ID3D12Resource*)desc->externalHandle;
        texture->handle->AddRef();
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

    memcpy(&(texture->desc), desc, sizeof(*desc));
    return (GPUTexture)texture;
}

static void d3d12_destroyTexture(gpu_renderer* driverData, GPUTexture handle)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)driverData;
    GPUTextureD3D12* texture = (GPUTextureD3D12*)handle;
    texture->handle->Release();
    VGPU_FREE(texture);
}

/* Device functions */
static void d3d12_destroyDevice(GPUDevice device)
{
    GPURendererD3D12* renderer = (GPURendererD3D12*)device->renderer;
    if (!renderer->device)
        return;

    d3d12_DestroyQueue(renderer->graphicsQueue);

    //SAFE_RELEASE(renderer->device3);
    //SAFE_RELEASE(renderer->device2);
    //SAFE_RELEASE(renderer->device1);

    ULONG ref_count = renderer->device->Release();
#if !defined(NDEBUG)
    if (ref_count > 0)
    {
        gpuLog(GPULogLevel_Error, "Direct3D12: There are %d unreleased references left on the device", ref_count);

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

    VGPU_FREE(renderer);
    VGPU_FREE(device);
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
        hr = gpuD3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug));
        if (SUCCEEDED(hr))
        {
            d3dDebug->EnableDebugLayer();

            hr = d3dDebug->QueryInterface(IID_PPV_ARGS(&d3dDebug1));
            if (SUCCEEDED(hr))
            {
                if (config->flags & GPUDebugFlags_GPUBasedValidation)
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
        if (SUCCEEDED(gpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
        {
            d3d12.factoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(agpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(DXGI_INFO_QUEUE_FILTER));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;

            dxgiInfoQueue->AddStorageFilterEntries(agpu_DXGI_DEBUG_DXGI, &filter);
            dxgiInfoQueue->Release();
        }
    }
#endif

    SAFE_RELEASE(d3d12.factory);
    hr = gpuCreateDXGIFactory2(d3d12.factoryFlags, IID_PPV_ARGS(&d3d12.factory));
    if (FAILED(hr)) {
        return false;
    }

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = d3d12.factory->QueryInterface(IID_PPV_ARGS(&factory5));
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
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
    if (SUCCEEDED(gpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))))
    {
        dxgi_debug->ReportLiveObjects(agpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
    SAFE_RELEASE(dxgi_debug);
#endif
}

static IDXGIAdapter1* gpuD3D12GetAdapter(GPUPowerPreference powerPreference)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    if (powerPreference != GPUPowerPreference_Default)
    {
        IDXGIFactory6* dxgi_factory6;
        HRESULT hr = d3d12.factory->QueryInterface(IID_PPV_ARGS(&dxgi_factory6));
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
                if (SUCCEEDED(gpuD3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
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
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != d3d12.factory->EnumAdapters1(i, &adapter); i++)
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
            if (SUCCEEDED(gpuD3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
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

    GPURendererD3D12* renderer = (GPURendererD3D12*)VGPU_MALLOC(sizeof(GPURendererD3D12));
    memset(renderer, 0, sizeof(GPURendererD3D12));

    hr = gpuD3D12CreateDevice((IUnknown*)adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&renderer->device));
    if (FAILED(hr))
    {
        return NULL;
    }

    /* Create command queue's */
    {
        renderer->graphicsQueue = d3d12_CreateCommandQueue(renderer, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    //ID3D12Device_QueryInterface(d3dDevice, &IID_ID3D12Device1, (void**)&renderer->device1);
    //ID3D12Device_QueryInterface(d3dDevice, &IID_ID3D12Device2, (void**)&renderer->device2);
    //ID3D12Device_QueryInterface(d3dDevice, &IID_ID3D12Device3, (void**)&renderer->device3);

    // Init features and limits.
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(adapter->GetDesc1(&adapter_desc));

        renderer->caps.backend = GPUBackendType_D3D12;
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

    adapter->Release();

    /* Reference gpu_device and renderer together. */
    GPUDeviceImpl* device = (GPUDeviceImpl*)VGPU_MALLOC(sizeof(GPUDeviceImpl));
    device->renderer = (gpu_renderer*)renderer;
    renderer->gpuDevice = device;
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

#undef VHR
#undef SAFE_RELEASE

#endif /* defined(GPU_D3D11_BACKEND) */
