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
#include "D3D12Texture.h"
#include "core/String.h"
#include <d3dcompiler.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif


using Microsoft::WRL::ComPtr;

namespace alimer
{
    namespace
    {
        struct FrameResources
        {
            ID3D12Resource* IndexBuffer;
            ID3D12Resource* VertexBuffer;
            int IndexBufferSize;
            int VertexBufferSize;
        };


        struct ImGuiViewportDataD3D12
        {
            IDXGISwapChain3* swapChain = nullptr;
            FrameResources* frame;

            ImGuiViewportDataD3D12(uint32_t frameCount)
            {
                frame = new FrameResources[frameCount];

                for (UINT i = 0; i < frameCount; ++i)
                {
                    //FrameCtx[i].CommandAllocator = NULL;
                    //FrameCtx[i].RenderTarget = NULL;

                    // Create buffers with a default size (they will later be grown as needed)
                    frame[i].IndexBuffer = NULL;
                    frame[i].VertexBuffer = NULL;
                    frame[i].VertexBufferSize = 5000;
                    frame[i].IndexBufferSize = 10000;
                }
            }

            ~ImGuiViewportDataD3D12()
            {
            }
        };

        static void ImGui_D3D12_CreateWindow(ImGuiViewport* viewport)
        {
            ImGuiViewportDataD3D12* data = IM_NEW(ImGuiViewportDataD3D12)(2u);
            viewport->RendererUserData = data;
        }

        static void ImGui_D3D12_DestroyWindow(ImGuiViewport* viewport)
        {
            // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
            if (ImGuiViewportDataD3D12* data = (ImGuiViewportDataD3D12*)viewport->RendererUserData)
            {
            }

            viewport->RendererUserData = nullptr;
        }

        static void ImGui_D3D12_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
        {
            ImGuiViewportDataD3D12* data = (ImGuiViewportDataD3D12*)viewport->RendererUserData;
        }

        static void ImGui_D3D12_RenderWindow(ImGuiViewport* viewport, void*)
        {
            ImGuiViewportDataD3D12* data = (ImGuiViewportDataD3D12*)viewport->RendererUserData;
        }

        static void ImGui_D3D12_SwapBuffers(ImGuiViewport* viewport, void*)
        {
            ImGuiViewportDataD3D12* data = (ImGuiViewportDataD3D12*)viewport->RendererUserData;

            // Present without vsync
            ThrowIfFailed(data->swapChain->Present(0, 0));
            //while (data->Fence->GetCompletedValue() < data->FenceSignaledValue)
            //    ::SwitchToThread();
        }

        struct VERTEX_CONSTANT_BUFFER
        {
            float   mvp[4][4];
        };
    }

    bool D3D12GraphicsDevice::IsAvailable()
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
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");
        if (CreateDXGIFactory2 == nullptr) {
            return false;
        }

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

    D3D12GraphicsDevice::D3D12GraphicsDevice(WindowHandle window, const Desc& desc)
        : GraphicsDevice(desc)
    {
        ALIMER_VERIFY(IsAvailable());

        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (desc.enableValidationLayer)
        {
            ID3D12Debug* debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                /*ID3D12Debug1* d3d12Debug1 = nullptr;
                if (SUCCEEDED(debugController->QueryInterface(&d3d12Debug1)))
                {
                    d3d12Debug1->SetEnableGPUBasedValidation(true);
                    //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                    d3d12Debug1->Release();
                }*/

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
#endif
        }

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            tearingSupported = true;
            BOOL allowTearing = FALSE;

            IDXGIFactory5* dxgiFactory5 = nullptr;
            HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory5);
            if (SUCCEEDED(hr))
            {
                hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
                tearingSupported = false;
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }

            SAFE_RELEASE(dxgiFactory5);
        }

        ComPtr<IDXGIAdapter1> adapter;
        GetAdapter(adapter.GetAddressOf());

        // Create the DX12 API device object.
        ThrowIfFailed(D3D12CreateDevice(
            adapter.Get(),
            minFeatureLevel,
            IID_PPV_ARGS(&d3dDevice)
        ));

        d3dDevice->SetName(L"Alimer Device");

        // Configure debug device (if active).
        if (desc.enableValidationLayer)
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

        // Create memory allocator
        {
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice;
            desc.pAdapter = adapter.Get();

            ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &memoryAllocator));
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

        InitCapabilities(adapter.Get());

        // Create command queue's
        {
            D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
            directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            directQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            directQueueDesc.NodeMask = 0;

            ThrowIfFailed(d3dDevice->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&directCommandQueue)));
            directCommandQueue->SetName(L"Direct Command Queue");
        }

        // Create descriptor heaps
        {
            // Render target descriptor heap (RTV).
            InitDescriptorHeap(&RTVHeap, 1024, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
            // Depth-stencil descriptor heap (DSV).
            InitDescriptorHeap(&DSVHeap, 256, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
            // Non-shader visible descriptor heap (CBV, SRV, UAV).
            InitDescriptorHeap(&CPUDescriptorHeap, 16 * 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false);
            // Shader visible descriptor heaps (CBV, SRV, UAV).
            for (uint32_t index = 0; index < maxInflightFrames; ++index)
            {
                InitDescriptorHeap(&GPUDescriptorHeaps[index], 16 * 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
            }
        }

        // Create SwapChain if not headless
        if (window)
        {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            RECT rect;
            BOOL success = GetClientRect(window, &rect);
            ALIMER_ASSERT_MSG(success, "GetWindowRect error.");
            size.width = rect.right - rect.left;
            size.height = rect.bottom - rect.top;
#else
            float dpiscale = 1.0f;
            size.width = uint32_t(window->Bounds.Width * dpiscale);
            size.height = uint32_t(window->Bounds.Height * dpiscale);
#endif

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = size.width;
            swapChainDesc.Height = size.height;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.Stereo = FALSE;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = backBufferCount;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            if (!desc.enableVsync && tearingSupported)
            {
                swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

            ComPtr<IDXGISwapChain1> tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = TRUE;

            ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
                directCommandQueue.Get(),
                window,
                &swapChainDesc,
                &fsSwapChainDesc,
                NULL,
                tempSwapChain.GetAddressOf()
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
            swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

            ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
                directCommandQueue.Get(),
                window,
                &swapChainDesc,
                nullptr,
                tempSwapChain.GetAddressOf()
            ));
#endif

            ThrowIfFailed(tempSwapChain.As(&swapChain));

            for (uint32_t i = 0; i < backBufferCount; i++)
            {
                swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainRenderTargets[i]));
                swapChainRenderTargetDescriptor[i] = AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false);
                d3dDevice->CreateRenderTargetView(swapChainRenderTargets[i], nullptr, swapChainRenderTargetDescriptor[i]);
            }
        }

        // Create frame command list and allocators
        {
            for (uint32_t Idx = 0; Idx < 2; ++Idx)
            {
                ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[Idx])));
            }

            ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0], nullptr, IID_PPV_ARGS(&commandList)));
            ThrowIfFailed(commandList->Close());
        }

        // Create a fence for tracking GPU execution progress.
        {
            ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence)));
            frameFence->SetName(L"Frame Fence");

            frameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
            if (!frameFenceEvent)
            {
                LOG_ERROR("Direct3D12: CreateEventEx failed.");
            }
        }

        // Setup ImGui
        {
            ImGuiIO& io = ImGui::GetIO();
            io.BackendRendererName = "Alimer Direct3D12";
            io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            main_viewport->RendererUserData = IM_NEW(ImGuiViewportDataD3D12)(maxInflightFrames);

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
                platform_io.Renderer_CreateWindow = ImGui_D3D12_CreateWindow;
                platform_io.Renderer_DestroyWindow = ImGui_D3D12_DestroyWindow;
                platform_io.Renderer_SetWindowSize = ImGui_D3D12_SetWindowSize;
                platform_io.Renderer_RenderWindow = ImGui_D3D12_RenderWindow;
                platform_io.Renderer_SwapBuffers = ImGui_D3D12_SwapBuffers;
            }
        }
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        Shutdown();
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        SAFE_RELEASE(RTVHeap.handle);
        SAFE_RELEASE(DSVHeap.handle);
        for (uint32_t Idx = 0; Idx < 2; ++Idx)
        {
            SAFE_RELEASE(commandAllocators[Idx]);
            SAFE_RELEASE(GPUDescriptorHeaps[Idx].handle);
            //SAFE_RELEASE(GPUUploadMemoryHeaps[Idx].handle);
        }
        SAFE_RELEASE(CPUDescriptorHeap.handle);
        SAFE_RELEASE(commandList);

        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        if (ImGuiViewportDataD3D12* data = (ImGuiViewportDataD3D12*)mainViewport->RendererUserData)
        {
            for (UINT i = 0; i < maxInflightFrames; i++)
            {
                SAFE_RELEASE(data->frame[i].IndexBuffer);
                SAFE_RELEASE(data->frame[i].VertexBuffer);
            }
            IM_DELETE(data);
        }
        mainViewport->RendererUserData = nullptr;

        SAFE_RELEASE(uiRootSignature);
        SAFE_RELEASE(uiPipelineState);
        fontTexture.Reset();

        // Release leaked resources.
        ReleaseTrackedResources();

        directCommandQueue.Reset();
        CloseHandle(frameFenceEvent);
        SAFE_RELEASE(frameFence);
        for (uint32_t i = 0; i < backBufferCount; i++)
        {
            SAFE_RELEASE(swapChainRenderTargets[i]);
        }

        swapChain.Reset();

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
        SAFE_RELEASE(dxgiFactory);

#ifdef _DEBUG
        {
            ComPtr<IDXGIDebug1> dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            }
        }
#endif
    }

    void D3D12GraphicsDevice::GetAdapter(IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6 = nullptr;
        HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (desc.powerPreference == PowerPreference::LowPower) {
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

        SAFE_RELEASE(dxgiFactory6);
#endif
        if (!adapter)
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory->EnumAdapters1(
                    adapterIndex,
                    adapter.ReleaseAndGetAddressOf()));
                ++adapterIndex)
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
                LOG_ERROR("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            LOG_ERROR("No Direct3D 12 device found");
        }

        *ppAdapter = adapter.Detach();
    }

    void D3D12GraphicsDevice::InitDescriptorHeap(DescriptorHeap* heap, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
    {
        heap->Size = 0;
        heap->Capacity = capacity;
        heap->DescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(type);

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
        heapDesc.Type = type;
        heapDesc.NumDescriptors = capacity;
        heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0x0;

        ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap->handle)));

        heap->CPUStart = heap->handle->GetCPUDescriptorHandleForHeapStart();
        if (shaderVisible) {
            heap->GPUStart = heap->handle->GetGPUDescriptorHandleForHeapStart();
        }
        else {
            heap->GPUStart.ptr = 0;
        }
    }

    void D3D12GraphicsDevice::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        // Init capabilities
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

            caps.backendType = BackendType::Direct3D12;
            caps.vendorId = static_cast<GPUVendorId>(desc.VendorId);
            caps.deviceId = desc.DeviceId;

            std::wstring deviceName(desc.Description);
            caps.adapterName = alimer::ToUtf8(deviceName);

            // Detect adapter type.
            /*if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                caps.adapterType = GPUAdapterType::CPU;
            }
            else {
                D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
                VHR(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

                caps.adapterType = arch.UMA ? GPUAdapterType::IntegratedGPU : GPUAdapterType::DiscreteGPU;
            }*/

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
            UINT dxgi_fmt_caps = 0;
            for (uint32_t format = (static_cast<uint32_t>(PixelFormat::Unknown) + 1); format < static_cast<uint32_t>(PixelFormat::Count); format++)
            {
                D3D12_FEATURE_DATA_FORMAT_SUPPORT support;
                support.Format = ToDXGIFormat((PixelFormat)format);

                if (support.Format == DXGI_FORMAT_UNKNOWN)
                    continue;

                ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)));
            }
        }
    }

    void D3D12GraphicsDevice::WaitForGPU()
    {
        directCommandQueue->Signal(frameFence, ++frameCount);
        frameFence->SetEventOnCompletion(frameCount, frameFenceEvent);
        WaitForSingleObject(frameFenceEvent, INFINITE);

        GPUDescriptorHeaps[frameIndex].Size = 0;
        //GPUUploadMemoryHeaps[frameIndex].Size = 0;
    }

    bool D3D12GraphicsDevice::BeginFrameImpl()
    {
        if (!uiPipelineState)
            CreateUIObjects();

        commandAllocators[frameIndex]->Reset();
        commandList->Reset(commandAllocators[frameIndex], nullptr);
        commandList->SetDescriptorHeaps(1, &GPUDescriptorHeaps[frameIndex].handle);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = swapChainRenderTargets[backbufferIndex];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        commandList->ResourceBarrier(1, &barrier);
        Color clearColor = Colors::CornflowerBlue;

        commandList->ClearRenderTargetView(swapChainRenderTargetDescriptor[backbufferIndex], &clearColor.r, 0, nullptr);
        commandList->OMSetRenderTargets(1, &swapChainRenderTargetDescriptor[backbufferIndex], FALSE, nullptr);

        return true;
    }

    void D3D12GraphicsDevice::EndFrameImpl()
    {
        RenderDrawData(ImGui::GetDrawData(), commandList);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = swapChainRenderTargets[backbufferIndex];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &barrier);

        commandList->Close();

        ID3D12CommandList* commandLists[] = { commandList };
        directCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        if (swapChain) {
            swapChain->Present(1, 0);
            backbufferIndex = swapChain->GetCurrentBackBufferIndex();
        }

        directCommandQueue->Signal(frameFence, ++frameCount);

        uint64_t GPUFrameCount = frameFence->GetCompletedValue();

        if ((frameCount - GPUFrameCount) >= maxInflightFrames)
        {
            frameFence->SetEventOnCompletion(GPUFrameCount + 1, frameFenceEvent);
            WaitForSingleObject(frameFenceEvent, INFINITE);
        }

        frameIndex = (frameIndex + 1) % maxInflightFrames;
        GPUDescriptorHeaps[frameIndex].Size = 0;
        //GPUUploadMemoryHeaps[frameIndex].Size = 0;
    }

    RefPtr<Texture> D3D12GraphicsDevice::CreateTexture(const TextureDescription& desc, const void* initialData)
    {
        return new D3D12Texture(this, desc, initialData);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12GraphicsDevice::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count, bool shaderVisible)
    {
        DescriptorHeap* heap = nullptr;
        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        {
            heap = &RTVHeap;
        }
        else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
        {
            heap = &DSVHeap;
        }
        else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        {
            if (!shaderVisible)
            {
                heap = &CPUDescriptorHeap;
            }
            else
            {
                heap = &GPUDescriptorHeaps[frameIndex];
            }
        }

        ALIMER_ASSERT((heap->Size + count) < heap->Capacity);

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        CPUHandle.ptr = heap->CPUStart.ptr + (size_t)heap->Size * heap->DescriptorSize;
        heap->Size += count;
        return CPUHandle;
    }

    void D3D12GraphicsDevice::AllocateGPUDescriptors(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE* OutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGPUHandle)
    {
        DescriptorHeap* heap = &GPUDescriptorHeaps[frameIndex];
        ALIMER_ASSERT((heap->Size + count) < heap->Capacity);

        OutCPUHandle->ptr = heap->CPUStart.ptr + (size_t)heap->Size * heap->DescriptorSize;
        OutGPUHandle->ptr = heap->GPUStart.ptr + (size_t)heap->Size * heap->DescriptorSize;

        heap->Size += count;
    }

    void D3D12GraphicsDevice::HandleDeviceLost()
    {

    }

    void D3D12GraphicsDevice::CreateUIObjects()
    {
        if (uiPipelineState)
            DestroyUIObjects();

        // Create the root signature.
        {
            D3D12_DESCRIPTOR_RANGE1 descRange = {};
            descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            descRange.NumDescriptors = 1;
            descRange.BaseShaderRegister = 0;
            descRange.RegisterSpace = 0;
            descRange.OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 param[2] = {};

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

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
            desc.Version = rootSignatureVersion;
            desc.Desc_1_1.NumParameters = _countof(param);
            desc.Desc_1_1.pParameters = param;
            desc.Desc_1_1.NumStaticSamplers = 1;
            desc.Desc_1_1.pStaticSamplers = &staticSampler;
            desc.Desc_1_1.Flags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

            ID3DBlob* blob = nullptr;
            ID3DBlob* errorBlob = nullptr;
            if (D3D12SerializeVersionedRootSignature(&desc, &blob, &errorBlob) != S_OK)
                return;

            d3dDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&uiRootSignature));
            blob->Release();
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.NodeMask = 1;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.pRootSignature = uiRootSignature;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = backBufferFormat;
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
                return;

            psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };

            // Create the input layout
            static D3D12_INPUT_ELEMENT_DESC local_layout[] = {
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
                return;
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

        ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&uiPipelineState)));
        vertexShaderBlob->Release();
        pixelShaderBlob->Release();

        CreateFontsTexture();
    }

    void D3D12GraphicsDevice::CreateFontsTexture()
    {
        // Build texture atlas
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        // Upload texture to graphics system
        {
            fontTexture = StaticCast<D3D12Texture>(CreateTexture2D(width, height, PixelFormat::RGBA8UNorm, 1, 1, TextureUsage::Sampled, pixels));

            UINT uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
            UINT uploadSize = height * uploadPitch;

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = uploadSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12_HEAP_PROPERTIES props;
            memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            ComPtr<ID3D12Resource> uploadBuffer;
            HRESULT hr = d3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
            IM_ASSERT(SUCCEEDED(hr));

            void* mapped = NULL;
            D3D12_RANGE range = { 0, uploadSize };
            hr = uploadBuffer->Map(0, &range, &mapped);
            IM_ASSERT(SUCCEEDED(hr));
            for (int y = 0; y < height; y++)
                memcpy((void*)((uintptr_t)mapped + y * uploadPitch), pixels + y * width * 4, width * 4);
            uploadBuffer->Unmap(0, &range);

            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = uploadBuffer.Get();
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srcLocation.PlacedFootprint.Footprint.Width = width;
            srcLocation.PlacedFootprint.Footprint.Height = height;
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

            D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
            dstLocation.pResource = fontTexture->GetResource();
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = fontTexture->GetResource();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            ID3D12CommandAllocator* commandAlloc = NULL;
            hr = d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAlloc));
            IM_ASSERT(SUCCEEDED(hr));

            ID3D12GraphicsCommandList* cmdList = NULL;
            hr = d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc, NULL, IID_PPV_ARGS(&cmdList));
            IM_ASSERT(SUCCEEDED(hr));

            cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
            cmdList->ResourceBarrier(1, &barrier);

            hr = cmdList->Close();
            IM_ASSERT(SUCCEEDED(hr));

            directCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmdList);
            WaitForGPU();

            cmdList->Release();
            commandAlloc->Release();

            // Create texture view
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            FontSRV = AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, false);
            d3dDevice->CreateShaderResourceView(fontTexture->GetResource(), &srvDesc, FontSRV);
        }

        // Store our identifier
        /*static_assert(sizeof(ImTextureID) >= sizeof(g_hFontSrvGpuDescHandle.ptr), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
        io.Fonts->TexID = (ImTextureID)g_hFontSrvGpuDescHandle.ptr;*/
    }

    void D3D12GraphicsDevice::DestroyUIObjects()
    {

    }

    void D3D12GraphicsDevice::SetupRenderState(ImDrawData* drawData, ID3D12GraphicsCommandList* commandList)
    {
        ImGuiViewportDataD3D12* render_data = (ImGuiViewportDataD3D12*)drawData->OwnerViewport->RendererUserData;
        FrameResources* fr = &render_data->frame[frameIndex];

        // Setup orthographic projection matrix into our constant buffer
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
        VERTEX_CONSTANT_BUFFER vertex_constant_buffer;
        {
            float L = drawData->DisplayPos.x;
            float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
            float T = drawData->DisplayPos.y;
            float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
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
        vp.Width = drawData->DisplaySize.x;
        vp.Height = drawData->DisplaySize.y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0.0f;
        commandList->RSSetViewports(1, &vp);

        // Bind shader and vertex buffers
        unsigned int stride = sizeof(ImDrawVert);
        unsigned int offset = 0;
        D3D12_VERTEX_BUFFER_VIEW vbv;
        memset(&vbv, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
        vbv.BufferLocation = fr->VertexBuffer->GetGPUVirtualAddress() + offset;
        vbv.SizeInBytes = fr->VertexBufferSize * stride;
        vbv.StrideInBytes = stride;
        commandList->IASetVertexBuffers(0, 1, &vbv);
        D3D12_INDEX_BUFFER_VIEW ibv;
        memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
        ibv.BufferLocation = fr->IndexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = fr->IndexBufferSize * sizeof(ImDrawIdx);
        ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        commandList->IASetIndexBuffer(&ibv);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->SetPipelineState(uiPipelineState);
        commandList->SetGraphicsRootSignature(uiRootSignature);
        commandList->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);

        // Setup blend factor
        const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
        commandList->OMSetBlendFactor(blend_factor);
    }

    void D3D12GraphicsDevice::RenderDrawData(ImDrawData* drawData, ID3D12GraphicsCommandList* commandList)
    {
        // Avoid rendering when minimized
        if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
            return;

        if (drawData->TotalVtxCount == 0)
            return;

        ImGuiViewportDataD3D12* render_data = (ImGuiViewportDataD3D12*)drawData->OwnerViewport->RendererUserData;
        FrameResources* fr = &render_data->frame[frameIndex];

        // Create and grow vertex/index buffers if needed
        if (fr->VertexBuffer == NULL || fr->VertexBufferSize < drawData->TotalVtxCount)
        {
            SAFE_RELEASE(fr->VertexBuffer);
            fr->VertexBufferSize = drawData->TotalVtxCount + 5000;

            D3D12_HEAP_PROPERTIES props = {};
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC desc = {};
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
        if (fr->IndexBuffer == NULL || fr->IndexBufferSize < drawData->TotalIdxCount)
        {
            SAFE_RELEASE(fr->IndexBuffer);
            fr->IndexBufferSize = drawData->TotalIdxCount + 10000;
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
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = drawData->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        fr->VertexBuffer->Unmap(0, &range);
        fr->IndexBuffer->Unmap(0, &range);

        // Setup desired DX state
        SetupRenderState(drawData, commandList);

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;
        ImVec2 clip_off = drawData->DisplayPos;
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = drawData->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != NULL)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                        SetupRenderState(drawData, commandList);
                    else
                        pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    // Apply Scissor, Bind texture, Draw
                    const D3D12_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
                    //commandList->SetGraphicsRootDescriptorTable(1, *(D3D12_GPU_DESCRIPTOR_HANDLE*)&pcmd->TextureId);
                    commandList->SetGraphicsRootDescriptorTable(1, CopyDescriptorsToGPUHeap(1, FontSRV));
                    commandList->RSSetScissorRects(1, &r);
                    commandList->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
                }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
        }
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12GraphicsDevice::CopyDescriptorsToGPUHeap(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE srcBaseHandle)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CPUBaseHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUBaseHandle;
        AllocateGPUDescriptors(count, &CPUBaseHandle, &GPUBaseHandle);
        d3dDevice->CopyDescriptorsSimple(count, CPUBaseHandle, srcBaseHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return GPUBaseHandle;
    }
}
