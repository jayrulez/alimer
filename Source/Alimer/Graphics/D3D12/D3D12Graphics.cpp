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

#include "Graphics/Graphics.h"
#include "D3D12Backend.h"
//#include "D3D12Texture.h"
//#include "D3D12Buffer.h"
//#include "D3D12CommandContext.h"

#include "Core/Window.h"
#include "Math/Matrix4x4.h"
#include <atomic>

//#include <imgui.h>
//#include <imgui_internal.h>
//#include <d3dcompiler.h>
//#ifdef _MSC_VER
//#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
//#endif

using Microsoft::WRL::ComPtr;

#if defined(_DEBUG)
#   define ENABLE_GPU_VALIDATION 0
#else
#   define ENABLE_GPU_VALIDATION 0
#endif

namespace alimer::Graphics
{
    struct SwapChain
    {
        IDXGISwapChain3* Handle;
        ID3D12Resource* Backbuffers[3];
        D3D12_CPU_DESCRIPTOR_HANDLE BackbuffersHandles[3];
        uint32_t BackBufferIndex;
    };

    static DWORD dxgiFactoryFlags = 0;
    static bool isTearingSupported = false;

    IDXGIFactory4* Factory = nullptr;
    IDXGIAdapter1* Adapter = nullptr;
    ID3D12Device* Device = nullptr;
    D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
    D3D12MA::Allocator* Allocator = nullptr;

    ID3D12CommandQueue* GraphicsQueue = nullptr;
    ID3D12Fence* FrameFence = nullptr;
    HANDLE FrameFenceEvent = INVALID_HANDLE_VALUE;

    /* Descriptor heaps */
    DescriptorHeap RtvHeap{};
    DescriptorHeap DsvHeap{};
    DescriptorHeap CbvSrvUavCpuHeap;
    DescriptorHeap CbvSrvUavGpuHeaps[kInflightFrameCount];

    SwapChain MainSwapChain;

    GraphicsCapabilities caps = {};
    bool SupportsRenderPass = false;
    uint64 NumFrames = 0;
    uint32 FrameIndex = 0;

    /* CommandLists */
    struct CommandListD3D12
    {
        ID3D12CommandAllocator* commandAllocators[kInflightFrameCount];
        ID3D12GraphicsCommandList* handle;
    };

    std::atomic<CommandList> CommandListCount{ 0 };
    CommandListD3D12 CommandLists[kMaxCommandLists] = {};
    static ID3D12CommandAllocator* GetCommandAllocator(CommandList cmd);
    static ID3D12GraphicsCommandList* GetCommandList(CommandList cmd);

    static void InitCapabilities();

    /* Heaps */
    static D3D12_CPU_DESCRIPTOR_HANDLE AllocateCPUDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    static void AllocateGPUDescriptors(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE* OutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGPUHandle);
    static D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptorsToGPUHeap(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE srcBaseHandle);

    /* SwapChain */
    static SwapChain CreateSwapChain(WindowHandle handle, uint32_t width, uint32_t height, bool fullscreen);
    static void DestroySwapChain(SwapChain* swapChain);

    bool Initialize(alimer::Window& window)
    {
        if (Device != nullptr) {
            return true;
        }

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
            {
                debugController->EnableDebugLayer();

#if ENABLE_GPU_VALIDATION
                ComPtr<ID3D12Debug1> d3d12Debug1;
                if (SUCCEEDED(debugController.As(&d3d12Debug1)))
                {
                    d3d12Debug1->SetEnableGPUBasedValidation(true);
                    //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                }
#endif
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
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
            }
        }
#endif

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            ComPtr<IDXGIFactory5> dxgiFactory5;
            HRESULT hr = Factory->QueryInterface(dxgiFactory5.GetAddressOf());
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
                isTearingSupported = true;
            }
        }

        // Detect adapter.
#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* factory6;
        HRESULT hr = Factory->QueryInterface(&factory6);
        if (SUCCEEDED(hr))
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                    IID_PPV_ARGS(&Adapter)));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(Adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    Adapter->Release();

                    continue;
                }

                // Try to create device.
                if (SUCCEEDED(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device))))
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
        if (!Adapter)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(Factory->EnumAdapters1(adapterIndex, &Adapter)); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(Adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    Adapter->Release();

                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device))))
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

        if (!Adapter)
        {
            throw std::exception("No Direct3D 12 device found");
        }

        Device->SetName(L"Alimer Device");

#ifndef NDEBUG
        // Configure debug device (if active).
        {
            ID3D12InfoQueue* d3dInfoQueue;
            if (SUCCEEDED(Device->QueryInterface(&d3dInfoQueue)))
            {
#ifdef _DEBUG
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
                D3D12_MESSAGE_ID hide[] =
                {
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

        // Init caps
        InitCapabilities();

        // Create memory allocator.
        D3D12MA::ALLOCATOR_DESC desc = {};
        desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
        desc.pDevice = Device;
        desc.pAdapter = Adapter;

        ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &Allocator));
        switch (Allocator->GetD3D12Options().ResourceHeapTier)
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

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;
        ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&GraphicsQueue)));
        GraphicsQueue->SetName(L"Graphics Command Queue");

        // Create a fence for tracking GPU execution progress.
        {
            ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&FrameFence)));
            FrameFence->SetName(L"Frame Fence");

            FrameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
            if (!FrameFenceEvent)
            {
                LOGE("Direct3D12: CreateEventEx failed.");
            }
        }


        // Init descriptor heaps.
        {
            RtvHeap = D3D12CreateDescriptorHeap(Device, 1024, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
            DsvHeap = D3D12CreateDescriptorHeap(Device, 1024, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
            CbvSrvUavCpuHeap = D3D12CreateDescriptorHeap(Device, 16 * 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

            for (uint32_t i = 0; i < kInflightFrameCount; ++i)
            {
                CbvSrvUavGpuHeaps[i] = D3D12CreateDescriptorHeap(Device, 16 * 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            }
        }

        MainSwapChain = CreateSwapChain(window.GetHandle(), window.GetWidth(), window.GetHeight(), window.IsFullscreen());

        return true;
    }

    static void InitCapabilities()
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(Adapter->GetDesc1(&desc));

        caps.backendType = BackendType::Direct3D12;
        caps.vendorId = desc.VendorId;
        caps.deviceId = desc.DeviceId;

        eastl::wstring deviceName(desc.Description);
        caps.adapterName = ToUtf8(deviceName);

        // Detect adapter type.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            caps.adapterType = GPUAdapterType::CPU;
        }
        else {
            D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
            ThrowIfFailed(Device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

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

        HRESULT hr = Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
        if (SUCCEEDED(hr))
        {
            FeatureLevel = featLevels.MaxSupportedFeatureLevel;
        }
        else
        {
            FeatureLevel = D3D_FEATURE_LEVEL_11_0;
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
        if (SUCCEEDED(Device->CheckFeatureSupport(
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

        SupportsRenderPass = false;
        if (d3d12options5.RenderPassesTier > D3D12_RENDER_PASS_TIER_0 &&
            static_cast<GPUKnownVendorId>(caps.vendorId) != GPUKnownVendorId::Intel)
        {
            SupportsRenderPass = true;
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

            ThrowIfFailed(Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)));
        }
    }

    void Shutdown()
    {
        WaitForGPU();

        // Allocator
        {
            D3D12MA::Stats stats;
            Allocator->CalculateStats(&stats);

            if (stats.Total.UsedBytes > 0) {
                LOGE("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
            }

            SafeRelease(Allocator);
        }

        SafeRelease(RtvHeap.handle);
        SafeRelease(DsvHeap.handle);
        SafeRelease(CbvSrvUavCpuHeap.handle);
        for (uint32_t i = 0; i < kInflightFrameCount; ++i)
        {
            SafeRelease(CbvSrvUavGpuHeaps[i].handle);
            //SafeRelease(GPUUploadMemoryHeaps[Idx].Heap);
        }

        for (CommandList cmd = 0; cmd < kMaxCommandLists; cmd++)
        {
            if (!CommandLists[cmd].handle)
                continue;

            for (uint32_t index = 0; index < kInflightFrameCount; ++index)
            {
                SafeRelease(CommandLists[cmd].commandAllocators[index]);
            }

            SafeRelease(CommandLists[cmd].handle);
        }

        DestroySwapChain(&MainSwapChain);
        SafeRelease(GraphicsQueue);
        CloseHandle(FrameFenceEvent);
        SafeRelease(FrameFence);

        ULONG refCount = Device->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            LOGD("Direct3D12: There are {} unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice;
            if (SUCCEEDED(Device->QueryInterface(&debugDevice)))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        SafeRelease(Adapter);
        SafeRelease(Factory);

#ifdef _DEBUG
        {
            IDXGIDebug1* dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
                dxgiDebug->Release();
            }
        }
#endif
    }

    bool BeginFrame()
    {
        return true;
    }

    void SubmitCommandLists()
    {
        uint32_t numCommandLists = 0;
        ID3D12CommandList* commandLists[kMaxCommandLists];

        CommandList cmd_last = CommandListCount.load();
        CommandListCount.store(0);
        for (CommandList cmd = 0; cmd < cmd_last; ++cmd)
        {
            // TODO: Perform query resolves (must be outside of render pass)

            /* Hack: Until we add render pass support. */
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = MainSwapChain.Backbuffers[MainSwapChain.BackBufferIndex];
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            GetCommandList(cmd)->ResourceBarrier(1, &barrier);

            // Close the command list.
            ThrowIfFailed(GetCommandList(cmd)->Close());
            commandLists[numCommandLists] = GetCommandList(cmd);
            numCommandLists++;
        }

        GraphicsQueue->ExecuteCommandLists(numCommandLists, commandLists);
    }

    void EndFrame(bool vsync)
    {
        /* TODO: Sync compute and copy queue */

        // Submit all command lists.
        SubmitCommandLists();

        HRESULT hr;
        if (vsync)
            hr = MainSwapChain.Handle->Present(1, 0);
        else
            hr = MainSwapChain.Handle->Present(0, isTearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {

#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? Device->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            //Device->HandleDeviceLost();
            return;
        }

        GraphicsQueue->Signal(FrameFence, ++NumFrames);

        uint64_t GPUFrameCount = FrameFence->GetCompletedValue();

        if ((NumFrames - GPUFrameCount) >= kInflightFrameCount)
        {
            FrameFence->SetEventOnCompletion(GPUFrameCount + 1, FrameFenceEvent);
            WaitForSingleObject(FrameFenceEvent, INFINITE);
        }

        FrameIndex = NumFrames % kInflightFrameCount;
        MainSwapChain.BackBufferIndex = MainSwapChain.Handle->GetCurrentBackBufferIndex();

        // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
        if (!Factory->IsCurrent())
        {
            Factory->Release();
            ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&Factory)));
        }
    }

    void WaitForGPU(void)
    {
        GraphicsQueue->Signal(FrameFence, ++NumFrames);
        FrameFence->SetEventOnCompletion(NumFrames, FrameFenceEvent);
        WaitForSingleObject(FrameFenceEvent, INFINITE);

        CbvSrvUavGpuHeaps[FrameIndex].Size = 0;
        //GPUUploadMemoryHeaps[currentFrameIndex].Size = 0;
    }

    const GraphicsCapabilities& GetCapabilities(void)
    {
        return caps;
    }

    uint64 GetFrameCount(void)
    {
        return NumFrames;
    }

    uint32 GetFrameIndex(void)
    {
        return FrameIndex;
    }

    static ID3D12CommandAllocator* GetCommandAllocator(CommandList cmd)
    {
        return CommandLists[cmd].commandAllocators[FrameIndex];
    }

    static ID3D12GraphicsCommandList* GetCommandList(CommandList cmd)
    {
        return CommandLists[cmd].handle;
    }

    CommandList BeginCommandList(void)
    {
        CommandList commandList = CommandListCount.fetch_add(1);
        if (GetCommandList(commandList) == nullptr)
        {
            // need to create one more command list:
            ALIMER_ASSERT(commandList < kMaxCommandLists);

            for (uint32_t i = 0; i < kInflightFrameCount; ++i)
            {
                ThrowIfFailed(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandLists[commandList].commandAllocators[i])));
            }

            ThrowIfFailed(Device->CreateCommandList(0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                CommandLists[commandList].commandAllocators[0],
                nullptr,
                IID_PPV_ARGS(&CommandLists[commandList].handle)
            ));
            ThrowIfFailed(GetCommandList(commandList)->Close());
        }

        ThrowIfFailed(GetCommandAllocator(commandList)->Reset());
        ThrowIfFailed(GetCommandList(commandList)->Reset(GetCommandAllocator(commandList), nullptr));

        /* Until we move to render pass logic. */
        auto clearColor = Colors::CornflowerBlue;
        auto rtvDescriptor = MainSwapChain.BackbuffersHandles[MainSwapChain.BackBufferIndex];
        //auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = MainSwapChain.Backbuffers[MainSwapChain.BackBufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

        GetCommandList(commandList)->ResourceBarrier(1, &barrier);
        GetCommandList(commandList)->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
        GetCommandList(commandList)->ClearRenderTargetView(rtvDescriptor, &clearColor.r, 0, nullptr);
        //GetCommandList(commandList)->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        return commandList;
    }

    /* Helper methods */
    static D3D12_CPU_DESCRIPTOR_HANDLE AllocateCPUDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
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

        ALIMER_ASSERT((descriptorHeap->Size + count) < descriptorHeap->Capacity);

        D3D12_CPU_DESCRIPTOR_HANDLE Handle;
        Handle.ptr = descriptorHeap->CpuStart.ptr + (size_t)descriptorHeap->Size * descriptorHeap->DescriptorSize;
        descriptorHeap->Size += count;
        return Handle;
    }

    static void AllocateGPUDescriptors(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE* OutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGPUHandle)
    {
        ALIMER_ASSERT(OutCPUHandle && OutGPUHandle && count > 0);

        DescriptorHeap* descriptorHeap = &CbvSrvUavGpuHeaps[FrameIndex];
        ALIMER_ASSERT((descriptorHeap->Size + count) < descriptorHeap->Capacity);

        OutCPUHandle->ptr = descriptorHeap->CpuStart.ptr + (uint64)descriptorHeap->Size * descriptorHeap->DescriptorSize;
        OutGPUHandle->ptr = descriptorHeap->GpuStart.ptr + (uint64)descriptorHeap->Size * descriptorHeap->DescriptorSize;

        descriptorHeap->Size += count;
    }

    static D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptorsToGPUHeap(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE srcBaseHandle)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CPUBaseHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUBaseHandle;
        AllocateGPUDescriptors(count, &CPUBaseHandle, &GPUBaseHandle);
        Device->CopyDescriptorsSimple(count, CPUBaseHandle, srcBaseHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return GPUBaseHandle;
    }

    SwapChain CreateSwapChain(WindowHandle window, uint32_t width, uint32_t height, bool fullscreen)
    {
        UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        if (isTearingSupported)
            flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        const DXGI_SCALING dxgiScaling = DXGI_SCALING_STRETCH;
#else
        const DXGI_SCALING dxgiScaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
#endif

        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = 0;
        swapchainDesc.Height = 0;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.Stereo = false;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.BufferCount = kInflightFrameCount;
        swapchainDesc.Scaling = dxgiScaling;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapchainDesc.Flags = flags;

        IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapchainDesc = {};
        fsSwapchainDesc.Windowed = !fullscreen;

        // Create a SwapChain from a Win32 window.
        ThrowIfFailed(Factory->CreateSwapChainForHwnd(
            GraphicsQueue,
            window,
            &swapchainDesc,
            &fsSwapchainDesc,
            nullptr,
            &tempSwapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(Factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
            device->GetGraphicsQueue(),
            window,
            &swapchainDesc,
            nullptr,
            tempSwapChain.GetAddressOf()
        ));
#endif

        SwapChain swapChain = {};
        ThrowIfFailed(tempSwapChain->QueryInterface(&swapChain.Handle));
        SafeRelease(tempSwapChain);

        for (uint32_t i = 0; i < kInflightFrameCount; i++)
        {
            ThrowIfFailed(swapChain.Handle->GetBuffer(i, IID_PPV_ARGS(&swapChain.Backbuffers[i])));

            swapChain.BackbuffersHandles[i] = AllocateCPUDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
            Device->CreateRenderTargetView(swapChain.Backbuffers[i], nullptr, swapChain.BackbuffersHandles[i]);
        }

        swapChain.BackBufferIndex = swapChain.Handle->GetCurrentBackBufferIndex();

        return swapChain;
    }

    static void DestroySwapChain(SwapChain* swapChain)
    {
        for (uint32_t i = 0; i < kInflightFrameCount; i++)
        {
            SafeRelease(swapChain->Backbuffers[i]);
        }

        SafeRelease(swapChain->Handle);
    }
}
