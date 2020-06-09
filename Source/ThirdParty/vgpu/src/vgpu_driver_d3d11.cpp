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

#if defined(VGPU_DRIVER_D3D11)
#define D3D11_NO_HELPERS
#include <d3d11_1.h>
#include "vgpu_d3d_common.h"
#include <cstdio>

struct D3D11Texture {
    enum { MAX_COUNT = 8192 };

    DXGI_FORMAT format;
    ID3D11Resource* handle;
};

struct D3D11Swapchain {
    UINT syncInterval;
    UINT presentFlags;
    IDXGISwapChain1* handle;
    VGPUTexture backbuffer;
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
    ID3DUserDefinedAnnotation* d3dAnnotation;
    D3D_FEATURE_LEVEL d3dFeatureLevel;
    D3D11Swapchain swapchains[16];
    bool isLost;

    VGPUPool<D3D11Texture, D3D11Texture::MAX_COUNT> textures;
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
static void d3d11_UpdateSwapChain(D3D11Swapchain* swapchain, const VGPUSwapchainDescription* desc);

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
        VHR(temp_d3d_context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&d3d11.d3dAnnotation));
        temp_d3d_context->Release();
        temp_d3d_device->Release();
    }

    adapter->Release();

    // Init pools.
    d3d11.textures.init();

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

    SAFE_RELEASE(d3d11.d3dContext);
    SAFE_RELEASE(d3d11.d3dAnnotation);

    ULONG refCount = d3d11.d3dDevice->Release();
#if !defined(NDEBUG)
    if (refCount > 0)
    {
        //gpuLog(GPULogLevel_Error, "Direct3D11: There are %d unreleased references left on the device", refCount);

        ID3D11Debug* d3dDebug = NULL;
        if (SUCCEEDED(d3d11.d3dDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug)))
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
    SAFE_RELEASE(swapchain->handle);
}

static void d3d11_UpdateSwapChain(D3D11Swapchain* swapchain, const VGPUSwapchainDescription* desc)
{
    if (swapchain->backbuffer.id != VGPU_INVALID_ID) {
        d3d11_DestroySwapChain(swapchain);
    }

    if (swapchain->handle) {
        /* TODO: Resize */
    }
    else {
        swapchain->syncInterval = _vgpuGetSyncInterval(desc->presentMode);
        swapchain->presentFlags = 0;

        if (desc->presentMode == VGPUPresentMode_Immediate
            && d3d11.factory_caps & DXGIFACTORY_CAPS_TEARING) {
            swapchain->presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        swapchain->handle = vgpu_d3d_create_swapchain(
            d3d11.factory,
            d3d11.d3dDevice,
            d3d11.factory_caps,
            desc->windowHandle,
            desc->width, desc->height,
            desc->colorFormat,
            d3d11.backbufferCount, /* 3 for triple buffering */
            desc->fullscreen
        );
    }

    ID3D11Texture2D* backbuffer;
    VHR(swapchain->handle->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer));

    D3D11_TEXTURE2D_DESC d3d_desc;
    backbuffer->GetDesc(&d3d_desc);

    VGPUTextureDescription textureDesc = {};
    textureDesc.width = d3d_desc.Width;
    textureDesc.height = d3d_desc.Height;
    textureDesc.depth = 1u;
    textureDesc.mipLevels = d3d_desc.MipLevels;
    textureDesc.arrayLayers = d3d_desc.ArraySize;
    textureDesc.format = desc->colorFormat;
    textureDesc.type = VGPUTextureType_2D;
    textureDesc.usage = VGPUTextureUsage_RenderTarget;
    textureDesc.sampleCount = (VGPUTextureSampleCount)d3d_desc.SampleDesc.Count;
    textureDesc.externalHandle = backbuffer;
    swapchain->backbuffer = vgpuCreateTexture(&textureDesc);
}

/* Texture */
VGPUTexture d3d11_allocTexture(void) {
    if (d3d11.textures.isFull()) {
        //vgpuLogError("Not enough free texture slots.");
        return { VGPU_INVALID_ID };
    }

    const int id = d3d11.textures.alloc();
    VGPU_ASSERT(id >= 0);

    D3D11Texture& texture = d3d11.textures[id];
    texture.handle = nullptr;
    return { (uint32_t)id };
}

static bool d3d11_initTexture(VGPUTexture handle, const VGPUTextureDescription* desc)
{
    D3D11Texture& texture = d3d11.textures[handle.id];
    // If depth and either ua or sr, set to typeless
    if (vgpuIsDepthOrStencilFormat(desc->format)
        && ((desc->usage & (VGPUTextureUsage_Sampled | VGPUTextureUsage_Storage)) != 0))
    {
        texture.format = _vgpuGetTypelessFormatFromDepthFormat(desc->format);
    }
    else {
        texture.format = _vgpuGetDXGIFormat(desc->format);
    }

    HRESULT hr = S_OK;
    if (desc->externalHandle) {
        texture.handle = (ID3D11Resource*)desc->externalHandle;
    }
    else {
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT bindFlags = 0;
        UINT CPUAccessFlags = 0;
        UINT miscFlags = 0;
        UINT arraySizeMultiplier = 1;
        if (desc->type == VGPUTextureType_Cube) {
            arraySizeMultiplier = 6;
            miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        if (desc->usage & VGPUTextureUsage_Sampled) {
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (desc->usage & VGPUTextureUsage_Storage) {
            bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (desc->usage & VGPUTextureUsage_RenderTarget) {
            if (vgpuIsDepthOrStencilFormat(desc->format)) {
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

        if (desc->type == VGPUTextureType_2D || desc->type == VGPUTextureType_Cube) {
            D3D11_TEXTURE2D_DESC d3d11_desc = {};
            d3d11_desc.Width = desc->width;
            d3d11_desc.Height = desc->height;
            d3d11_desc.MipLevels = desc->mipLevels;
            d3d11_desc.ArraySize = desc->arrayLayers * arraySizeMultiplier;
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
        else if (desc->type == VGPUTextureType_3D) {
            /*const D3D11_TEXTURE3D_DESC d3d11_desc = {
                .Width = desc->width,
                .Height = desc->height,
                .Depth = desc->depth,
                .MipLevels = desc->mipLevels,
                .Format = result->format,
                .Usage = usage,
                .BindFlags = bindFlags,
                .CPUAccessFlags = CPUAccessFlags,
                .MiscFlags = miscFlags
            };

            hr = ID3D11Device_CreateTexture3D(
                renderer->device,
                &d3d11_desc,
                NULL,
                (ID3D11Texture3D**)&result->handle
            );*/
        }

        if (FAILED(hr)) {
            return false;
        }
    }

    return true;
}

static void d3d11_destroyTexture(VGPUTexture handle) {
    D3D11Texture& texture = d3d11.textures[handle.id];
    SAFE_RELEASE(texture.handle);
    d3d11.textures.dealloc(handle.id);
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
