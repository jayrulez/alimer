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

#include "D3D12GraphicsDevice.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"
#include "D3D12GraphicsContext.h"
#include "D3D12MemAlloc.h"
#include "core/String.h"

namespace alimer
{
    uint32_t D3D12GraphicsDevice::deviceCount = 0;

    bool D3D12GraphicsDevice::IsAvailable()
    {
        static bool availableCheck = false;
        static bool available = false;
        if (availableCheck) {
            return available;
        }

        availableCheck = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        static HMODULE dxgiDLL = LoadLibraryW(L"dxgi.dll");
        if (dxgiDLL == nullptr)
            return false;

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiDLL, "CreateDXGIFactory2");
        if (CreateDXGIFactory2 == nullptr)
            return false;

        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiDLL, "DXGIGetDebugInterface1");

        static HMODULE d3d12DLL = LoadLibraryW(L"d3d12.dll");
        if (d3d12DLL == nullptr)
            return false;

        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12DLL, "D3D12CreateDevice");
        if (D3D12CreateDevice == nullptr)
            return false;

        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12DLL, "D3D12GetDebugInterface");
        D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12DLL, "D3D12SerializeRootSignature");
        D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12DLL, "D3D12CreateRootSignatureDeserializer");
        D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(d3d12DLL, "D3D12SerializeVersionedRootSignature");
        D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12DLL, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

        available = true;
        return available;
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        WaitForIdle();

        //ReleaseTrackedResources();
        //ExecuteDeferredReleases();

        Shutdown();
    }

    void D3D12GraphicsDevice::GetAdapter(D3D_FEATURE_LEVEL d3dMinFeatureLevel, IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        IDXGIAdapter1* adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6;
        HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                    IID_PPV_ARGS(&adapter)));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter, d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                    OutputDebugStringW(buff);
#endif
                    break;
                }
            }

            dxgiFactory6->Release();
        }
#endif
        if (!adapter)
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, &adapter));
                ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter, d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                    OutputDebugStringW(buff);
#endif
                    break;
                }
            }
        }

#if !defined(NDEBUG)
        if (!adapter)
        {
            // Try WARP12 instead
            if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
            {
                LOG_ERROR("WARP12 not available. Enable the 'Graphics Tools' optional feature");
                ALIMER_FORCE_CRASH();
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            LOG_ERROR("No Direct3D 12 device found");
            ALIMER_FORCE_CRASH();
        }

        *ppAdapter = adapter;
    }

    bool D3D12GraphicsDevice::Init(const Desc& desc)
    {
        if (!IsAvailable()) {
            return false;
        }

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (desc.enableDebugLayer)
        {
            ID3D12Debug* debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                ID3D12Debug1* debugController1;
                if (SUCCEEDED(debugController->QueryInterface(&debugController1)))
                {
                    debugController1->SetEnableGPUBasedValidation(true);
                    //debugController1->SetEnableSynchronizedCommandQueueValidation(true);
                    debugController1->Release();
                }

                debugController->Release();
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }
#endif

        VHR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

        // Check tearing support.
        {
            BOOL allowTearing = FALSE;

            IDXGIFactory5* dxgiFactory5 = nullptr;
            HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory5);
            if (SUCCEEDED(hr))
            {
                hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
                isTearingSupported = false;
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                isTearingSupported = true;
            }
            SafeRelease(dxgiFactory5);
        }

        IDXGIAdapter1* adapter;
        GetAdapter(minFeatureLevel, &adapter);

        // Create the DX12 API device object.
        VHR(D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&d3dDevice)));

#ifndef NDEBUG
        // Configure debug device (if active).
        ID3D12InfoQueue* d3dInfoQueue;
        if (SUCCEEDED(d3dDevice->QueryInterface(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D12_MESSAGE_ID hide[] =
            {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
                //D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            };
            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
            d3dInfoQueue->Release();
        }
#endif

        // Create memory allocator
        {
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice;
            desc.pAdapter = adapter;

            VHR(D3D12MA::CreateAllocator(&desc, &memoryAllocator));
            switch (memoryAllocator->GetD3D12Options().ResourceHeapTier)
            {
            case D3D12_RESOURCE_HEAP_TIER_1:
                LOG_DEBUG("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
                break;
            case D3D12_RESOURCE_HEAP_TIER_2:
                LOG_DEBUG("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
                break;
            default:
                break;
            }
        }

        // Init caps and features.
        InitCapabilities(adapter);
        adapter->Release();

        // Create command queues
        graphicsCommandQueue = new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
        computeCommandQueue = new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
        copyCommandQueue = new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COPY);

        // Render target descriptor heap (RTV).
        {
            RTVHeap.Capacity = 1024;

            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = RTVHeap.Capacity;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            VHR(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&RTVHeap.Heap)));
            RTVHeap.CPUStart = RTVHeap.Heap->GetCPUDescriptorHandleForHeapStart();
        }
        // Depth-stencil descriptor heap (DSV).
        {
            DSVHeap.Capacity = 256;

            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = DSVHeap.Capacity;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            VHR(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DSVHeap.Heap)));
            DSVHeap.CPUStart = DSVHeap.Heap->GetCPUDescriptorHandleForHeapStart();
        }

        deviceCount++;
        return true;
    }

    void D3D12GraphicsDevice::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        // Init capabilities
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(dxgiAdapter->GetDesc1(&desc));

            caps.backendType = BackendType::Direct3D12;
            caps.vendorId = desc.VendorId;
            caps.deviceId = desc.DeviceId;

            std::wstring deviceName(desc.Description);
            caps.adapterName = alimer::ToUtf8(deviceName);

            // Detect adapter type.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                caps.adapterType = GPUAdapterType::CPU;
            }
            else {
                D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
                VHR(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

                caps.adapterType = arch.UMA ? GPUAdapterType::IntegratedGPU : GPUAdapterType::DiscreteGPU;
            }

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

            HRESULT hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
            if (SUCCEEDED(hr))
            {
                featureLevel = featLevels.MaxSupportedFeatureLevel;
            }
            else
            {
                featureLevel = D3D_FEATURE_LEVEL_11_0;
            }

            D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

            // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

            if (FAILED(d3dDevice->CheckFeatureSupport(
                D3D12_FEATURE_ROOT_SIGNATURE,
                &featureData, sizeof(featureData))))
            {
                rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }

            // Features
            caps.features.independentBlend = true;
            caps.features.computeShader = true;
            caps.features.geometryShader = true;
            caps.features.tessellationShader = true;
            caps.features.logicOp = true;
            caps.features.multiViewport = true;
            caps.features.fullDrawIndexUint32 = true;
            caps.features.multiDrawIndirect = true;
            caps.features.fillModeNonSolid = true;
            caps.features.samplerAnisotropy = true;
            caps.features.textureCompressionETC2 = false;
            caps.features.textureCompressionASTC_LDR = false;
            caps.features.textureCompressionBC = true;
            caps.features.textureCubeArray = true;

            D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12options5 = {};
            if (SUCCEEDED(d3dDevice->CheckFeatureSupport(
                D3D12_FEATURE_D3D12_OPTIONS5,
                &d3d12options5, sizeof(d3d12options5)))
                && d3d12options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
            {
                caps.features.raytracing = true;
            }
            else
            {
                caps.features.raytracing = false;
            }

            supportsRenderPass = false;
            if (d3d12options5.RenderPassesTier > D3D12_RENDER_PASS_TIER_0 &&
                static_cast<GPUVendorId>(caps.vendorId) != GPUVendorId::Intel)
            {
                supportsRenderPass = true;
            }

            // Limits
            caps.limits.maxVertexAttributes = kMaxVertexAttributes;
            caps.limits.maxVertexBindings = kMaxVertexAttributes;
            caps.limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
            caps.limits.maxVertexBindingStride = kMaxVertexBufferStride;

            //caps.limits.maxTextureDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
            caps.limits.maxTextureDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            caps.limits.maxTextureDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            caps.limits.maxTextureDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
            caps.limits.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            caps.limits.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
            caps.limits.maxUniformBufferSize = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
            caps.limits.minUniformBufferOffsetAlignment = 256u;
            caps.limits.maxStorageBufferSize = UINT32_MAX;
            caps.limits.minStorageBufferOffsetAlignment = 16;
            caps.limits.maxSamplerAnisotropy = D3D12_MAX_MAXANISOTROPY;
            caps.limits.maxViewports = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            caps.limits.maxViewportWidth = D3D12_VIEWPORT_BOUNDS_MAX;
            caps.limits.maxViewportHeight = D3D12_VIEWPORT_BOUNDS_MAX;
            caps.limits.maxTessellationPatchSize = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
            caps.limits.pointSizeRangeMin = 1.0f;
            caps.limits.pointSizeRangeMax = 1.0f;
            caps.limits.lineWidthRangeMin = 1.0f;
            caps.limits.lineWidthRangeMax = 1.0f;
            caps.limits.maxComputeSharedMemorySize = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
            caps.limits.maxComputeWorkGroupCountX = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            caps.limits.maxComputeWorkGroupCountY = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            caps.limits.maxComputeWorkGroupCountZ = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            caps.limits.maxComputeWorkGroupInvocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
            caps.limits.maxComputeWorkGroupSizeX = D3D12_CS_THREAD_GROUP_MAX_X;
            caps.limits.maxComputeWorkGroupSizeY = D3D12_CS_THREAD_GROUP_MAX_Y;
            caps.limits.maxComputeWorkGroupSizeZ = D3D12_CS_THREAD_GROUP_MAX_Z;

            /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
            /*UINT dxgi_fmt_caps = 0;
            for (int fmt = (VGPUTextureFormat_Undefined + 1); fmt < VGPUTextureFormat_Count; fmt++)
            {
                DXGI_FORMAT dxgi_fmt = _vgpu_d3d_get_format((VGPUTextureFormat)fmt);
                HRESULT hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, dxgi_fmt, &dxgi_fmt_caps);
                VGPU_ASSERT(SUCCEEDED(hr));
                sg_pixelformat_info* info = &_sg.formats[fmt];
                info->sample = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D);
                info->filter = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
                info->render = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
                info->blend = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE);
                info->msaa = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);
                info->depth = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
                if (info->depth) {
                    info->render = true;
                }
            }*/
        }
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        SAFE_RELEASE(RTVHeap.Heap);
        SAFE_RELEASE(DSVHeap.Heap);

        SafeDelete(copyCommandQueue);
        SafeDelete(computeCommandQueue);
        SafeDelete(graphicsCommandQueue);

        // Allocator
        D3D12MA::Stats stats;
        memoryAllocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            LOG_ERROR("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        memoryAllocator->Release();
        memoryAllocator = nullptr;

        ULONG refCount = d3dDevice->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            LOG_DEBUG("Direct3D12: There are %d unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice;
            if (SUCCEEDED(d3dDevice->QueryInterface(IID_PPV_ARGS(&debugDevice))))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        deviceCount--;
#ifdef _DEBUG
        if (deviceCount == 0)
        {
            IDXGIDebug1* dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug->Release();
            }
        }
#endif
    }

    D3D12CommandQueue* D3D12GraphicsDevice::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return computeCommandQueue;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            return copyCommandQueue;
        default:
            return graphicsCommandQueue;
        }
    }

    ID3D12CommandQueue* D3D12GraphicsDevice::GetD3DCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return computeCommandQueue->GetHandle();
        case D3D12_COMMAND_LIST_TYPE_COPY:
            return copyCommandQueue->GetHandle();
        default:
            return graphicsCommandQueue->GetHandle();
        }
    }

    void D3D12GraphicsDevice::WaitForIdle()
    {
        graphicsCommandQueue->WaitForIdle();
        computeCommandQueue->WaitForIdle();
        copyCommandQueue->WaitForIdle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12GraphicsDevice::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
    {
        DescriptorHeap* heap = nullptr;
        uint32_t descriptorSize = 0;
        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        {
            descriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            heap = &RTVHeap;
        }
        else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
        {
            descriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            heap = &DSVHeap;
        }

        ALIMER_ASSERT((heap->Size + count) < heap->Capacity);

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        CPUHandle.ptr = heap->CPUStart.ptr + (size_t)heap->Size * descriptorSize;
        heap->Size += count;
        return CPUHandle;
    }

    GraphicsContext* D3D12GraphicsDevice::CreateContext(const GraphicsContextDescription& desc)
    {
        return new D3D12GraphicsContext(this, desc);
    }

    Texture* D3D12GraphicsDevice::CreateTexture(const TextureDescription& desc, const void* initialData)
    {
        return new D3D12Texture(this, desc, initialData);
    }

    /*D3D12CommandContext* D3D12GraphicsDevice::AllocateContext(D3D12_COMMAND_LIST_TYPE type, const std::string& id)
    {
        std::lock_guard<std::mutex> LockGuard(cmdBufferAllocationMutex);

        auto& availableContextsQueue = availableContexts[type];

        D3D12CommandContext* result = nullptr;
        if (availableContextsQueue.empty())
        {
            result = new D3D12CommandContext(this, type, id);
            commandBufferPool[type].emplace_back(result);
        }
        else
        {
            result = availableContextsQueue.front();
            availableContextsQueue.pop();
            result->Reset();
        }

        ALIMER_ASSERT(result != nullptr);
        return result;
    }

    void D3D12GraphicsDevice::FreeContext(D3D12_COMMAND_LIST_TYPE type, D3D12CommandContext* context)
    {
        ALIMER_ASSERT(context != nullptr);
        std::lock_guard<std::mutex> LockGuard(cmdBufferAllocationMutex);
        availableContexts[type].push(context);
    }*/

    void D3D12GraphicsDevice::WaitForFenceValue(uint64_t fenceValue)
    {
        D3D12CommandQueue* producer = GetCommandQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
        producer->WaitForFenceValue(fenceValue);
    }


    void D3D12GraphicsDevice::HandleDeviceLost()
    {

    }
}
