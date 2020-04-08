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

#include "vgpu_backend.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#if defined(_WIN32)
#   define NODRAWTEXT
#   define NOGDI
#   define NOBITMAP
#   define NOMCX
#   define NOSERVICE
#   define NOHELP
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <d3d12.h>
#include <D3D12MemAlloc.h>

#ifdef _DEBUG
#include <dxgidebug.h>

// Declare debug guids to avoid linking with "dxguid.lib"
static const GUID g_d3d12_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
static const GUID g_d3d12_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

#define VHR(hr) if (FAILED(hr)) { VGPU_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

struct TextureD3D12 {
};

static struct {
    bool                    available_initialized;
    bool                    available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    HMODULE                 dxgi_handle;
    HMODULE                 d3d12_handle;

    // DXGI functions
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    // D3D12 functions.
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    bool shutting_down;
    uint32_t factory_flags;
    IDXGIFactory4* dxgi_factory;
    bool tearing_supported;

    bool is_lost;
    ID3D12Device* device;
    /// Memor allocator;
    D3D12MA::Allocator* memory_allocator = nullptr;

    /* Pools */
    Pool<TextureD3D12, VGPU_MAX_TEXTURES>           textures;

} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
#   define vgpuCreateDXGIFactory2 d3d12.CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 d3d12.DXGIGetDebugInterface1

#   define vgpuD3D12CreateDevice d3d12.D3D12CreateDevice
#   define vgpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#   define vgpuD3D12SerializeRootSignature d3d12.D3D12SerializeRootSignature
#   define vgpuD3D12CreateRootSignatureDeserializer d3d12.D3D12CreateRootSignatureDeserializer
#   define vgpuD3D12SerializeVersionedRootSignature d3d12.D3D12SerializeVersionedRootSignature
#   define vgpuD3D12CreateVersionedRootSignatureDeserializer d3d12.D3D12CreateVersionedRootSignatureDeserializer

#else
#   define vgpuCreateDXGIFactory2 CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 DXGIGetDebugInterface1

#   define vgpuD3D12CreateDevice D3D12CreateDevice
#   define vgpuD3D12GetDebugInterface D3D12GetDebugInterface
#   define vgpuD3D12SerializeRootSignature D3D12SerializeRootSignature
#   define vgpuD3D12CreateRootSignatureDeserializer D3D12CreateRootSignatureDeserializer
#   define vgpuD3D12SerializeVersionedRootSignature D3D12SerializeVersionedRootSignature
#   define vgpuD3D12CreateVersionedRootSignatureDeserializer D3D12CreateVersionedRootSignatureDeserializer

#endif

int vgpu_d3d12_supported(void) {
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
    d3d12.D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12.d3d12_handle, "D3D12SerializeRootSignature");
    d3d12.D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12.d3d12_handle, "D3D12CreateRootSignatureDeserializer");
    d3d12.D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(d3d12.d3d12_handle, "D3D12SerializeVersionedRootSignature");
    d3d12.D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12.d3d12_handle, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

    // Create temp factory and detect adapter support
    {
        IDXGIFactory4* temp_dxgi_factory;
        HRESULT hr = vgpuCreateDXGIFactory2(0, IID_PPV_ARGS(&temp_dxgi_factory));
        if (FAILED(hr))
        {
            return false;
        }
        SAFE_RELEASE(temp_dxgi_factory);

        if (SUCCEEDED(vgpuD3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            d3d12.available = true;
        }
    }

    return d3d12.available;
}

static bool d3d12_init(const char* app_name, const vgpu_desc* desc) {
    _VGPU_UNUSED(app_name);

    if (!vgpu_d3d12_supported())
    {
        vgpu_log_error("Direct3D12", "Backend is not supported");
        return false;
    }

    /* Init pools first. */
    d3d12.textures.init();

#if defined(_DEBUG)
    const bool enable_validation =
        (desc->flags & VGPU_CONFIG_FLAGS_VALIDATION) ||
        (desc->flags & VGPU_CONFIG_FLAGS_GPU_BASED_VALIDATION);

    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    if (enable_validation)
    {
        ID3D12Debug* d3d12debug;
        if (SUCCEEDED(vgpuD3D12GetDebugInterface(IID_PPV_ARGS(&d3d12debug))))
        {
            d3d12debug->EnableDebugLayer();

            ID3D12Debug1* d3d12debug1;
            if (SUCCEEDED(d3d12debug->QueryInterface(IID_PPV_ARGS(&d3d12debug1))))
            {
                if (desc->flags & VGPU_CONFIG_FLAGS_GPU_BASED_VALIDATION)
                {
                    // Enable these if you want full validation (will slow down rendering a lot).
                    d3d12debug1->SetEnableGPUBasedValidation(TRUE);
                    d3d12debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                }
                else
                {
                    d3d12debug1->SetEnableGPUBasedValidation(FALSE);
                }

                d3d12debug1->Release();
            }

            d3d12debug->Release();
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        IDXGIInfoQueue* dxgiInfoQueue;
        if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
        {
            // Enable additional debug layers.
            d3d12.factory_flags |= DXGI_CREATE_FACTORY_DEBUG;

            // Declare debug guids to avoid linking with "dxguid.lib"
            static const GUID g_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
            static const GUID g_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };

            dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80, // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(g_DXGI_DEBUG_DXGI, &filter);
            dxgiInfoQueue->Release();
        }
    }
#endif

    if (FAILED(vgpuCreateDXGIFactory2(d3d12.factory_flags, IID_PPV_ARGS(&d3d12.dxgi_factory))))
    {
        vgpu_log_error("Direct3D12", "Failed to create DXGI factory");
        return false;
    }

    /* Check tearing support. */
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* dxgi_factory5 = nullptr;
        HRESULT hr = d3d12.dxgi_factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&dxgi_factory5);
        if (SUCCEEDED(hr))
        {
            hr = dxgi_factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            d3d12.tearing_supported = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            d3d12.tearing_supported = true;
        }

        SAFE_RELEASE(dxgi_factory5);
    }

    IDXGIAdapter1* dxgi_adapter = nullptr;
#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* dxgi_factory6 = nullptr;
    if (SUCCEEDED(d3d12.dxgi_factory->QueryInterface(&dxgi_factory6)))
    {
        UINT index = 0;
        while (dxgi_factory6->EnumAdapterByGpuPreference(index++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter1), (void**)&dxgi_adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 desc = {};
            dxgi_adapter->GetDesc1(&desc);

            // Don't select the Basic Render Driver adapter.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                dxgi_adapter->Release();
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(vgpuD3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }

            dxgi_adapter->Release();
        }

    }
    SAFE_RELEASE(dxgi_factory6);
#endif

    if (dxgi_adapter == nullptr)
    {
        UINT index = 0;
        while (d3d12.dxgi_factory->EnumAdapters1(index++, &dxgi_adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 desc;
            dxgi_adapter->GetDesc1(&desc);

            // Don't select the Basic Render Driver adapter.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                dxgi_adapter->Release();
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(vgpuD3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }

            dxgi_adapter->Release();
        }
    }

#if !defined(NDEBUG)
    if (!dxgi_adapter)
    {
        // Try WARP12 instead
        if (FAILED(d3d12.dxgi_factory->EnumWarpAdapter(__uuidof(IDXGIAdapter), (void**)&dxgi_adapter)))
        {
            vgpu_log_error("Direct3D12", "WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    if (!dxgi_adapter)
    {
        return false;
    }

    VGPU_ASSERT(d3d12.dxgi_factory->IsCurrent());

    if (FAILED(vgpuD3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12.device))))
    {
        //vgpuLogError("Direct3D12: Failed to create device");
        return false;
    }

#if defined(_DEBUG)
    if (enable_validation)
    {
        // Setup break on error + corruption.
        ID3D12InfoQueue* info_queue;
        if (SUCCEEDED(d3d12.device->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)&info_queue)))
        {
#ifdef _DEBUG
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
#endif

            D3D12_MESSAGE_ID hide[] =
            {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                //D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                //D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            info_queue->AddStorageFilterEntries(&filter);
            info_queue->Release();
        }
    }
#endif

    // Create memory allocator
    {
        D3D12MA::ALLOCATOR_DESC desc = {};
        desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
        desc.pDevice = d3d12.device;
        desc.pAdapter = dxgi_adapter;

        VHR(D3D12MA::CreateAllocator(&desc, &d3d12.memory_allocator));

        switch (d3d12.memory_allocator->GetD3D12Options().ResourceHeapTier)
        {
        case D3D12_RESOURCE_HEAP_TIER_1:
            //vgpuLog(VGPU_LOG_TYPE_INFO, "ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
            break;
        case D3D12_RESOURCE_HEAP_TIER_2:
            //vgpuLog(VGPU_LOG_TYPE_INFO, "ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
            break;
        default:
            assert(0);
        }
    }

    //InitCapsAndLimits();


    SAFE_RELEASE(dxgi_adapter);

    return true;
}

static void d3d12_shutdown(void) {
    d3d12.shutting_down = true;

    // Allocator
    D3D12MA::Stats stats;
    d3d12.memory_allocator->CalculateStats(&stats);

    if (stats.Total.UsedBytes > 0) {
        //vgpuLogErrorFormat("Total device memory leaked: %lu bytes.", stats.Total.UsedBytes);
    }

    SAFE_RELEASE(d3d12.memory_allocator);

#if !defined(NDEBUG)
    ULONG refCount = d3d12.device->Release();
    if (refCount > 0)
    {
        //DebugString("Direct3D12: There are %d unreleased references left on the device", refCount);

        ID3D12DebugDevice* debug_device = nullptr;
        if (SUCCEEDED(d3d12.device->QueryInterface(&debug_device)))
        {
            debug_device->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
            debug_device->Release();
        }
    }
#else
    d3d12.device->Release();
#endif

    SAFE_RELEASE(d3d12.dxgi_factory);

#ifdef _DEBUG
    IDXGIDebug* dxgi_debug;
    if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, __uuidof(IDXGIDebug), (void**)&dxgi_debug)))
    {
        dxgi_debug->ReportLiveObjects(g_d3d12_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
    SAFE_RELEASE(dxgi_debug);
#endif
}

static void d3d12_wait_idle(void) {
}

static void d3d12_begin_frame(void) {
}

static void d3d12_end_frame(void) {
}

static vgpu_backend d3d12_get_backend(void) {
    return VGPU_BACKEND_DIRECT3D12;
}

vgpu_renderer* vgpu_create_d3d12_backend(void) {
    static vgpu_renderer render_api = { nullptr };
    ASSIGN_DRIVER(d3d12);
    return &render_api;
}
