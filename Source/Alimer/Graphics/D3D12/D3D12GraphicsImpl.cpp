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

#include "D3D12GraphicsImpl.h"
//#include "D3D12SwapChain.h"
#include "D3D12Texture.h"
//#include "D3D12Buffer.h"
#include "D3D12CommandBuffer.h"
#include "Core/Window.h"
#include "Math/Matrix4x4.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <d3dcompiler.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif

namespace alimer
{
    namespace
    {
        struct VERTEX_CONSTANT_BUFFER
        {
            float   mvp[4][4];
        };

        // Buffers used for secondary viewports created by the multi-viewports systems
        struct ImGui_ImplDX12_FrameContext
        {
            ID3D12CommandAllocator* CommandAllocator;
            ID3D12Resource* RenderTarget;
            D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetCpuDescriptors;
        };

        // Buffers used during the rendering of a frame
        struct ImGui_ImplDX12_RenderBuffers
        {
            ID3D12Resource* IndexBuffer;
            ID3D12Resource* VertexBuffer;
            int IndexBufferSize;
            int VertexBufferSize;
        };

        struct ImGuiViewportDataDx12
        {
            // Window
            ID3D12CommandQueue* CommandQueue;
            ID3D12GraphicsCommandList* CommandList;
            ID3D12DescriptorHeap* RtvDescHeap;
            IDXGISwapChain3* SwapChain;
            ID3D12Fence* Fence;
            UINT64                          FenceSignaledValue;
            HANDLE                          FenceEvent;
            ImGui_ImplDX12_FrameContext* FrameCtx;

            // Render buffers
            UINT  FrameIndex;
            ImGui_ImplDX12_RenderBuffers* FrameRenderBuffers;

            ImGuiViewportDataDx12()
            {
                CommandQueue = NULL;
                CommandList = NULL;
                RtvDescHeap = NULL;
                SwapChain = NULL;
                Fence = NULL;
                FenceSignaledValue = 0;
                FenceEvent = NULL;
                FrameCtx = new ImGui_ImplDX12_FrameContext[kInflightFrameCount];
                FrameIndex = UINT_MAX;
                FrameRenderBuffers = new ImGui_ImplDX12_RenderBuffers[kInflightFrameCount];

                for (uint32_t i = 0; i < kInflightFrameCount; ++i)
                {
                    FrameCtx[i].CommandAllocator = NULL;
                    FrameCtx[i].RenderTarget = NULL;

                    // Create buffers with a default size (they will later be grown as needed)
                    FrameRenderBuffers[i].IndexBuffer = NULL;
                    FrameRenderBuffers[i].VertexBuffer = NULL;
                    FrameRenderBuffers[i].VertexBufferSize = 5000;
                    FrameRenderBuffers[i].IndexBufferSize = 10000;
                }
            }
            ~ImGuiViewportDataDx12()
            {
                IM_ASSERT(CommandQueue == NULL && CommandList == NULL);
                IM_ASSERT(RtvDescHeap == NULL);
                IM_ASSERT(SwapChain == NULL);
                IM_ASSERT(Fence == NULL);
                IM_ASSERT(FenceEvent == NULL);

                for (UINT i = 0; i < kInflightFrameCount; ++i)
                {
                    IM_ASSERT(FrameCtx[i].CommandAllocator == NULL && FrameCtx[i].RenderTarget == NULL);
                    IM_ASSERT(FrameRenderBuffers[i].IndexBuffer == NULL && FrameRenderBuffers[i].VertexBuffer == NULL);
                }

                delete[] FrameCtx; FrameCtx = NULL;
                delete[] FrameRenderBuffers; FrameRenderBuffers = NULL;
            }
        };
    }

    bool D3D12GraphicsImpl::IsAvailable()
    {
        static bool available_initialized = false;
        static bool available = false;
        if (available_initialized) {
            return available;
        }

        available_initialized = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        static HMODULE dxgiLib = LoadLibraryA("dxgi.dll");
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        if (CreateDXGIFactory2 == nullptr) {
            return false;
        }
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");

        static HMODULE d3d12Lib = LoadLibraryA("d3d12.dll");
        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12Lib, "D3D12CreateDevice");
        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12Lib, "D3D12GetDebugInterface");
        if (D3D12CreateDevice == nullptr) {
            return false;
        }

        D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12Lib, "D3D12SerializeRootSignature");
        D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12Lib, "D3D12CreateRootSignatureDeserializer");
        D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(d3d12Lib, "D3D12SerializeVersionedRootSignature");
        D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12Lib, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

        available = true;
        return true;
    }

    D3D12GraphicsImpl::D3D12GraphicsImpl(Window* window, GPUFlags flags)
        : frameIndex(0)
    {
        ALIMER_VERIFY(IsAvailable());

        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(flags & GPUFlags::DebugRuntime | GPUFlags::GPUBaseValidation))
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
            {
                debugController->EnableDebugLayer();

                if (any(flags & GPUFlags::GPUBaseValidation))
                {
                    ComPtr<ID3D12Debug1> d3d12Debug1;
                    if (SUCCEEDED(debugController.As(&d3d12Debug1)))
                    {
                        d3d12Debug1->SetEnableGPUBasedValidation(true);
                        //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                    }
                }
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

#if defined(_DEBUG)
            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(g_DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
#endif
        }

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            ComPtr<IDXGIFactory5> dxgiFactory5 = nullptr;
            HRESULT hr = dxgiFactory.As(&dxgiFactory5);
            if (SUCCEEDED(hr))
            {
                hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                dxgiFactoryCaps |= DXGIFactoryCaps::Tearing;
            }
        }

        ComPtr<IDXGIAdapter1> adapter;
        GetAdapter(&adapter);

        // Create the DX12 API device object.
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), minFeatureLevel, IID_PPV_ARGS(&d3dDevice)));
        d3dDevice->SetName(L"Alimer Device");

        // Configure debug device (if active).
#ifndef NDEBUG
        {
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
                    D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
                    D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE
                };
                D3D12_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                d3dInfoQueue->AddStorageFilterEntries(&filter);
                d3dInfoQueue->Release();
            }
        }
#endif

        // Create memory allocator
        {
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice;
            desc.pAdapter = adapter.Get();

            ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &allocator));
            switch (allocator->GetD3D12Options().ResourceHeapTier)
            {
            case D3D12_RESOURCE_HEAP_TIER_1:
                LOGD("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
                break;
            case D3D12_RESOURCE_HEAP_TIER_2:
                LOGD("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
                break;
            default:
                break;
            }
        }

        InitCapabilities(adapter.Get());

        // Create command queue's
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 0;
            ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(graphicsQueue.ReleaseAndGetAddressOf())));
            graphicsQueue->SetName(L"Graphics Command Queue");

            //computeQueue = CreateCommandQueue(CommandQueueType::Compute, "");
            //copyQueue = CreateCommandQueue(CommandQueueType::Copy, "");
        }

        // Init descriptor heaps.
        {
            RtvHeap = D3D12CreateDescriptorHeap(d3dDevice, 1024, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
            DsvHeap = D3D12CreateDescriptorHeap(d3dDevice, 1024, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
            CbvSrvUavCpuHeap = D3D12CreateDescriptorHeap(d3dDevice, 16 * 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

            for (uint32_t i = 0; i < kInflightFrameCount; ++i)
            {
                CbvSrvUavGpuHeaps[i] = D3D12CreateDescriptorHeap(d3dDevice, 16 * 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            }
        }

        // Create main swapchain.
        {
            IDXGISwapChain1* tempSwapChain = DXGICreateSwapchain(
                dxgiFactory.Get(),
                dxgiFactoryCaps,
                graphicsQueue.Get(),
                window->GetNativeHandle(),
                0, 0,
                PixelFormat::BGRA8Unorm,
                kInflightFrameCount
            );

            ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));
            SafeRelease(tempSwapChain);

            for (uint32_t Idx = 0; Idx < kInflightFrameCount; ++Idx)
            {
                {
                    ID3D12Resource* backbufferResource;
                    ThrowIfFailed(swapChain->GetBuffer(Idx, IID_PPV_ARGS(&backbufferResource)));
                    backbufferTextures[Idx] = new D3D12Texture(this, backbufferResource, D3D12_RESOURCE_STATE_PRESENT);
                }
            }

            backbufferIndex = swapChain->GetCurrentBackBufferIndex();
        }

        // Create a fence for tracking GPU execution progress.
        {
            ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence)));
            frameFence->SetName(L"Frame Fence");

            frameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
            if (!frameFenceEvent)
            {
                LOGE("Direct3D12: CreateEventEx failed.");
            }
        }

        // Init imgui backend.
        InitImGui();
    }

    D3D12GraphicsImpl::~D3D12GraphicsImpl()
    {
        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            LOGE("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);

        // Release Swapchain resources
        {
            SafeRelease(swapChain);
        }

        {
            CloseHandle(frameFenceEvent);
            SafeRelease(frameFence);
        }

        ReleaseTrackedResources();

        // Command queues
        {
            commandBufferPool.clear();
            graphicsQueue.Reset();
        }

        ShutdownImgui(true);

        ULONG refCount = d3dDevice->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            LOGD("Direct3D12: There are {} unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice;
            if (SUCCEEDED(d3dDevice->QueryInterface(&debugDevice)))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        // Release factory at last.
        dxgiFactory.Reset();

#ifdef _DEBUG
        {
            IDXGIDebug1* dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug->Release();
            }
        }
#endif
    }

    void D3D12GraphicsImpl::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

        caps.backendType = GPUBackendType::Direct3D12;
        caps.vendorId = desc.VendorId;
        caps.deviceId = desc.DeviceId;

        std::wstring deviceName(desc.Description);
        caps.adapterName = ToUtf8(deviceName);

        // Detect adapter type.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            caps.adapterType = GPUAdapterType::CPU;
        }
        else {
            D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
            ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

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
            static_cast<GPUKnownVendorId>(caps.vendorId) != GPUKnownVendorId::Intel)
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
        if (caps.limits.maxViewports > kMaxViewportAndScissorRects) {
            caps.limits.maxViewports = kMaxViewportAndScissorRects;
        }

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
        UINT dxgi_fmt_caps = 0;
        for (uint32_t format = (static_cast<uint32_t>(PixelFormat::Invalid) + 1); format < static_cast<uint32_t>(PixelFormat::Count); format++)
        {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT support;
            support.Format = ToDXGIFormat((PixelFormat)format);

            if (support.Format == DXGI_FORMAT_UNKNOWN)
                continue;

            ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)));
        }
    }

    void D3D12GraphicsImpl::GetAdapter(IDXGIAdapter1** ppAdapter, bool lowPower)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> dxgiFactory6;
        HRESULT hr = dxgiFactory.As(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (lowPower) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    gpuPreference,
                    IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
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
#endif

        if (!adapter)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf())); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
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
            if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
            {
                LOGE("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            LOGE("No Direct3D 12 device found");
        }

        *ppAdapter = adapter.Detach();
    }


    void D3D12GraphicsImpl::WaitForGPU()
    {
        graphicsQueue->Signal(frameFence, ++frameCount);
        frameFence->SetEventOnCompletion(frameCount, frameFenceEvent);
        WaitForSingleObject(frameFenceEvent, INFINITE);

        CbvSrvUavGpuHeaps[frameIndex].Size = 0;
        //GPUUploadMemoryHeaps[Gfx->FrameIndex].Size = 0;
    }

    bool D3D12GraphicsImpl::BeginFrame()
    {
        if (!imguiPipelineState)
            CreateImguiObjects();

        return true;
    }

    void D3D12GraphicsImpl::EndFrame()
    {
        // TODO: Sync copy and compute here
        CommandBuffer& commandBuffer = GPU->BeginCommandBuffer("imGui");
        auto d3dCommandBuffer = static_cast<D3D12CommandBuffer*>(&commandBuffer);
        d3dCommandBuffer->GetCommandList()->SetDescriptorHeaps(1, &CbvSrvUavGpuHeaps[frameIndex].handle);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = backbufferTextures[backbufferIndex]->GetResource();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        d3dCommandBuffer->GetCommandList()->ResourceBarrier(1, &barrier);

        auto RTV = backbufferTextures[backbufferIndex]->GetRTV();
        auto clearColor = Colors::CornflowerBlue;
        d3dCommandBuffer->GetCommandList()->ClearRenderTargetView(RTV, &clearColor.r, 0, nullptr);
        d3dCommandBuffer->GetCommandList()->OMSetRenderTargets(1, &RTV, FALSE, NULL);

        RenderDrawData(ImGui::GetDrawData(), d3dCommandBuffer->GetCommandList());
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        d3dCommandBuffer->GetCommandList()->ResourceBarrier(1, &barrier);
        commandBuffer.Commit();

        // Execute prending command lists.
        if (!pendingCommandLists.empty())
        {
            commandBufferAllocationMutex.lock();
            graphicsQueue->ExecuteCommandLists(static_cast<UINT>(pendingCommandLists.size()), pendingCommandLists.data());
            pendingCommandLists.clear();
            commandBufferAllocationMutex.unlock();
        }

        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
        }

        uint32_t syncInterval = 1;
        uint32_t presentFlags = 0;
        /*if (!vSync) {
            syncInterval = 0;
            if (any(dxgiFactoryCaps & DXGIFactoryCaps::Tearing)) {
                presentFlags = DXGI_PRESENT_ALLOW_TEARING;
            }
        }*/

        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        HRESULT hr = swapChain->Present(syncInterval, presentFlags);

        // If the device was reset we must completely reinitialize the renderer.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            HandleDeviceLost();
        }
        else
        {
            ThrowIfFailed(hr);

            graphicsQueue->Signal(frameFence, ++frameCount);
            uint64_t GPUFrameCount = frameFence->GetCompletedValue();

            if ((frameCount - GPUFrameCount) >= kInflightFrameCount)
            {
                frameFence->SetEventOnCompletion(GPUFrameCount + 1, frameFenceEvent);
                WaitForSingleObject(frameFenceEvent, INFINITE);
            }

            frameIndex = (frameIndex + 1) % kInflightFrameCount;
            backbufferIndex = swapChain->GetCurrentBackBufferIndex();
            CbvSrvUavGpuHeaps[frameIndex].Size = 0;

            if (!dxgiFactory->IsCurrent())
            {
                // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
                ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
            }
        }
    }


    void D3D12GraphicsImpl::HandleDeviceLost()
    {

    }

    Texture* D3D12GraphicsImpl::GetBackbufferTexture() const
    {
        return backbufferTextures[backbufferIndex].Get();
    }

    CommandBuffer& D3D12GraphicsImpl::BeginCommandBuffer(const std::string_view id)
    {
        std::lock_guard<std::mutex> LockGuard(commandBufferAllocationMutex);

        D3D12CommandBuffer* commandBuffer = nullptr;
        if (commandBufferQueue.empty())
        {
            commandBuffer = new D3D12CommandBuffer(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
            commandBufferPool.emplace_back(commandBuffer);
        }
        else
        {
            commandBuffer = commandBufferQueue.front();
            commandBufferQueue.pop();
            commandBuffer->Reset(frameIndex);
        }

        ALIMER_ASSERT(commandBuffer != nullptr);
        return *commandBuffer;
    }

    void D3D12GraphicsImpl::CommitCommandBuffer(D3D12CommandBuffer* commandBuffer, bool waitForCompletion)
    {
        std::lock_guard<std::mutex> LockGuard(commandBufferAllocationMutex);

        auto commandList = commandBuffer->GetCommandList();
        ThrowIfFailed(commandList->Close());

        if (waitForCompletion)
        {
            ID3D12CommandList* commandLists[] = { commandList };
            graphicsQueue->ExecuteCommandLists(1, commandLists);

            ComPtr<ID3D12Fence> fence;
            ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
            HANDLE gpuCompletedEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
            if (!gpuCompletedEvent)
                throw std::exception("CreateEventEx");

            ThrowIfFailed(graphicsQueue->Signal(fence.Get(), 1ULL));
            ThrowIfFailed(fence->SetEventOnCompletion(1ULL, gpuCompletedEvent));
            WaitForSingleObject(gpuCompletedEvent, INFINITE);
        }
        else {
            pendingCommandLists.push_back(commandList);
        }

        // Recycle command buffer.
        commandBufferQueue.push(commandBuffer);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12GraphicsImpl::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
    {
        DescriptorHeap* descriptorHeap;

        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        {
            descriptorHeap = &RtvHeap;
        }
        else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
        {
            descriptorHeap = &DsvHeap;
        }
        else
        {
            descriptorHeap = &CbvSrvUavCpuHeap;
        }

        ALIMER_ASSERT(descriptorHeap != nullptr && (descriptorHeap->Size + count) < descriptorHeap->Capacity);

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        CPUHandle.ptr = descriptorHeap->CpuStart.ptr + (size_t)descriptorHeap->Size * descriptorHeap->DescriptorSize;
        descriptorHeap->Size += count;

        return CPUHandle;
    }

    void D3D12GraphicsImpl::AllocateGPUDescriptors(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE* OutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGPUHandle)
    {
        ALIMER_ASSERT(OutCPUHandle && OutGPUHandle && count > 0);

        DescriptorHeap* descriptorHeap = &CbvSrvUavGpuHeaps[frameIndex];

        assert((descriptorHeap->Size + count) < descriptorHeap->Capacity);

        OutCPUHandle->ptr = descriptorHeap->CpuStart.ptr + (u64)descriptorHeap->Size * descriptorHeap->DescriptorSize;
        OutGPUHandle->ptr = descriptorHeap->GpuStart.ptr + (u64)descriptorHeap->Size * descriptorHeap->DescriptorSize;

        descriptorHeap->Size += count;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12GraphicsImpl::CopyDescriptorsToGPUHeap(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE srcBaseHandle)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CPUBaseHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUBaseHandle;
        AllocateGPUDescriptors(count, &CPUBaseHandle, &GPUBaseHandle);
        d3dDevice->CopyDescriptorsSimple(count, CPUBaseHandle, srcBaseHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return GPUBaseHandle;
    }

    /* ImGui integration */
    void D3D12GraphicsImpl::InitImGui()
    {
        // Setup back-end capabilities flags
        ImGuiIO& io = ImGui::GetIO();
        io.BackendRendererName = "imgui_impl_dx12";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional) // FIXME-VIEWPORT: Actually unfinished..

        // Create a dummy ImGuiViewportDataDx12 holder for the main viewport,
        // Since this is created and managed by the application, we will only use the ->Resources[] fields.
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->RendererUserData = IM_NEW(ImGuiViewportDataDx12)();

        // Setup back-end capabilities flags
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;    // We can create multi-viewports on the Renderer side (optional)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            //ImGui_ImplDX12_InitPlatformInterface();
        }
    }

    void D3D12GraphicsImpl::ShutdownImgui(bool all)
    {
        if (all)
        {
            // Manually delete main viewport render resources in-case we haven't initialized for viewports
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            if (ImGuiViewportDataDx12* data = (ImGuiViewportDataDx12*)main_viewport->RendererUserData)
            {
                // We could just call ImGui_ImplDX12_DestroyWindow(main_viewport) as a convenience but that would be misleading since we only use data->Resources[]
                /*for (UINT i = 0; i < g_numFramesInFlight; i++)
                    ImGui_ImplDX12_DestroyRenderBuffers(&data->FrameRenderBuffers[i]);
                IM_DELETE(data);*/

                main_viewport->RendererUserData = NULL;
            }
        }

        SafeRelease(imguiRootSignature);
        SafeRelease(imguiPipelineState);
        SafeRelease(imguiFontTextureResource);

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->TexID = nullptr; // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.
    }

    bool D3D12GraphicsImpl::CreateImguiObjects()
    {
        if (imguiPipelineState)
            ShutdownImgui(false);

        // Create the root signature
        {
            D3D12_DESCRIPTOR_RANGE descRange = {};
            descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            descRange.NumDescriptors = 1;
            descRange.BaseShaderRegister = 0;
            descRange.RegisterSpace = 0;
            descRange.OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER param[2] = {};

            param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            param[0].Constants.ShaderRegister = 0;
            param[0].Constants.RegisterSpace = 0;
            param[0].Constants.Num32BitValues = 16;
            param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

            param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param[1].DescriptorTable.NumDescriptorRanges = 1;
            param[1].DescriptorTable.pDescriptorRanges = &descRange;
            param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_STATIC_SAMPLER_DESC staticSampler = {};
            staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.MipLODBias = 0.f;
            staticSampler.MaxAnisotropy = 0;
            staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSampler.MinLOD = 0.f;
            staticSampler.MaxLOD = 0.f;
            staticSampler.ShaderRegister = 0;
            staticSampler.RegisterSpace = 0;
            staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_ROOT_SIGNATURE_DESC desc = {};
            desc.NumParameters = _countof(param);
            desc.pParameters = param;
            desc.NumStaticSamplers = 1;
            desc.pStaticSamplers = &staticSampler;
            desc.Flags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

            ID3DBlob* blob = NULL;
            if (D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, NULL) != S_OK)
                return false;

            ThrowIfFailed(d3dDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&imguiRootSignature)));
            blob->Release();
        }

        // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
        // If you would like to use this DX12 sample code but remove this dependency you can:
        //  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
        //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
        // See https://github.com/ocornut/imgui/pull/638 for sources and details.

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
        memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        psoDesc.NodeMask = 1;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.pRootSignature = imguiRootSignature;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = backbufferFormat;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        ID3DBlob* vertexShaderBlob;
        ID3DBlob* pixelShaderBlob;

        // Create the vertex shader
        {
            static const char* vertexShader =
                "cbuffer vertexBuffer : register(b0) \
            {\
              float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
              float2 pos : POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
              PS_INPUT output;\
              output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
              output.col = input.col;\
              output.uv  = input.uv;\
              return output;\
            }";

            if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &vertexShaderBlob, NULL)))
                return false; // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };

            // Create the input layout
            static D3D12_INPUT_ELEMENT_DESC local_layout[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };
            psoDesc.InputLayout = { local_layout, 3 };
        }

        // Create the pixel shader
        {
            static const char* pixelShader =
                "struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            SamplerState sampler0 : register(s0);\
            Texture2D texture0 : register(t0);\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
              float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
              return out_col; \
            }";

            if (FAILED(D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &pixelShaderBlob, NULL)))
            {
                vertexShaderBlob->Release();
                return false; // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            }
            psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        }

        // Create the blending setup
        {
            D3D12_BLEND_DESC& desc = psoDesc.BlendState;
            desc.AlphaToCoverageEnable = false;
            desc.RenderTarget[0].BlendEnable = true;
            desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }

        // Create the rasterizer state
        {
            D3D12_RASTERIZER_DESC& desc = psoDesc.RasterizerState;
            desc.FillMode = D3D12_FILL_MODE_SOLID;
            desc.CullMode = D3D12_CULL_MODE_NONE;
            desc.FrontCounterClockwise = FALSE;
            desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            desc.DepthClipEnable = true;
            desc.MultisampleEnable = FALSE;
            desc.AntialiasedLineEnable = FALSE;
            desc.ForcedSampleCount = 0;
            desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        }

        // Create depth-stencil State
        {
            D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
            desc.DepthEnable = false;
            desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.StencilEnable = false;
            desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.BackFace = desc.FrontFace;
        }

        HRESULT result_pipeline_state = d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&imguiPipelineState));
        vertexShaderBlob->Release();
        pixelShaderBlob->Release();
        if (result_pipeline_state != S_OK)
            return false;

        CreateImguiFontsTexture();

        return true;
    }

    void D3D12GraphicsImpl::CreateImguiFontsTexture()
    {
        // Build texture atlas
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        // Upload texture to graphics system
        {
            D3D12_HEAP_PROPERTIES props;
            memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_DEFAULT;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Alignment = 0;
            desc.Width = width;
            desc.Height = height;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            ID3D12Resource* pTexture = NULL;
            d3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&pTexture));

            UINT uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
            UINT uploadSize = height * uploadPitch;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Alignment = 0;
            desc.Width = uploadSize;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            ID3D12Resource* uploadBuffer = NULL;
            HRESULT hr = d3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer));
            IM_ASSERT(SUCCEEDED(hr));

            void* mapped = NULL;
            D3D12_RANGE range = { 0, uploadSize };
            hr = uploadBuffer->Map(0, &range, &mapped);
            IM_ASSERT(SUCCEEDED(hr));
            for (int y = 0; y < height; y++)
                memcpy((void*)((uintptr_t)mapped + y * uploadPitch), pixels + y * width * 4, width * 4);
            uploadBuffer->Unmap(0, &range);

            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = uploadBuffer;
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srcLocation.PlacedFootprint.Footprint.Width = width;
            srcLocation.PlacedFootprint.Footprint.Height = height;
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

            D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
            dstLocation.pResource = pTexture;
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = pTexture;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            ID3D12Fence* fence = NULL;
            hr = d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
            IM_ASSERT(SUCCEEDED(hr));

            HANDLE event = CreateEvent(0, 0, 0, 0);
            IM_ASSERT(event != NULL);

            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 1;

            ID3D12CommandAllocator* cmdAlloc = NULL;
            hr = d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
            IM_ASSERT(SUCCEEDED(hr));

            ID3D12GraphicsCommandList* cmdList = NULL;
            hr = d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, NULL, IID_PPV_ARGS(&cmdList));
            IM_ASSERT(SUCCEEDED(hr));

            cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
            cmdList->ResourceBarrier(1, &barrier);

            hr = cmdList->Close();
            IM_ASSERT(SUCCEEDED(hr));

            graphicsQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmdList);
            hr = graphicsQueue->Signal(fence, 1);
            IM_ASSERT(SUCCEEDED(hr));

            fence->SetEventOnCompletion(1, event);
            WaitForSingleObject(event, INFINITE);

            cmdList->Release();
            cmdAlloc->Release();
            CloseHandle(event);
            fence->Release();
            uploadBuffer->Release();

            // Create texture view
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            imguiFontTextureSRV = AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
            d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, imguiFontTextureSRV);
        }

        // Store our identifier
        static_assert(sizeof(ImTextureID) >= sizeof(imguiFontTextureSRV.ptr), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
        io.Fonts->TexID = (ImTextureID)imguiFontTextureSRV.ptr;
    }

    static void ImGui_ImplDX12_SetupRenderState(
        ImDrawData* draw_data,
        ID3D12GraphicsCommandList* ctx,
        ImGui_ImplDX12_RenderBuffers* fr,
        ID3D12PipelineState* pipeline,
        ID3D12RootSignature* rootSignature)
    {
        // Setup orthographic projection matrix into our constant buffer
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
        VERTEX_CONSTANT_BUFFER vertex_constant_buffer;
        {
            float L = draw_data->DisplayPos.x;
            float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
            float T = draw_data->DisplayPos.y;
            float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
            float mvp[4][4] =
            {
                { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
                { 0.0f,         0.0f,           0.5f,       0.0f },
                { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
            };
            memcpy(&vertex_constant_buffer.mvp, mvp, sizeof(mvp));
        }

        // Setup viewport
        D3D12_VIEWPORT vp;
        memset(&vp, 0, sizeof(D3D12_VIEWPORT));
        vp.Width = draw_data->DisplaySize.x;
        vp.Height = draw_data->DisplaySize.y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0.0f;
        ctx->RSSetViewports(1, &vp);

        // Bind shader and vertex buffers
        unsigned int stride = sizeof(ImDrawVert);
        unsigned int offset = 0;
        D3D12_VERTEX_BUFFER_VIEW vbv;
        memset(&vbv, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
        vbv.BufferLocation = fr->VertexBuffer->GetGPUVirtualAddress() + offset;
        vbv.SizeInBytes = fr->VertexBufferSize * stride;
        vbv.StrideInBytes = stride;
        ctx->IASetVertexBuffers(0, 1, &vbv);
        D3D12_INDEX_BUFFER_VIEW ibv;
        memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
        ibv.BufferLocation = fr->IndexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = fr->IndexBufferSize * sizeof(ImDrawIdx);
        ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        ctx->IASetIndexBuffer(&ibv);
        ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->SetPipelineState(pipeline);
        ctx->SetGraphicsRootSignature(rootSignature);
        ctx->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);

        // Setup blend factor
        const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
        ctx->OMSetBlendFactor(blend_factor);
    }

    void D3D12GraphicsImpl::RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* commandList)
    {
        // Avoid rendering when minimized
        if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
            return;

        ImGuiViewportDataDx12* render_data = (ImGuiViewportDataDx12*)draw_data->OwnerViewport->RendererUserData;
        render_data->FrameIndex++;
        ImGui_ImplDX12_RenderBuffers* fr = &render_data->FrameRenderBuffers[render_data->FrameIndex % kInflightFrameCount];

        // Create and grow vertex/index buffers if needed
        if (fr->VertexBuffer == NULL || fr->VertexBufferSize < draw_data->TotalVtxCount)
        {
            SafeRelease(fr->VertexBuffer);
            fr->VertexBufferSize = draw_data->TotalVtxCount + 5000;
            D3D12_HEAP_PROPERTIES props;
            memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            D3D12_RESOURCE_DESC desc;
            memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = fr->VertexBufferSize * sizeof(ImDrawVert);
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (d3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&fr->VertexBuffer)) < 0)
                return;
        }
        if (fr->IndexBuffer == NULL || fr->IndexBufferSize < draw_data->TotalIdxCount)
        {
            SafeRelease(fr->IndexBuffer);
            fr->IndexBufferSize = draw_data->TotalIdxCount + 10000;
            D3D12_HEAP_PROPERTIES props;
            memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            D3D12_RESOURCE_DESC desc;
            memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = fr->IndexBufferSize * sizeof(ImDrawIdx);
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (d3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&fr->IndexBuffer)) < 0)
                return;
        }

        // Upload vertex/index data into a single contiguous GPU buffer
        void* vtx_resource, * idx_resource;
        D3D12_RANGE range;
        memset(&range, 0, sizeof(D3D12_RANGE));
        if (fr->VertexBuffer->Map(0, &range, &vtx_resource) != S_OK)
            return;
        if (fr->IndexBuffer->Map(0, &range, &idx_resource) != S_OK)
            return;
        ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource;
        ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        fr->VertexBuffer->Unmap(0, &range);
        fr->IndexBuffer->Unmap(0, &range);

        // Setup desired DX state
        ImGui_ImplDX12_SetupRenderState(draw_data, commandList, fr, imguiPipelineState, imguiRootSignature);

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;
        ImVec2 clip_off = draw_data->DisplayPos;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != NULL)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                        ImGui_ImplDX12_SetupRenderState(draw_data, commandList, fr, imguiPipelineState, imguiRootSignature);
                    else
                        pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    // Apply Scissor, Bind texture, Draw
                    const D3D12_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
                    commandList->SetGraphicsRootDescriptorTable(1, CopyDescriptorsToGPUHeap(1, *(D3D12_CPU_DESCRIPTOR_HANDLE*)&pcmd->TextureId));
                    commandList->RSSetScissorRects(1, &r);
                    commandList->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
                }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
        }
    }
}
