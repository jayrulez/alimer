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

#if defined(VGPU_DRIVER_D3D11) && defined(TODO_D3D11)
#define D3D11_NO_HELPERS
#include <d3d11_1.h>
#include "vgpu_d3d_common.h"
#include <cstdio>

struct D3D11Texture {
    enum { MAX_COUNT = 4096 };

    ID3D11Resource* handle;
    DXGI_FORMAT format;
    uint32_t width;
    uint32_t height;
    uint32_t depth_or_layers;
    uint32_t mipLevels;
    vgpu_texture_type type;
    uint32_t sample_count;
};

struct D3D11Buffer {
    enum { MAX_COUNT = 4096 };
    ID3D11Buffer* handle;
};

struct D3D11Framebuffer {
    enum { MAX_COUNT = 1024 };

    uint32_t colorAttachmentCount;
    ID3D11RenderTargetView* colorAttachments[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView* depthStencilAttachment;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
};

struct D3D11Swapchain {
    UINT syncInterval;
    UINT presentFlags;
    IDXGISwapChain1* handle;
    vgpu_framebuffer framebuffer;
    vgpu_texture backbuffer;
};

struct D3D11CommandBuffer {
    enum { MAX_COUNT = 16 };

    ID3D11DeviceContext1* context;
    ID3DUserDefinedAnnotation* annotation;
    ID3D11CommandList* commandList;
    bool profile;
};

/* Global data */
static struct {
    bool available_initialized;
    bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    struct
    {
        HMODULE instance;
        PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
        PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
        PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    } dxgi;

    HMODULE instance;
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    IDXGIFactory2* factory;
    uint32_t factory_caps;

    uint32_t backbufferCount;

    ID3D11Device1* d3dDevice;
    ID3D11DeviceContext1* d3dContext;
    D3D_FEATURE_LEVEL d3dFeatureLevel;
    D3D11Swapchain swapchains[16];
    bool isLost;

    VGPUPool<D3D11Texture, D3D11Texture::MAX_COUNT> textures;
    VGPUPool<D3D11Buffer, D3D11Buffer::MAX_COUNT> buffers;
    VGPUPool<D3D11Framebuffer, D3D11Framebuffer::MAX_COUNT> framebuffers;

    std::atomic<uint8_t> commandBufferCount{ 0 };
    VGPUThreadSafeRingBuffer<VGPUCommandBuffer, D3D11CommandBuffer::MAX_COUNT> freeCommandBuffers;
    VGPUThreadSafeRingBuffer<VGPUCommandBuffer, D3D11CommandBuffer::MAX_COUNT> activeCommandBuffers;
    D3D11CommandBuffer commandBuffers[D3D11CommandBuffer::MAX_COUNT];
} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define vgpuCreateDXGIFactory1 d3d11.dxgi.CreateDXGIFactory1
#   define vgpuCreateDXGIFactory2 d3d11.dxgi.CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 d3d11.dxgi.DXGIGetDebugInterface1
#   define vgpuD3D11CreateDevice d3d11.D3D11CreateDevice
#else
#   define vgpuCreateDXGIFactory1 CreateDXGIFactory1
#   define vgpuCreateDXGIFactory2 CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define vgpuD3D11CreateDevice D3D11CreateDevice
#endif

#ifdef _DEBUG
static const IID D3D11_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
static const IID D3D11_IID_ID3D11Debug = { 0x79cf2233, 0x7536, 0x4948, {0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60} };
static const IID D3D11_IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, {0xab,0x82,0xe9,0x7e,0xc7,0x43,0x26,0xf6} };
#endif

static bool d3d11_sdk_layers_available() {
    HRESULT hr = vgpuD3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_NULL,
        NULL,
        D3D11_CREATE_DEVICE_DEBUG,
        NULL,
        0,
        D3D11_SDK_VERSION,
        NULL,
        NULL,
        NULL
    );

    return SUCCEEDED(hr);
}

/* Device functions */
static void d3d11_DestroySwapChain(D3D11Swapchain* swapchain);
static void d3d11_UpdateSwapChain(D3D11Swapchain* swapchain, const vgpu_swapchain_info* info);

static bool d3d11_createFactory(bool validation)
{
    SAFE_RELEASE(d3d11.factory);

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    bool debugDXGI = false;

    if (validation)
    {
        IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 &&
            SUCCEEDED(vgpuDXGIGetDebugInterface1(0, __uuidof(IDXGIInfoQueue), (void**)&dxgiInfoQueue)))
#else
        if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, __uuidof(IDXGIInfoQueue), (void**)&dxgiInfoQueue)))
#endif
        {
            debugDXGI = true;
            hr = vgpuCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, __uuidof(IDXGIFactory2), (void**)&d3d11.factory);
            if (FAILED(hr)) {
                return false;
            }

            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            };

            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(D3D_DXGI_DEBUG_DXGI, &filter);
            dxgiInfoQueue->Release();
        }
    }

    if (!debugDXGI)
#endif
    {
        hr = vgpuCreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&d3d11.factory);
        if (FAILED(hr)) {
            return false;
        }
    }

    d3d11.factory_caps = 0;

    // Disable FLIP if not on a supporting OS
    d3d11.factory_caps |= DXGIFACTORY_CAPS_FLIP_PRESENT;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    {
        IDXGIFactory4* factory4;
        HRESULT hr = d3d11.factory->QueryInterface(__uuidof(IDXGIFactory4), (void**)&factory4);
        if (FAILED(hr))
        {
            d3d11.factory_caps &= DXGIFACTORY_CAPS_FLIP_PRESENT;
        }
        factory4->Release();
    }
#endif

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = d3d11.factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&factory5);
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            factory5->Release();
        }

        if (FAILED(hr) || !allowTearing)
        {
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            d3d11.factory_caps |= DXGIFACTORY_CAPS_TEARING;
        }
    }

    return true;
}



static IDXGIAdapter1* d3d11_getAdapter(IDXGIFactory2* factory, bool lowPower)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6;
    HRESULT hr = factory->QueryInterface(__uuidof(IDXGIFactory6), (void**)&factory6);
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        if (lowPower) {
            gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
        }

        for (uint32_t i = 0;
            DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(i, gpuPreference, __uuidof(IDXGIAdapter1), (void**)&adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                adapter->Release();
                continue;
            }

            break;
        }

        factory6->Release();
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                adapter->Release();

                continue;
            }

            break;
        }
    }

    return adapter;
}

static bool d3d11_init(const VGPUDeviceDescription* desc)
{
    d3d11.backbufferCount = 2u;

    if (!d3d11_createFactory(desc->debug)) {
        return false;
    }

    IDXGIAdapter1* adapter = d3d11_getAdapter(d3d11.factory, false);

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (desc->debug && d3d11_sdk_layers_available())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
        }
#if defined(_DEBUG)
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
                &d3d11.d3dFeatureLevel,
                &temp_d3d_context
            );
        }
#if defined(NDEBUG)
        else
        {
            //vgpu_log(AGPULogLevel_Error, "No Direct3D hardware device found");
            VGPU_UNREACHABLE();
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
                &d3d11.d3dFeatureLevel,
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
        if (SUCCEEDED(temp_d3d_device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3d_debug)))
        {
            ID3D11InfoQueue* d3d_info_queue;
            if (SUCCEEDED(d3d_debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3d_info_queue)))
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
                d3d_info_queue->Release();
            }

            d3d_debug->Release();
        }
#endif

        VHR(temp_d3d_device->QueryInterface(__uuidof(ID3D11Device1), (void**)&d3d11.d3dDevice));
        VHR(temp_d3d_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&d3d11.d3dContext));
        temp_d3d_context->Release();
        temp_d3d_device->Release();
    }

    adapter->Release();

    // Init pools.
    d3d11.textures.init();
    d3d11.buffers.init();
    d3d11.framebuffers.init();

    // Init main swap chain if required.
    if (desc->swapchain.windowHandle) {
        d3d11_UpdateSwapChain(&d3d11.swapchains[0], &desc->swapchain);
    }

    return true;
}

static void d3d11_shutdown(void)
{
    for (auto& swapchain : d3d11.swapchains) {
        if (!swapchain.handle) continue;

        d3d11_DestroySwapChain(&swapchain);
    }

    for (uint32_t i = 0; i < D3D11CommandBuffer::MAX_COUNT; i++) {
        SAFE_RELEASE(d3d11.commandBuffers[i].commandList);
        SAFE_RELEASE(d3d11.commandBuffers[i].annotation);
        SAFE_RELEASE(d3d11.commandBuffers[i].context);
    }

    SAFE_RELEASE(d3d11.d3dContext);
    //SAFE_RELEASE(d3d11.d3dAnnotation);

    ULONG refCount = d3d11.d3dDevice->Release();
#if !defined(NDEBUG)
    if (refCount > 0)
    {
        //gpuLog(GPULogLevel_Error, "Direct3D11: There are %d unreleased references left on the device", refCount);

        ID3D11Debug* d3dDebug = nullptr;
        if (SUCCEEDED(d3d11.d3dDevice->QueryInterface(IID_PPV_ARGS(&d3dDebug))))
        {
            d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
            d3dDebug->Release();
        }
    }
#else
    (void)refCount; // avoid warning
#endif

    SAFE_RELEASE(d3d11.factory);
    memset(&d3d11, 0, sizeof(d3d11));
}

static bool d3d11_beginFrame(void) {
    return true;
}

static void d3d11_endFrame(void) {
    HRESULT hr = S_OK;

    // Execute deferred command lists:
    {
        VGPUCommandBuffer commandBuffer;
        while (d3d11.activeCommandBuffers.pop_front(commandBuffer))
        {
            vgpuPopDebugGroup(commandBuffer);

            d3d11.commandBuffers[commandBuffer].context->FinishCommandList(false, &d3d11.commandBuffers[commandBuffer].commandList);
            d3d11.d3dContext->ExecuteCommandList(d3d11.commandBuffers[commandBuffer].commandList, false);
            SAFE_RELEASE(d3d11.commandBuffers[commandBuffer].commandList);

            d3d11.freeCommandBuffers.push_back(commandBuffer);
        }
    }

    for (auto& swapchain : d3d11.swapchains) {
        if (!swapchain.handle) continue;

        swapchain.handle->Present(swapchain.syncInterval, swapchain.presentFlags);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            d3d11.isLost = true;
            break;
        }
    }

    if (d3d11.isLost)
    {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
            static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3d11.d3dDevice->GetDeviceRemovedReason() : hr));
        OutputDebugStringA(buff);
#endif
    }

    VHR(hr);
}

/* Swapchain */
static void d3d11_DestroySwapChain(D3D11Swapchain* swapchain)
{
    vgpu_framebuffer_destroy(swapchain->framebuffer);
    vgpu_texture_destroy(swapchain->backbuffer);
    SAFE_RELEASE(swapchain->handle);
}

static void d3d11_UpdateSwapChain(D3D11Swapchain* swapchain, const vgpu_swapchain_info* info)
{
    if (swapchain->backbuffer.id != VGPU_INVALID_ID) {
        d3d11_DestroySwapChain(swapchain);
    }

    if (swapchain->handle) {
        /* TODO: Resize */
    }
    else {
        swapchain->syncInterval = _vgpuGetSyncInterval(info->presentMode);
        swapchain->presentFlags = 0;

        if (info->presentMode == VGPUPresentMode_Immediate
            && d3d11.factory_caps & DXGIFACTORY_CAPS_TEARING) {
            swapchain->presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        swapchain->handle = vgpu_d3d_create_swapchain(
            d3d11.factory,
            d3d11.d3dDevice,
            d3d11.factory_caps,
            info->windowHandle,
            info->width, info->height,
            info->colorFormat,
            d3d11.backbufferCount, /* 3 for triple buffering */
            info->fullscreen
        );
    }

    ID3D11Texture2D* backbuffer;
    VHR(swapchain->handle->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer));

    D3D11_TEXTURE2D_DESC d3d_desc;
    backbuffer->GetDesc(&d3d_desc);

    vgpu_texture_info textureDesc = {};
    textureDesc.width = d3d_desc.Width;
    textureDesc.height = d3d_desc.Height;
    textureDesc.array_layers = d3d_desc.ArraySize;
    textureDesc.mip_levels = d3d_desc.MipLevels;
    textureDesc.format = info->colorFormat;
    textureDesc.type = VGPU_TEXTURE_TYPE_2D;
    textureDesc.usage = VGPUTextureUsage_RenderTarget;
    textureDesc.sample_count = d3d_desc.SampleDesc.Count;
    textureDesc.external_handle = backbuffer;
    swapchain->backbuffer = vgpu_texture_create(&textureDesc);

    /* Create framebuffer */
    VGPUFramebufferDescription fboDesc = {};
    fboDesc.colorAttachments[0].texture = swapchain->backbuffer;
    swapchain->framebuffer = vgpu_framebuffer_create(&fboDesc);
}

/* Texture */
static vgpu_texture d3d11_texture_create(const vgpu_texture_info* info)
{
    if (d3d11.textures.isFull()) {
        //vgpuLogError("Not enough free texture slots.");
        return { VGPU_INVALID_ID };
    }

    const int id = d3d11.textures.alloc();
    VGPU_ASSERT(id >= 0);

    D3D11Texture& texture = d3d11.textures[id];
    // If depth and either ua or sr, set to typeless
    if (vgpuIsDepthOrStencilFormat(info->format)
        && ((info->usage & (VGPUTextureUsage_Sampled | VGPUTextureUsage_Storage)) != 0))
    {
        texture.format = _vgpuGetTypelessFormatFromDepthFormat(info->format);
    }
    else {
        texture.format = _vgpuGetDXGIFormat(info->format);
    }

    texture.width = info->width;
    texture.height = info->height;
    texture.depth_or_layers = info->depth;
    texture.mipLevels = info->mip_levels;
    texture.type = info->type;
    texture.sample_count = info->sample_count;

    HRESULT hr = S_OK;
    if (info->external_handle) {
        texture.handle = (ID3D11Resource*)info->external_handle;
    }
    else {
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT bindFlags = 0;
        UINT CPUAccessFlags = 0;
        UINT miscFlags = 0;
        UINT arraySizeMultiplier = 1;
        if (info->type == VGPU_TEXTURE_TYPE_CUBE) {
            arraySizeMultiplier = 6;
            miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        if (info->usage & VGPUTextureUsage_Sampled) {
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (info->usage & VGPUTextureUsage_Storage) {
            bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (info->usage & VGPUTextureUsage_RenderTarget) {
            if (vgpuIsDepthOrStencilFormat(info->format)) {
                bindFlags |= D3D11_BIND_DEPTH_STENCIL;
            }
            else {
                bindFlags |= D3D11_BIND_RENDER_TARGET;
            }
        }

        const bool generateMipmaps = false;
        if (generateMipmaps)
        {
            bindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            miscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        if (info->type == VGPU_TEXTURE_TYPE_2D || info->type == VGPU_TEXTURE_TYPE_CUBE) {
            D3D11_TEXTURE2D_DESC d3d11_desc = {};
            d3d11_desc.Width = info->width;
            d3d11_desc.Height = info->height;
            d3d11_desc.MipLevels = info->mip_levels;
            d3d11_desc.ArraySize = info->array_layers * arraySizeMultiplier;
            d3d11_desc.Format = texture.format;
            d3d11_desc.SampleDesc.Count = 1;
            d3d11_desc.SampleDesc.Quality = 0;
            d3d11_desc.Usage = usage;
            d3d11_desc.BindFlags = bindFlags;
            d3d11_desc.CPUAccessFlags = CPUAccessFlags;
            d3d11_desc.MiscFlags = miscFlags;

            hr = d3d11.d3dDevice->CreateTexture2D(
                &d3d11_desc,
                NULL,
                (ID3D11Texture2D**)&texture.handle
            );
        }
        else if (info->type == VGPU_TEXTURE_TYPE_3D) {
            D3D11_TEXTURE3D_DESC d3d11_desc = {};
            d3d11_desc.Width = info->width;
            d3d11_desc.Height = info->height;
            d3d11_desc.Depth = info->depth;
            d3d11_desc.MipLevels = info->mip_levels;
            d3d11_desc.Format = texture.format;
            d3d11_desc.Usage = usage;
            d3d11_desc.BindFlags = bindFlags;
            d3d11_desc.CPUAccessFlags = CPUAccessFlags;
            d3d11_desc.MiscFlags = miscFlags;

            hr = d3d11.d3dDevice->CreateTexture3D(
                &d3d11_desc,
                NULL,
                (ID3D11Texture3D**)&texture.handle
            );
        }

        if (FAILED(hr)) {
            return { VGPU_INVALID_ID };
        }
    }

    return { (uint32_t)id };
}

static void d3d11_texture_destroy(vgpu_texture handle) {
    D3D11Texture& texture = d3d11.textures[handle.id];
    SAFE_RELEASE(texture.handle);
    d3d11.textures.dealloc(handle.id);
}

static uint32_t d3d11_texture_get_width(vgpu_texture handle, uint32_t mipLevel) {
    D3D11Texture& texture = d3d11.textures[handle.id];
    return _vgpu_max(1, texture.width >> mipLevel);
}

static uint32_t d3d11_texture_get_height(vgpu_texture handle, uint32_t mipLevel) {
    D3D11Texture& texture = d3d11.textures[handle.id];
    return _vgpu_max(1, texture.height >> mipLevel);
}

/* Buffer */
static vgpu_buffer d3d11_buffer_create(const vgpu_buffer_info* info)
{
    return { VGPU_INVALID_ID };
}

static void d3d11_buffer_destroy(vgpu_buffer handle) {
    D3D11Buffer& buffer = d3d11.buffers[handle.id];
    SAFE_RELEASE(buffer.handle);
    d3d11.buffers.dealloc(handle.id);
}


/* Framebuffer */
static vgpu_framebuffer d3d11_framebuffer_create(const VGPUFramebufferDescription* desc)
{
    if (d3d11.framebuffers.isFull()) {
        //vgpuLogError("Not enough free texture slots.");
        return { VGPU_INVALID_ID };
    }

    const int id = d3d11.framebuffers.alloc();
    VGPU_ASSERT(id >= 0);

    D3D11Framebuffer& framebuffer = d3d11.framebuffers[id];
    framebuffer.colorAttachmentCount = 0;
    framebuffer.depthStencilAttachment = nullptr;
    framebuffer.width = desc->width;
    framebuffer.height = desc->height;
    framebuffer.layers = desc->layers;

    for (uint32_t i = 0; i < VGPU_MAX_COLOR_ATTACHMENTS; i++)
    {
        const VGPUFramebufferAttachment* attachment = &desc->colorAttachments[i];
        if (attachment->texture.id == VGPU_INVALID_ID) {
            continue;
        }

        D3D11Texture& texture = d3d11.textures[attachment->texture.id];

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        memset(&rtvDesc, 0, sizeof(rtvDesc));

        /* TODO: Understand SwapChain RTV format when using RGBA8UNormSrgb */
        rtvDesc.Format = texture.format;
        switch (texture.type)
        {
        case VGPU_TEXTURE_TYPE_2D:

            if (texture.sample_count <= 1)
            {
                if (texture.depth_or_layers > 1)
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.MipSlice = attachment->mipLevel;
                    rtvDesc.Texture2DArray.FirstArraySlice = attachment->slice;
                    rtvDesc.Texture2DArray.ArraySize = texture.depth_or_layers;
                }
                else
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = attachment->mipLevel;
                }
            }
            else
            {
                if (texture.depth_or_layers > 1)
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    rtvDesc.Texture2DMSArray.FirstArraySlice = attachment->slice;
                    rtvDesc.Texture2DMSArray.ArraySize = texture.depth_or_layers;
                }
                else
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
            }

            break;
        case VGPU_TEXTURE_TYPE_3D:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = attachment->mipLevel;
            rtvDesc.Texture3D.FirstWSlice = attachment->slice;
            rtvDesc.Texture3D.WSize = static_cast<UINT>(-1);
            break;
        case VGPU_TEXTURE_TYPE_CUBE:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = attachment->mipLevel;
            rtvDesc.Texture2DArray.FirstArraySlice = attachment->slice;
            rtvDesc.Texture2DArray.ArraySize = texture.depth_or_layers;
            break;

        default:
            break;
        }

        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        VHR(d3d11.d3dDevice->CreateRenderTargetView(
            texture.handle,
            &rtvDesc,
            &framebuffer.colorAttachments[framebuffer.colorAttachmentCount++])
        );
    }

    return { (uint32_t)id };
}

static vgpu_framebuffer d3d11_framebuffer_create_from_window(const vgpu_swapchain_info* info)
{
    return { VGPU_INVALID_ID };
}

static void d3d11_framebuffer_destroy(vgpu_framebuffer handle) {
    D3D11Framebuffer& framebuffer = d3d11.framebuffers[handle.id];
    for (uint32_t i = 0; i < framebuffer.colorAttachmentCount; i++) {
        SAFE_RELEASE(framebuffer.colorAttachments[i]);
    }

    SAFE_RELEASE(framebuffer.depthStencilAttachment);
    d3d11.framebuffers.dealloc(handle.id);
}

static vgpu_framebuffer d3d11_getDefaultFramebuffer(void) {
    return d3d11.swapchains[0].framebuffer;
}

/* CommandBuffer */
static VGPUCommandBuffer d3d11_beginCommandBuffer(const char* name, bool profile) {
    VGPUCommandBuffer commandBuffer;
    if (!d3d11.freeCommandBuffers.pop_front(commandBuffer))
    {
        commandBuffer = (VGPUCommandBuffer)d3d11.commandBufferCount.fetch_add(1);
        assert(commandBuffer < D3D11CommandBuffer::MAX_COUNT);

        VHR(d3d11.d3dDevice->CreateDeferredContext1(0, &d3d11.commandBuffers[commandBuffer].context));
        VHR(d3d11.commandBuffers[commandBuffer].context->QueryInterface(IID_PPV_ARGS(&d3d11.commandBuffers[commandBuffer].annotation)));
    }

    d3d11.commandBuffers[commandBuffer].profile = profile;
    vgpuPushDebugGroup(commandBuffer, name);

    d3d11.activeCommandBuffers.push_back(commandBuffer);
    return commandBuffer;
}

static void d3d11_insertDebugMarker(VGPUCommandBuffer commandBuffer, const char* name)
{
    wchar_t wideName[128];
    int num = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (num > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, name, -1, &wideName[0], num);
    }

    if (num > 0) {
        d3d11.commandBuffers[commandBuffer].annotation->SetMarker(wideName);
    }
}

static void d3d11_pushDebugGroup(VGPUCommandBuffer commandBuffer, const char* name)
{
    wchar_t wideName[128];
    int num = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (num > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, name, -1, &wideName[0], num);
    }

    if (num > 0) {
        d3d11.commandBuffers[commandBuffer].annotation->BeginEvent(wideName);
    }
}

static void d3d11_popDebugGroup(VGPUCommandBuffer commandBuffer)
{
    d3d11.commandBuffers[commandBuffer].annotation->EndEvent();
}

static void d3d11_beginRenderPass(VGPUCommandBuffer commandBuffer, const VGPURenderPassBeginDescription* beginDesc) {
    D3D11Framebuffer& framebuffer = d3d11.framebuffers[beginDesc->framebuffer.id];

    for (uint32_t i = 0; i < framebuffer.colorAttachmentCount; i++)
    {
        if (beginDesc->colorAttachments[i].loadAction == VGPULoadAction_Clear)
        {
            d3d11.commandBuffers[commandBuffer].context->ClearRenderTargetView(
                framebuffer.colorAttachments[i],
                &beginDesc->colorAttachments[i].clear_color.r
            );
        }
    }

    if (framebuffer.depthStencilAttachment != nullptr) {
        UINT clearFlags = 0;
        if (beginDesc->depthStencilAttachment.depthLoadAction == VGPULoadAction_Clear)
        {
            clearFlags |= D3D11_CLEAR_DEPTH;
        }

        if (beginDesc->depthStencilAttachment.stencilLoadAction == VGPULoadAction_Clear)
        {
            clearFlags |= D3D11_CLEAR_STENCIL;
        }

        d3d11.commandBuffers[commandBuffer].context->ClearDepthStencilView(
            framebuffer.depthStencilAttachment,
            clearFlags,
            beginDesc->depthStencilAttachment.clearDepth, beginDesc->depthStencilAttachment.clearStencil
        );
    }

    d3d11.commandBuffers[commandBuffer].context->OMSetRenderTargets(
        framebuffer.colorAttachmentCount,
        framebuffer.colorAttachments,
        framebuffer.depthStencilAttachment
    );

    /* set viewport and scissor rect to cover render target size */
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (FLOAT)framebuffer.width;
    viewport.Height = (FLOAT)framebuffer.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D11_RECT scissorRect;
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = (LONG)framebuffer.width;
    scissorRect.bottom = (LONG)framebuffer.height;

    d3d11.commandBuffers[commandBuffer].context->RSSetViewports(1, &viewport);
    d3d11.commandBuffers[commandBuffer].context->RSSetScissorRects(1, &scissorRect);
}

static void d3d11_endRenderPass(VGPUCommandBuffer commandBuffer) {

}

/* Driver functions */
static bool d3d11_isSupported(void) {
    if (d3d11.available_initialized) {
        return d3d11.available;
    }
    d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d11.dxgi.instance = LoadLibraryA("dxgi.dll");
    if (!d3d11.dxgi.instance) {
        return false;
    }

    d3d11.dxgi.CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgi.instance, "CreateDXGIFactory1");
    if (!d3d11.dxgi.CreateDXGIFactory1)
    {
        return false;
    }

    d3d11.dxgi.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgi.instance, "CreateDXGIFactory2");
    if (!d3d11.dxgi.CreateDXGIFactory2)
    {
        return false;
    }
    d3d11.dxgi.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgi.instance, "DXGIGetDebugInterface1");

    d3d11.instance = LoadLibraryA("d3d11.dll");
    if (!d3d11.instance) {
        return false;
    }

    d3d11.D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.instance, "D3D11CreateDevice");
    if (!d3d11.D3D11CreateDevice) {
        return false;
    }
#endif

    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = vgpuD3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        s_featureLevels,
        _vgpu_count_of(s_featureLevels),
        D3D11_SDK_VERSION,
        NULL,
        NULL,
        NULL
    );

    if (FAILED(hr))
    {
        return false;
    }

    d3d11.available = true;
    return true;
}

static VGPUGraphicsContext* d3d11_createContext(void) {
    static VGPUGraphicsContext graphicsContext = { nullptr };
    ASSIGN_DRIVER(d3d11);
    return &graphicsContext;
}

vgpu_driver d3d11_driver = {
    VGPUBackendType_D3D11,
    d3d11_isSupported,
    d3d11_createContext
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
