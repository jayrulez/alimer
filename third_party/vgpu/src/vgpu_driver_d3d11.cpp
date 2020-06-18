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
#include <stdio.h>

#ifdef _DEBUG
static const IID D3D11_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
#endif

struct d3d11_texture {
    enum { MAX_COUNT = 8192 };

    ID3D11Resource* handle;
    DXGI_FORMAT dxgi_format;

    TextureType type;
    uint32_t width;
    uint32_t height;
    uint32_t depth_or_layers;
    uint32_t mipLevels;
    uint32_t sample_count;
};

struct d3d11_framebuffer {
    uint32_t num_rtvs;
    ID3D11RenderTargetView* rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView* dsv;

    uint32_t width;
    uint32_t height;
    uint32_t layers;
};

struct D3D11Buffer {
    enum { MAX_COUNT = 8192 };

    ID3D11Buffer* handle;
};

struct D3D11Swapchain {
    uint32_t width;
    uint32_t height;
    PixelFormat depthStencilFormat;
    IDXGISwapChain1* handle;
    Texture backbuffer;
    Texture depthStencilTexture;
    vgpu_framebuffer framebuffer;
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

    Caps caps;

    IDXGIFactory2* factory;
    uint32_t factory_caps;

    ID3D11Device1* device;
    ID3D11DeviceContext1* context;
    ID3DUserDefinedAnnotation* d3d_annotation;
    D3D_FEATURE_LEVEL feature_level;

    uint8_t num_backbuffers = 2u;

    Pool<d3d11_texture, d3d11_texture::MAX_COUNT> textures;
    Pool<D3D11Buffer, D3D11Buffer::MAX_COUNT> buffers;
    bool isLost;

    D3D11Swapchain* mainSwapchain;    // Guaranteed to be == swapchains[0]
    D3D11Swapchain  swapchains[32];   // Main swapchain, followed by all secondary swapchains.
} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define _vgpu_CreateDXGIFactory1 d3d11.dxgi.CreateDXGIFactory1
#   define _vgpu_CreateDXGIFactory2 d3d11.dxgi.CreateDXGIFactory2
#   define _vgpu_DXGIGetDebugInterface1 d3d11.dxgi.DXGIGetDebugInterface1
#   define _vgpu_D3D11CreateDevice d3d11.D3D11CreateDevice
#else
#   define _vgpu_CreateDXGIFactory1 CreateDXGIFactory1
#   define _vgpu_CreateDXGIFactory2 CreateDXGIFactory2
#   define _vgpu_DXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define _vgpu_D3D11CreateDevice D3D11CreateDevice
#endif

static bool _vgpu_d3d11_sdk_layers_available() {
    HRESULT hr = _vgpu_D3D11CreateDevice(
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

static bool d3d11_create_factory(bool validation)
{
    SAFE_RELEASE(d3d11.factory);

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    bool debugDXGI = false;

    if (validation)
    {
        IDXGIInfoQueue* dxgi_info_queue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 &&
            SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue))))
#else
        if (SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue)())
#endif
        {
            debugDXGI = true;
            hr = _vgpu_CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&d3d11.factory));
            if (FAILED(hr)) {
                return false;
            }

            VHR(dxgi_info_queue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(dxgi_info_queue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(dxgi_info_queue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            };

            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgi_info_queue->AddStorageFilterEntries(D3D_DXGI_DEBUG_DXGI, &filter);
            dxgi_info_queue->Release();
        }
    }

    if (!debugDXGI)
#endif
    {
        hr = _vgpu_CreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory));
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
        HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory4));
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
        HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory5));
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

static IDXGIAdapter1* d3d11GetAdapter(bool lowPower)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6;
    HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory6));
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = lowPower ? DXGI_GPU_PREFERENCE_MINIMUM_POWER : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;

        for (uint32_t i = 0;
            DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(&adapter)); i++)
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
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != d3d11.factory->EnumAdapters1(i, &adapter); i++)
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

static bool d3d11_swapchain_init(D3D11Swapchain* swapchain, void* windowHandle, PixelFormat colorFormat, PixelFormat depthStencilFormat);
static void d3d11_swapchain_destroy(D3D11Swapchain* swapchain);

static bool d3d11_init(void* windowHandle, InitFlags flags) {
    const bool validation =
        (flags & InitFlags::DebugOutput) != InitFlags::None
        || (flags & InitFlags::GPUBasedValidation) != InitFlags::None;

    if (!d3d11_create_factory(validation)) {
        return false;
    }

    const bool lowPower = (flags & InitFlags::LowPower) != InitFlags::None;
    IDXGIAdapter1* dxgiAdapter = d3d11GetAdapter(lowPower);

    /* Create d3d11 device */
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (validation) {

            if (_vgpu_d3d11_sdk_layers_available())
            {
                // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
            }
#if defined(_DEBUG)
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }
#endif
        }

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
        if (dxgiAdapter)
        {
            hr = _vgpu_D3D11CreateDevice(
                dxgiAdapter,
                D3D_DRIVER_TYPE_UNKNOWN,
                nullptr,
                creationFlags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &d3d11.feature_level,
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
            hr = _vgpu_D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                nullptr,
                creationFlags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &d3d11.feature_level,
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
                d3d_info_queue->Release();
            }

            d3d_debug->Release();
        }

        VHR(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3d11.device)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.context)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.d3d_annotation)));
        temp_d3d_context->Release();
        temp_d3d_device->Release();
    }

    if (windowHandle) {
        if (!d3d11_swapchain_init(&d3d11.swapchains[0], windowHandle, PixelFormat::BGRA8Unorm, PixelFormat::Depth32Float)) {
            return false;
        }

        d3d11.mainSwapchain = &d3d11.swapchains[0];
    }

    /* Init caps first. */
    {
        DXGI_ADAPTER_DESC1 desc;
        VHR(dxgiAdapter->GetDesc1(&desc));

        d3d11.caps.backendType = BackendType::D3D11;
        d3d11.caps.vendorId = desc.VendorId;
        d3d11.caps.deviceId = desc.DeviceId;

        // Features
        d3d11.caps.features.independentBlend = true;
        d3d11.caps.features.computeShader = true;
        d3d11.caps.features.tessellationShader = true;
        d3d11.caps.features.logicOp = true;
        d3d11.caps.features.multiViewport = true;
        d3d11.caps.features.indexTypeUint32 = true;
        d3d11.caps.features.multiDrawIndirect = true;
        d3d11.caps.features.fillModeNonSolid = true;
        d3d11.caps.features.samplerAnisotropy = true;
        d3d11.caps.features.textureCompressionETC2 = false;
        d3d11.caps.features.textureCompressionASTC_LDR = false;
        d3d11.caps.features.textureCompressionBC = true;
        d3d11.caps.features.textureCubeArray = true;
        d3d11.caps.features.raytracing = false;

        // Limits
        d3d11.caps.limits.maxVertexAttributes = kMaxVertexAttributes;
        d3d11.caps.limits.maxVertexBindings = kMaxVertexAttributes;
        d3d11.caps.limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
        d3d11.caps.limits.maxVertexBindingStride = kMaxVertexBufferStride;

        //caps.limits.maxTextureDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        d3d11.caps.limits.maxTextureDimension2D = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        d3d11.caps.limits.maxTextureDimension3D = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        d3d11.caps.limits.maxTextureDimensionCube = D3D11_REQ_TEXTURECUBE_DIMENSION;
        d3d11.caps.limits.maxTextureArrayLayers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        d3d11.caps.limits.maxColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
        d3d11.caps.limits.maxUniformBufferSize = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        d3d11.caps.limits.minUniformBufferOffsetAlignment = 256u;
        d3d11.caps.limits.maxStorageBufferSize = UINT32_MAX;
        d3d11.caps.limits.minStorageBufferOffsetAlignment = 16;
        d3d11.caps.limits.maxSamplerAnisotropy = D3D11_MAX_MAXANISOTROPY;
        d3d11.caps.limits.maxViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        d3d11.caps.limits.maxViewportWidth = D3D11_VIEWPORT_BOUNDS_MAX;
        d3d11.caps.limits.maxViewportHeight = D3D11_VIEWPORT_BOUNDS_MAX;
        d3d11.caps.limits.maxTessellationPatchSize = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        d3d11.caps.limits.pointSizeRangeMin = 1.0f;
        d3d11.caps.limits.pointSizeRangeMax = 1.0f;
        d3d11.caps.limits.lineWidthRangeMin = 1.0f;
        d3d11.caps.limits.lineWidthRangeMax = 1.0f;
        d3d11.caps.limits.maxComputeSharedMemorySize = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        d3d11.caps.limits.maxComputeWorkGroupCountX = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d11.caps.limits.maxComputeWorkGroupCountY = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d11.caps.limits.maxComputeWorkGroupCountZ = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d11.caps.limits.maxComputeWorkGroupInvocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        d3d11.caps.limits.maxComputeWorkGroupSizeX = D3D11_CS_THREAD_GROUP_MAX_X;
        d3d11.caps.limits.maxComputeWorkGroupSizeY = D3D11_CS_THREAD_GROUP_MAX_Y;
        d3d11.caps.limits.maxComputeWorkGroupSizeZ = D3D11_CS_THREAD_GROUP_MAX_Z;

        /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
        for (uint32_t format = (static_cast<uint32_t>(PixelFormat::Undefined) + 1); format < static_cast<uint32_t>(PixelFormat::Count); format++)
        {
            const DXGI_FORMAT dxgiFormat = ToDXGIFormat((PixelFormat)format);
            if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
                continue;

            UINT formatSupport = 0;
            if (FAILED(d3d11.device->CheckFormatSupport(dxgiFormat, &formatSupport))) {
                continue;
            }

            /*D3D12_FEATURE_DATA_FORMAT_SUPPORT support;
            support.Format = ToDXGIFormat((PixelFormat)format);

            if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
                continue;

            VHR(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)));*/
        }
    }

    SAFE_RELEASE(dxgiAdapter);

    return true;
}

static void d3d11_shutdown(void) {
    for (uint32_t i = 1; i < _countof(d3d11.swapchains); i++)
    {
        if (!d3d11.swapchains[i].handle)
            continue;

        d3d11_swapchain_destroy(&d3d11.swapchains[i]);
    }

    if (d3d11.mainSwapchain) {
        d3d11_swapchain_destroy(d3d11.mainSwapchain);
    }

    /*if (d3d11.main_swapchain) {
        vgpu_swapchain_destroy(d3d11.main_swapchain);
    }*/

    SAFE_RELEASE(d3d11.context);
    SAFE_RELEASE(d3d11.d3d_annotation);

    ULONG refCount = d3d11.device->Release();
#if !defined(NDEBUG)
    if (refCount > 0)
    {
        vgpu_log_error("Direct3D11: There are %d unreleased references left on the device", refCount);

        ID3D11Debug* d3d11_debug = NULL;
        if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&d3d11_debug))))
        {
            d3d11_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            d3d11_debug->Release();
        }
    }
#else
    (void)refCount; // avoid warning
#endif

    SAFE_RELEASE(d3d11.factory);

#ifdef _DEBUG
    IDXGIDebug1* dxgi_debug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    if (d3d11.dxgi.DXGIGetDebugInterface1 &&
        SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug1))))
#else
    if (SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug1))))
#endif
    {
        dxgi_debug1->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        dxgi_debug1->Release();
    }
#endif

    memset(&d3d11, 0, sizeof(d3d11));
}

static const Caps* d3d11_getCaps(void) {
    return &d3d11.caps;
}

static bool d3d11_frame_begin(void) {
    return true;
}

static void d3d11_frame_end(void) {

    // Present sequentally all swap chains.
    HRESULT hr = S_OK;

    UINT noVSyncPrefentFlags = 0u;
    if (d3d11.factory_caps & DXGIFACTORY_CAPS_TEARING) {
        noVSyncPrefentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    for (uint32_t i = 1; i < _countof(d3d11.swapchains); i++)
    {
        if (!d3d11.swapchains[i].handle)
            continue;

        // Secondary windows are presented without vsync.
        hr = d3d11.swapchains[i].handle->Present(0, noVSyncPrefentFlags);

        d3d11.isLost = d3dIsLost(hr);
    }

    if (d3d11.mainSwapchain) {
        // Main swap chain: Present with vsync
        hr = d3d11.mainSwapchain->handle->Present(1, 0);

        d3d11.isLost = d3dIsLost(hr);
    }

    if (d3d11.isLost)
    {
#ifdef _DEBUG
        //char buff[64] = {};
        //sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
        //    static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3d11.d3dDevice->GetDeviceRemovedReason() : hr));
        //OutputDebugStringA(buff);
#endif
    }

    VHR(hr);
}

static inline D3D11_USAGE d3d11UsageFromMemoryUsage(ResourceMemoryUsage usage)
{
    switch (usage)
    {
    case ResourceMemoryUsage::GpuOnly: return D3D11_USAGE_DEFAULT;
    case ResourceMemoryUsage::CpuOnly: return D3D11_USAGE_STAGING;
    case ResourceMemoryUsage::CpuToGpu: return D3D11_USAGE_DYNAMIC;
    case ResourceMemoryUsage::GpuToCpu: return D3D11_USAGE_STAGING;
    default:
        VGPU_UNREACHABLE();
        return D3D11_USAGE_DEFAULT;
    }
}

static inline D3D11_CPU_ACCESS_FLAG d3d11CpuAccessFromMemoryUsage(ResourceMemoryUsage usage)
{
    switch (usage)
    {
    case ResourceMemoryUsage::GpuOnly: return (D3D11_CPU_ACCESS_FLAG)0;
    case ResourceMemoryUsage::CpuOnly: return (D3D11_CPU_ACCESS_FLAG)(D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);
    case ResourceMemoryUsage::CpuToGpu: return (D3D11_CPU_ACCESS_FLAG)(D3D11_CPU_ACCESS_WRITE);
    case ResourceMemoryUsage::GpuToCpu: return (D3D11_CPU_ACCESS_FLAG)(D3D11_CPU_ACCESS_READ);
    default:
        VGPU_UNREACHABLE();
        return (D3D11_CPU_ACCESS_FLAG)0;
    }
}

/* Texture */
static Texture d3d11_texture_create(const TextureDesc& desc) {
    if (d3d11.textures.isFull()) {
        vgpu_log_error("Not enough free texture slots.");
        return kInvalidTexture;
    }
    const int id = d3d11.textures.alloc();
    VGPU_ASSERT(id >= 0);

    d3d11_texture& texture = d3d11.textures[id];
    texture.type = desc.textureType;
    texture.width = desc.width;
    texture.height = desc.height;
    texture.depth_or_layers = desc.depth;
    texture.mipLevels = desc.mipLevels;
    texture.sample_count = desc.sampleCount;

    texture.dxgi_format = ToDXGIFormatWitUsage(desc.format, desc.usage);

    HRESULT hr = S_OK;
    if (desc.externalHandle) {
        texture.handle = (ID3D11Resource*)desc.externalHandle;
    }
    else {
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT bindFlags = 0;
        UINT CPUAccessFlags = 0;
        UINT miscFlags = 0;
        if (desc.textureType == TextureType::TypeCube) {
            miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        if ((desc.usage & TextureUsage::Sampled) != TextureUsage::None) {
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if ((desc.usage & TextureUsage::Storage) != TextureUsage::None) {
            bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if ((desc.usage & TextureUsage::RenderTarget) != TextureUsage::None) {
            if (isDepthStencilFormat(desc.format)) {
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

        if (desc.textureType == TextureType::Type2D || desc.textureType == TextureType::TypeCube) {
            D3D11_TEXTURE2D_DESC d3d11_desc = {};
            d3d11_desc.Width = desc.width;
            d3d11_desc.Height = desc.height;
            d3d11_desc.MipLevels = desc.mipLevels;
            d3d11_desc.ArraySize = desc.arrayLayers;
            d3d11_desc.Format = texture.dxgi_format;
            d3d11_desc.SampleDesc.Count = 1;
            d3d11_desc.SampleDesc.Quality = 0;
            d3d11_desc.Usage = usage;
            d3d11_desc.BindFlags = bindFlags;
            d3d11_desc.CPUAccessFlags = CPUAccessFlags;
            d3d11_desc.MiscFlags = miscFlags;

            hr = d3d11.device->CreateTexture2D(
                &d3d11_desc,
                NULL,
                (ID3D11Texture2D**)&texture.handle
            );
        }
        else if (desc.textureType == TextureType::Type3D) {
            D3D11_TEXTURE3D_DESC d3d11_desc = {};
            d3d11_desc.Width = desc.width;
            d3d11_desc.Height = desc.height;
            d3d11_desc.Depth = desc.depth;
            d3d11_desc.MipLevels = desc.mipLevels;
            d3d11_desc.Format = texture.dxgi_format;
            d3d11_desc.Usage = usage;
            d3d11_desc.BindFlags = bindFlags;
            d3d11_desc.CPUAccessFlags = CPUAccessFlags;
            d3d11_desc.MiscFlags = miscFlags;

            hr = d3d11.device->CreateTexture3D(
                &d3d11_desc,
                NULL,
                (ID3D11Texture3D**)&texture.handle
            );
        }
    }

    return { (uint32_t)id };
}

static void d3d11_texture_destroy(Texture handle) {
    d3d11_texture& texture = d3d11.textures[handle.id];
    SAFE_RELEASE(texture.handle);
    d3d11.textures.dealloc(handle.id);
}

static uint32_t d3d11_texture_get_width(Texture handle, uint32_t mipLevel) {
    d3d11_texture& texture = d3d11.textures[handle.id];
    return _vgpu_max(1, texture.width >> mipLevel);
}

static uint32_t d3d11_texture_get_height(Texture handle, uint32_t mipLevel) {
    d3d11_texture& texture = d3d11.textures[handle.id];
    return _vgpu_max(1, texture.height >> mipLevel);
}

/* Framebuffer */
static vgpu_framebuffer d3d11_framebuffer_create(const vgpu_framebuffer_info* info) {
    d3d11_framebuffer* framebuffer = VGPU_ALLOC_HANDLE(d3d11_framebuffer);

#if TODO
    for (uint32_t i = 0; i < _vgpu_count_of(info->color_attachments) && info->color_attachments[i].texture.id; i++, framebuffer->num_rtvs++) {
        d3d11_texture& texture = d3d11.textures[info->color_attachments[i].texture.id];
        uint32_t mip_level = info->color_attachments[i].level;
        uint32_t slice = info->color_attachments[i].slice;

        /* TODO: Understand SwapChain RTV format when using RGBA8UNormSrgb */
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        memset(&rtvDesc, 0, sizeof(rtvDesc));
        rtvDesc.Format = texture.dxgi_format;
        switch (texture.type)
        {
        case TextureType::Type2D:

            if (texture.sample_count <= 1)
            {
                if (texture.depth_or_layers > 1)
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.MipSlice = mip_level;
                    rtvDesc.Texture2DArray.FirstArraySlice = slice;
                    rtvDesc.Texture2DArray.ArraySize = texture.depth_or_layers;
                }
                else
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = mip_level;
                }
            }
            else
            {
                if (texture.depth_or_layers > 1)
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    rtvDesc.Texture2DMSArray.FirstArraySlice = slice;
                    rtvDesc.Texture2DMSArray.ArraySize = texture.depth_or_layers;
                }
                else
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
            }

            break;
        case TextureType::Type3D:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = mip_level;
            rtvDesc.Texture3D.FirstWSlice = slice;
            rtvDesc.Texture3D.WSize = (UINT)-1;
            break;
        case TextureType::TypeCube:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = mip_level;
            rtvDesc.Texture2DArray.FirstArraySlice = slice;
            rtvDesc.Texture2DArray.ArraySize = texture.depth_or_layers;
            break;

        default:
            break;
        }

        VHR(d3d11.device->CreateRenderTargetView(
            texture.handle,
            &rtvDesc,
            &framebuffer->rtvs[framebuffer->num_rtvs])
        );
    }

    if (info->depth_stencil_attachment.texture.id)
    {
        d3d11_texture& texture = d3d11.textures[info->depth_stencil_attachment.texture.id];
        uint32_t mip_level = info->depth_stencil_attachment.level;
        uint32_t slice = info->depth_stencil_attachment.slice;

        D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_dsv_desc;
        memset(&d3d11_dsv_desc, 0, sizeof(d3d11_dsv_desc));
        d3d11_dsv_desc.Format = texture.dxgi_format;

        switch (texture.type)
        {
        case TextureType::Type2D:

            if (texture.sample_count <= 1)
            {
                if (texture.depth_or_layers > 1)
                {
                    d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    d3d11_dsv_desc.Texture2DArray.MipSlice = mip_level;
                    d3d11_dsv_desc.Texture2DArray.FirstArraySlice = slice;
                    d3d11_dsv_desc.Texture2DArray.ArraySize = texture.depth_or_layers;
                }
                else
                {
                    d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    d3d11_dsv_desc.Texture2D.MipSlice = mip_level;
                }
            }
            else
            {
                if (texture.depth_or_layers > 1)
                {
                    d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    d3d11_dsv_desc.Texture2DMSArray.FirstArraySlice = slice;
                    d3d11_dsv_desc.Texture2DMSArray.ArraySize = texture.depth_or_layers;
                }
                else
                {
                    d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
            }

            break;
        case TextureType::TypeCube:
            d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            d3d11_dsv_desc.Texture2DArray.MipSlice = mip_level;
            d3d11_dsv_desc.Texture2DArray.FirstArraySlice = slice;
            d3d11_dsv_desc.Texture2DArray.ArraySize = texture.depth_or_layers;
            break;

        default:
            break;
        }

        VHR(d3d11.device->CreateDepthStencilView(
            texture.handle,
            &d3d11_dsv_desc,
            &framebuffer->dsv)
        );
    }
#endif // TODO


    return (vgpu_framebuffer)framebuffer;
}

static void d3d11_framebuffer_destroy(vgpu_framebuffer handle) {
    d3d11_framebuffer* framebuffer = (d3d11_framebuffer*)handle;
    for (uint32_t i = 0; i < framebuffer->num_rtvs; i++) {
        SAFE_RELEASE(framebuffer->rtvs[i]);
    }

    if (framebuffer->dsv) {
        SAFE_RELEASE(framebuffer->dsv);
    }

    VGPU_FREE(framebuffer);
}

/* Buffer */
D3D11_BIND_FLAG d3d11BindFlagsFromBufferUsage(BufferUsage usage)
{
    if ((usage & BufferUsage::Uniform) != BufferUsage::None)
    {
        /* Exclusive usage */
        return D3D11_BIND_CONSTANT_BUFFER;
    }

    uint32_t result = 0;
    if ((usage & BufferUsage::Vertex) != BufferUsage::None)
        result |= D3D11_BIND_VERTEX_BUFFER;
    if ((usage & BufferUsage::Index) != BufferUsage::None)
        result |= D3D11_BIND_INDEX_BUFFER;
    if ((usage & BufferUsage::Storage) != BufferUsage::None)
        result |= D3D11_BIND_UNORDERED_ACCESS;

    /* TODO: StorageRead? Sampled? */
    //if (type & DESCRIPTOR_TYPE_BUFFER)
    //    result |= D3D11_BIND_SHADER_RESOURCE;

    return (D3D11_BIND_FLAG)result;
}

static Buffer d3d11_createBuffer(const BufferDesc* desc) {
    if (d3d11.buffers.isFull()) {
        vgpu_log_error("Not enough free buffer slots.");
        return kInvalidBuffer;
    }
    const int id = d3d11.buffers.alloc();
    VGPU_ASSERT(id >= 0);

    D3D11Buffer& buffer = d3d11.buffers[id];

    uint64_t allocationSize = desc->size;
    // Align the buffer size to multiples of 256
    if ((desc->usage & BufferUsage::Uniform) != BufferUsage::None)
    {
        allocationSize = RountUp64(allocationSize, d3d11.caps.limits.minUniformBufferOffsetAlignment);
    }

    D3D11_BUFFER_DESC d3d11Desc = {};
    d3d11Desc.ByteWidth = (UINT)allocationSize;
    d3d11Desc.Usage = d3d11UsageFromMemoryUsage(desc->memoryUsage);
    d3d11Desc.BindFlags = d3d11BindFlagsFromBufferUsage(desc->usage);
    d3d11Desc.CPUAccessFlags = d3d11CpuAccessFromMemoryUsage(desc->memoryUsage);

    if ((desc->usage & BufferUsage::Indirect) != BufferUsage::None)
        d3d11Desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

    d3d11Desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA subresourceData = {};
    if (desc->content != nullptr) {
        subresourceData.pSysMem = desc->content;
    }

    HRESULT hr = d3d11.device->CreateBuffer(
        &d3d11Desc,
        desc->content == nullptr ? nullptr : &subresourceData,
        &buffer.handle);
    if (FAILED(hr)) {
        d3d11.buffers.dealloc(id);
        return kInvalidBuffer;
    }

    return { (uint32_t)id };
}

static void d3d11_destroyBuffer(Buffer handle) {
    D3D11Buffer& buffer = d3d11.buffers[handle.id];
    SAFE_RELEASE(buffer.handle);
    d3d11.buffers.dealloc(handle.id);
}

/* Swapchain */
static bool d3d11_swapchain_init(D3D11Swapchain* swapchain, void* windowHandle, PixelFormat colorFormat, PixelFormat depthStencilFormat) {

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window = static_cast<HWND>(windowHandle);
    if (!IsWindow(window)) {
        vgpu_log_error("Invalid HWND handle");
        return false;
    }

    RECT rc;
    GetClientRect(window, &rc);
    swapchain->width = uint32_t(rc.right - rc.left);
    swapchain->height = uint32_t(rc.bottom - rc.top);
#else
    IUnknown* window = static_cast<IUnknown*>(handle);
#endif

    swapchain->handle = vgpu_d3d_create_swapchain(
        d3d11.factory,
        d3d11.factory_caps,
        d3d11.device,
        windowHandle,
        swapchain->width, swapchain->height,
        colorFormat,
        d3d11.num_backbuffers,  /* 3 for triple buffering */
        true
    );

    return true;
}

static void d3d11_swapchain_destroy(D3D11Swapchain* swapchain) {
    //vgpu_framebuffer_destroy(swapchain->framebuffer);
    destroyTexture(swapchain->backbuffer);
    destroyTexture(swapchain->depthStencilTexture);
    SAFE_RELEASE(swapchain->handle);
}

static void d3d11_swapchain_resize(D3D11Swapchain* swapchain, uint32_t width, uint32_t height) {

    if (swapchain->handle) {
        UINT flags = 0;
        if (d3d11.factory_caps & DXGIFACTORY_CAPS_TEARING)
        {
            flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        HRESULT hr = swapchain->handle->ResizeBuffers(
            d3d11.num_backbuffers,
            width,
            height,
            DXGI_FORMAT_UNKNOWN,
            flags
        );

        if (d3dIsLost(hr))
        {
            d3d11.isLost = true;
            return;
        }
    }
    else {

    }

    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    VHR(swapchain->handle->GetDesc1(&swapchain_desc));
    swapchain->width = swapchain_desc.Width;
    swapchain->height = swapchain_desc.Height;

    ID3D11Texture2D* backbuffer;
    VHR(swapchain->handle->GetBuffer(0, IID_PPV_ARGS(&backbuffer)));

    TextureDesc backbufferTextureDesc = {};
    backbufferTextureDesc.format = PixelFormat::BGRA8Unorm;
    backbufferTextureDesc.width = swapchain->width;
    backbufferTextureDesc.height = swapchain->height;
    backbufferTextureDesc.usage = TextureUsage::RenderTarget;
    backbufferTextureDesc.externalHandle = backbuffer;

    swapchain->backbuffer = createTexture(backbufferTextureDesc);

    /*if (swapchain->depthStencilFormat != PixelFormat::Undefined) {
        const vgpu_texture_info depth_texture_info = {
            .width = swapchain->width,
            .height = swapchain->height,
            .array_layers = 1u,
            .mip_levels = 1,
            .format = swapchain->depth_stencil_format,
            .type = VGPU_TEXTURE_TYPE_2D,
            .usage = VGPU_TEXTURE_USAGE_RENDER_TARGET,
            .sample_count = 1u,
        };

        swapchain->depth_stencil_texture = vgpu_texture_create(&depth_texture_info);
    }

    // Create framebuffer 
    vgpu_framebuffer_info fbo_info;
    memset(&fbo_info, 0, sizeof(fbo_info));
    fbo_info.color_attachments[0].texture = swapchain->backbuffer;

    if (swapchain->depthStencilFormat != PixelFormat::Undefined) {
        fbo_info.depth_stencil_attachment.texture = swapchain->depth_stencil_texture;
    }

    swapchain->framebuffer = vgpu_framebuffer_create(&fbo_info);*/
}

/* Commands */
static void d3d11_insertDebugMarker(const char* name)
{
    wchar_t wideName[128];
    int num = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (num > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, name, -1, &wideName[0], num);
    }

    if (num > 0) {
        d3d11.d3d_annotation->SetMarker(wideName);
    }
}

static void d3d11_pushDebugGroup(const char* name)
{
    wchar_t wideName[128];
    int num = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (num > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, name, -1, &wideName[0], num);
    }

    if (num > 0) {
        d3d11.d3d_annotation->BeginEvent(wideName);
    }
}

static void d3d11_popDebugGroup(void)
{
    d3d11.d3d_annotation->EndEvent();
}

static void d3d11_beginRenderPass(const RenderPassDesc* desc) {
#if TODO
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
#endif // TODO
}

static void d3d11_endRenderPass(void) {

}


/* Driver functions */
static bool d3d11_is_supported(void) {
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

    HRESULT hr = _vgpu_D3D11CreateDevice(
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
};

static vgpu_context* d3d11_create_context(void) {
    static vgpu_context context = { 0 };
    context.init = d3d11_init;
    context.shutdown = d3d11_shutdown;
    context.getCaps = d3d11_getCaps;
    context.frame_begin = d3d11_frame_begin;
    context.frame_end = d3d11_frame_end;
    context.texture_create = d3d11_texture_create;
    context.texture_destroy = d3d11_texture_destroy;
    context.texture_get_width = d3d11_texture_get_width;
    context.texture_get_height = d3d11_texture_get_height;
    context.createBuffer = d3d11_createBuffer;
    context.destroyBuffer = d3d11_destroyBuffer;
    /* */
    context.insertDebugMarker = d3d11_insertDebugMarker;
    context.pushDebugGroup = d3d11_pushDebugGroup;
    context.popDebugGroup = d3d11_popDebugGroup;
    context.beginRenderPass = d3d11_beginRenderPass;
    context.endRenderPass = d3d11_endRenderPass;
    return &context;
}

vgpu_driver d3d11_driver = {
    BackendType::D3D11,
    d3d11_is_supported,
    d3d11_create_context
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
