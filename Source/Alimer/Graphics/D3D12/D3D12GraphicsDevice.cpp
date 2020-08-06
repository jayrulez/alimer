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
//#include "D3D12SwapChain.h"
//#include "D3D12Texture.h"
//#include "D3D12Buffer.h"
//#include "D3D12CommandContext.h"

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

    D3D12GraphicsDevice::D3D12GraphicsDevice(GPUDeviceFlags flags, D3D_FEATURE_LEVEL minFeatureLevel)
    {
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(flags & GPUDeviceFlags::DebugRuntime)
            || any(flags & GPUDeviceFlags::GPUBaseValidation))
        {
            ID3D12Debug* debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                if (any(flags & GPUDeviceFlags::GPUBaseValidation))
                {
                    ID3D12Debug1* d3d12Debug1;
                    if (SUCCEEDED(debugController->QueryInterface(&d3d12Debug1)))
                    {
                        d3d12Debug1->SetEnableGPUBasedValidation(true);
                        //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                        d3d12Debug1->Release();
                    }
                }
                debugController->Release();
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

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

        dxgiFactoryCaps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;

        // Determines whether tearing support is available for fullscreen borderless windows.
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
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                dxgiFactoryCaps |= DXGIFactoryCaps::Tearing;
            }

            SafeRelease(dxgiFactory5);
        }

        // Enum adapter and create device.
        {
            IDXGIAdapter1* dxgiAdapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
            IDXGIFactory6* dxgiFactory6;
            HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
            if (SUCCEEDED(hr))
            {
                DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
                if (any(flags & GPUDeviceFlags::LowPowerPreference))
                {
                    gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
                }

                for (UINT adapterIndex = 0;
                    SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
                        adapterIndex,
                        gpuPreference,
                        IID_PPV_ARGS(&dxgiAdapter)));
                    adapterIndex++)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    {
                        // Don't select the Basic Render Driver adapter.
                        dxgiAdapter->Release();

                        continue;
                    }

                    // Check to see if the adapter supports Direct3D 12 and create the actual device yet.
                    if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter, minFeatureLevel, _uuidof(ID3D12Device), (void**)&d3dDevice)))
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

            SafeRelease(dxgiFactory);
#endif

            if (!dxgiAdapter)
            {
                for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, &dxgiAdapter)); ++adapterIndex)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    {
                        // Don't select the Basic Render Driver adapter.
                        dxgiAdapter->Release();

                        continue;
                    }

                    // Check to see if the adapter supports Direct3D 12 and create the actual device yet.
                    if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter, minFeatureLevel, _uuidof(ID3D12Device), (void**)&d3dDevice)))
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

            if (!dxgiAdapter)
            {
                LOGE("No Direct3D 12 device found");
                return;
            }

            d3dDevice->SetName(L"Alimer Device");

            // Init caps
            InitCapabilities(dxgiAdapter);

            // Create memory allocator.
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice;
            desc.pAdapter = dxgiAdapter;

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

            // Release adapter now.
            dxgiAdapter->Release();
        }

        // Configure debug device (if active).
        if (any(flags & GPUDeviceFlags::DebugRuntime)
            || any(flags & GPUDeviceFlags::GPUBaseValidation))
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

#if TODO



        // Create immediate context.
        immediateContext.reset(new D3D12CommandContext(this, D3D12_COMMAND_LIST_TYPE_DIRECT));

        // Create main swapchain.
        //SwapchainDescription swapchainDesc = {};
        //mainSwapchain.reset(new D3D12Swapchain(this, swapchainDesc));


        // Init imgui backend.
        //InitImGui();  
#endif // TODO

    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        Shutdown();
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            LOGE("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);

        CloseHandle(frameFenceEvent);
        SafeRelease(frameFence);

        //ReleaseTrackedResources();

        // Command queues
        {
            //commandBufferPool.clear();
            SafeRelease(graphicsQueue);
        }

        //ShutdownImgui(true);

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
        SafeRelease(dxgiFactory);

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

    bool D3D12GraphicsDevice::Initialize(Window& window)
    {
        // Create command queue's
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 0;
            ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&graphicsQueue)));
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

        return true;
    }

    void D3D12GraphicsDevice::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

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

    void D3D12GraphicsDevice::WaitForGPU()
    {
        graphicsQueue->Signal(frameFence, ++frameCount);
        frameFence->SetEventOnCompletion(frameCount, frameFenceEvent);
        WaitForSingleObject(frameFenceEvent, INFINITE);

        CbvSrvUavGpuHeaps[frameIndex].Size = 0;
        //GPUUploadMemoryHeaps[Gfx->FrameIndex].Size = 0;
    }

    bool D3D12GraphicsDevice::BeginFrame()
    {
        //if (!imguiPipelineState)
        //    CreateImguiObjects();

        return true;
    }

    uint64_t D3D12GraphicsDevice::EndFrame()
    {
        return 0;
    }

    void D3D12GraphicsDevice::HandleDeviceLost()
    {

    }

    /*CommandBuffer& D3D12GraphicsImpl::BeginCommandBuffer(const std::string_view id)
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
    }*/

    /*Swapchain* D3D12GraphicsDevice::GetMainSwapchain() const
    {
        return mainSwapchain.get();
    }

    CommandContext* D3D12GraphicsDevice::GetImmediateContext() const
    {
        return immediateContext.get();
    }*/

    /*void D3D12GraphicsImpl::CommitCommandBuffer(D3D12CommandBuffer* commandBuffer, bool waitForCompletion)
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
        commandBufferQueue.push(commandBuffer);*
    }*/

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12GraphicsDevice::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
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

    void D3D12GraphicsDevice::AllocateGPUDescriptors(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE* OutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGPUHandle)
    {
        ALIMER_ASSERT(OutCPUHandle && OutGPUHandle && count > 0);

        DescriptorHeap* descriptorHeap = &CbvSrvUavGpuHeaps[frameIndex];

        assert((descriptorHeap->Size + count) < descriptorHeap->Capacity);

        OutCPUHandle->ptr = descriptorHeap->CpuStart.ptr + (uint64)descriptorHeap->Size * descriptorHeap->DescriptorSize;
        OutGPUHandle->ptr = descriptorHeap->GpuStart.ptr + (uint64)descriptorHeap->Size * descriptorHeap->DescriptorSize;

        descriptorHeap->Size += count;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12GraphicsDevice::CopyDescriptorsToGPUHeap(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE srcBaseHandle)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CPUBaseHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUBaseHandle;
        AllocateGPUDescriptors(count, &CPUBaseHandle, &GPUBaseHandle);
        d3dDevice->CopyDescriptorsSimple(count, CPUBaseHandle, srcBaseHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return GPUBaseHandle;
    }

    /* Resource creation methods */
    Buffer* D3D12GraphicsDevice::CreateBuffer(const eastl::string_view& name)
    {
        return nullptr;
    }
}
