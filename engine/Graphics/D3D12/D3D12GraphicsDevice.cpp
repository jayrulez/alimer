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
#include "D3D12SwapChain.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "Core/String.h"
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
        inline D3D12_RESOURCE_DIMENSION D3D12GetResourceDimension(TextureType type)
        {
            switch (type)
            {
            case TextureType::Texture1D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

            case TextureType::Texture2D:
            case TextureType::TextureCube:
                return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            case TextureType::Texture3D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            default:
                ALIMER_UNREACHABLE();
                return D3D12_RESOURCE_DIMENSION_UNKNOWN;
            }
        }

        struct ImGuiViewportDataD3D12
        {
            IDXGISwapChain3* swapChain = nullptr;

            ImGuiViewportDataD3D12(uint32_t frameCount)
            {
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

        struct ImGuiConstants
        {
            Matrix4x4 mvp;
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

    D3D12GraphicsDevice::D3D12GraphicsDevice()
    {
        ALIMER_VERIFY(IsAvailable());

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ID3D12Debug* debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                ID3D12Debug1* d3d12Debug1 = nullptr;
                if (SUCCEEDED(debugController->QueryInterface(&d3d12Debug1)))
                {
                    d3d12Debug1->SetEnableGPUBasedValidation(true);
                    //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                    d3d12Debug1->Release();
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
#endif

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

        IDXGIAdapter1* adapter;
        GetAdapter(&adapter);

        // Create the DX12 API device object.
        {
            ThrowIfFailed(D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&d3dDevice)));
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
                desc.pAdapter = adapter;

                ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &allocator));
                switch (allocator->GetD3D12Options().ResourceHeapTier)
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

            InitCapabilities(adapter);
            adapter->Release();
        }

        // Create command queue's
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 1;

            ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&graphicsQueue)));
            graphicsQueue->SetName(L"Direct Command Queue");
        }

        // Setup upload data.
        InitializeUpload();

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
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        BackendShutdown();
    }

    void D3D12GraphicsDevice::BackendShutdown()
    {
        SAFE_RELEASE(RTVHeap.handle);
        SAFE_RELEASE(DSVHeap.handle);
        for (uint32_t index = 0; index < maxInflightFrames; ++index)
        {
            SAFE_RELEASE(GPUDescriptorHeaps[index].handle);
            //SAFE_RELEASE(GPUUploadMemoryHeaps[Idx].handle);
        }
        SAFE_RELEASE(CPUDescriptorHeap.handle);

        /*for (uint32_t i = 0; i < kMaxCommandLists; i++)
        {
            if (commandLists[i]) {
                commandLists[i]->Release();
                commandLists[i] = nullptr;

                for (uint32_t index = 0; index < maxInflightFrames; ++index)
                {
                    SAFE_RELEASE(frames[index].commandAllocators[i]);
                }
            }
        }

        //SAFE_RELEASE(commandList);*/

        /*ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        if (ImGuiViewportDataD3D12* data = (ImGuiViewportDataD3D12*)mainViewport->RendererUserData)
        {
            IM_DELETE(data);
        }
        mainViewport->RendererUserData = nullptr;

        SAFE_RELEASE(uiRootSignature);
        SAFE_RELEASE(uiPipelineState);
        fontTexture.Reset();*/

        // Release leaked resources.
        ReleaseTrackedResources();

        ShutdownUpload();

        SAFE_RELEASE(graphicsQueue);
        CloseHandle(frameFenceEvent);
        SAFE_RELEASE(frameFence);
        /*for (uint32_t i = 0; i < backBufferCount; i++)
        {
            SAFE_RELEASE(swapChainRenderTargets[i]);
        }

        //swapChain.Reset();*/

        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            LOG_ERROR("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);

        ULONG refCount = d3dDevice->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            LOG_DEBUG("Direct3D12: There are {} unreleased references left on the device", refCount);

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
            IDXGIDebug1* dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug->Release();
            }
        }
#endif
    }
    
    void D3D12GraphicsDevice::InitializeUpload()
    {
        for (uint64_t i = 0; i < kMaxUploadSubmissions; ++i) {
            UploadSubmission& submission = uploadSubmissions[i];
            ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&submission.commandAllocator)));
            ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, submission.commandAllocator, nullptr, IID_PPV_ARGS(&submission.commandList)));
            ThrowIfFailed(submission.commandList->Close());

            submission.commandList->SetName(L"Upload Command List");
        }

        D3D12_COMMAND_QUEUE_DESC queueDesc = { };
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&uploadCommandQueue)));
        uploadCommandQueue->SetName(L"Upload Copy Queue");

        // Fence
        ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence)));
        uploadFence->SetName(L"Upload Fence");

        uploadFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (!uploadFenceEvent)
        {
            LOG_ERROR("Direct3D12: CreateEventEx failed.");
        }

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc = { };
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = kUploadBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Alignment = 0;

        ThrowIfFailed(allocator->CreateResource(
            &allocationDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &uploadBufferAllocation,
            IID_PPV_ARGS(&uploadBuffer)
        ));

        D3D12_RANGE readRange = { };
        ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferCPUAddr)));

        // Temporary buffer memory that swaps every frame
        resourceDesc.Width = kTempBufferSize;

        for (uint64_t i = 0; i < kRenderLatency; ++i)
        {
            ThrowIfFailed(allocator->CreateResource(
                &allocationDesc,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                &tempBufferAllocations[i],
                IID_PPV_ARGS(&tempFrameBuffers[i]))
            );

            ThrowIfFailed(tempFrameBuffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&tempFrameCPUMem[i])));
            tempFrameGPUMem[i] = tempFrameBuffers[i]->GetGPUVirtualAddress();
        }
    }

    void D3D12GraphicsDevice::ShutdownUpload()
    {
        for (uint64_t i = 0; i < kMaxUploadSubmissions; ++i) {
            SafeRelease(uploadSubmissions[i].commandAllocator);
            SafeRelease(uploadSubmissions[i].commandList);
        }

        for (uint64_t i = 0; i < _countof(tempFrameBuffers); ++i)
        {
            SafeRelease(tempBufferAllocations[i]);
            SafeRelease(tempFrameBuffers[i]);
        }

        SafeRelease(uploadBufferAllocation);
        SafeRelease(uploadBuffer);
        SafeRelease(uploadCommandQueue);
        CloseHandle(uploadFenceEvent);
        SafeRelease(uploadFence);
    }

    void D3D12GraphicsDevice::EndFrameUpload()
    {
        // If we can grab the lock, try to clear out any completed submissions
        if (TryAcquireSRWLockExclusive(&uploadSubmissionLock))
        {
            ClearFinishedUploads(0);

            ReleaseSRWLockExclusive(&uploadSubmissionLock);
        }

        {
            AcquireSRWLockExclusive(&uploadQueueLock);

            // Make sure to sync on any pending uploads
            ClearFinishedUploads(0);
            graphicsQueue->Wait(uploadFence, uploadFenceValue);

            ReleaseSRWLockExclusive(&uploadQueueLock);
        }

        tempFrameUsed = 0;
    }

    D3D12GraphicsDevice::UploadSubmission* D3D12GraphicsDevice::AllocUploadSubmission(uint64_t size)
    {
        ALIMER_ASSERT(uploadSubmissionUsed <= kMaxUploadSubmissions);
        if (uploadSubmissionUsed == kMaxUploadSubmissions)
            return nullptr;

        const uint64_t submissionIdx = (uploadSubmissionStart + uploadSubmissionUsed) % kMaxUploadSubmissions;
        ALIMER_ASSERT(uploadSubmissions[submissionIdx].Size == 0);

        ALIMER_ASSERT(uploadBufferUsed <= kUploadBufferSize);
        if (size > (kUploadBufferSize - uploadBufferUsed))
            return nullptr;

        const uint64_t start = uploadBufferStart;
        const uint64_t end = uploadBufferStart + uploadBufferUsed;
        uint64_t allocOffset = uint64_t(-1);
        uint64_t padding = 0;
        if (end < kUploadBufferSize)
        {
            const uint64_t endAmt = kUploadBufferSize - end;
            if (endAmt >= size)
            {
                allocOffset = end;
            }
            else if (start >= size)
            {
                // Wrap around to the beginning
                allocOffset = 0;
                uploadBufferUsed += endAmt;
                padding = endAmt;
            }
        }
        else
        {
            const uint64_t wrappedEnd = end % kUploadBufferSize;
            if ((start - wrappedEnd) >= size)
                allocOffset = wrappedEnd;
        }

        if (allocOffset == uint64_t(-1))
            return nullptr;

        uploadSubmissionUsed += 1;
        uploadBufferUsed += size;

        UploadSubmission* submission = &uploadSubmissions[submissionIdx];
        submission->Offset = allocOffset;
        submission->Size = size;
        submission->FenceValue = uint64_t(-1);
        submission->Padding = padding;

        return submission;
    }


    void D3D12GraphicsDevice::ClearFinishedUploads(uint64_t flushCount)
    {
        const uint64_t start = uploadSubmissionStart;
        const uint64_t used = uploadSubmissionUsed;
        for (uint64_t i = 0; i < used; ++i)
        {
            const uint64_t idx = (start + i) % kMaxUploadSubmissions;
            UploadSubmission& submission = uploadSubmissions[idx];
            ALIMER_ASSERT(submission.Size > 0);
            ALIMER_ASSERT(uploadBufferUsed >= submission.Size);

            // If the submission hasn't been sent to the GPU yet we can't wait for it
            if (submission.FenceValue == uint64_t(-1))
                return;

            if (i < flushCount)
            {
                // Wait for fence value
                if (uploadFence->GetCompletedValue() < submission.FenceValue)
                {
                    ThrowIfFailed(uploadFence->SetEventOnCompletion(submission.FenceValue, uploadFenceEvent));
                    WaitForSingleObject(uploadFenceEvent, INFINITE);
                }
            }

            if (uploadFence->GetCompletedValue() >= submission.FenceValue)
            {
                uploadSubmissionStart = (uploadSubmissionStart + 1) % kMaxUploadSubmissions;
                uploadSubmissionUsed -= 1;
                uploadBufferStart = (uploadBufferStart + submission.Padding) % kUploadBufferSize;
                ALIMER_ASSERT(submission.Offset == uploadBufferStart);
                ALIMER_ASSERT(uploadBufferStart + submission.Size <= kUploadBufferSize);
                uploadBufferStart = (uploadBufferStart + submission.Size) % kUploadBufferSize;
                uploadBufferUsed -= (submission.Size + submission.Padding);
                submission.Reset();

                if (uploadBufferUsed == 0)
                    uploadBufferStart = 0;
            }
        }
    }

    void D3D12GraphicsDevice::GetAdapter(IDXGIAdapter1** ppAdapter, bool lowPower)
    {
        *ppAdapter = nullptr;

        IDXGIAdapter1* adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6 = nullptr;
        HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
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
                if (SUCCEEDED(D3D12CreateDevice(adapter, minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
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
            for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
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
                if (SUCCEEDED(D3D12CreateDevice(adapter, minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
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
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            LOG_ERROR("No Direct3D 12 device found");
        }

        *ppAdapter = adapter;
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
        graphicsQueue->Signal(frameFence, ++frameCount);
        frameFence->SetEventOnCompletion(frameCount, frameFenceEvent);
        WaitForSingleObject(frameFenceEvent, INFINITE);

        GPUDescriptorHeaps[frameIndex].Size = 0;
        //GPUUploadMemoryHeaps[frameIndex].Size = 0;
    }

    static CommandList commandList = 0;

    bool D3D12GraphicsDevice::BeginFrameImpl()
    {
        /*if (!uiPipelineState)
            CreateUIObjects();

        commandList = BeginCommandList("Frame");

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = swapChainRenderTargets[backbufferIndex];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        GetCommandList(commandList)->ResourceBarrier(1, &barrier);
        Color clearColor = Colors::CornflowerBlue;

        GetCommandList(commandList)->ClearRenderTargetView(swapChainRenderTargetDescriptor[backbufferIndex], &clearColor.r, 0, nullptr);
        GetCommandList(commandList)->OMSetRenderTargets(1, &swapChainRenderTargetDescriptor[backbufferIndex], FALSE, nullptr);*/

        return true;
    }

    void D3D12GraphicsDevice::EndFrameImpl()
    {
        /*RenderDrawData(ImGui::GetDrawData(), commandList);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = swapChainRenderTargets[backbufferIndex];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        GetCommandList(commandList)->ResourceBarrier(1, &barrier);
        */

        EndFrameUpload();

        // Execute all command lists
        {
            ID3D12CommandList* commandLists[kMaxCommandLists];
            uint32_t commandListsCount = 0;

            CommandList cmd;
            while (activeCommandLists.pop_front(cmd))
            {
                // TODO: Perform query resolve.
                ThrowIfFailed(GetCommandList(cmd)->Close());

                commandLists[commandListsCount] = GetCommandList(cmd);
                commandListsCount++;

                freeCommandLists.push_back(cmd);
            }

            graphicsQueue->ExecuteCommandLists(commandListsCount, commandLists);
        }

        if (swapChain) {
            swapChain->Present(1, 0);
            //backbufferIndex = swapChain->GetCurrentBackBufferIndex();
        }

        graphicsQueue->Signal(frameFence, ++frameCount);

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

    SwapChain* D3D12GraphicsDevice::CreateSwapChain(const SwapChainDescription& desc)
    {
        return new D3D12SwapChain(this, desc);
    }

    Texture* D3D12GraphicsDevice::CreateTexture(const TextureDescription& desc, const void* initialData)
    {
        return new D3D12Texture(this, desc, initialData);
    }

    CommandList D3D12GraphicsDevice::BeginCommandList(const char* name)
    {
        CommandList cmd;
        if (!freeCommandLists.pop_front(cmd))
        {
            // need to create one more command list:
            cmd = commandlistCount.fetch_add(1);
            assert(cmd < kMaxCommandLists);

            for (uint32_t i = 0; i < maxInflightFrames; ++i)
            {
                ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frames[i].commandAllocators[cmd])));
            }

            ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[0].commandAllocators[cmd], nullptr, IID_PPV_ARGS(&commandLists[cmd])));
            ThrowIfFailed(commandLists[cmd]->Close());
        }

        ThrowIfFailed(frame().commandAllocators[cmd]->Reset());
        ThrowIfFailed(commandLists[cmd]->Reset(frame().commandAllocators[cmd], nullptr));
        commandLists[cmd]->SetDescriptorHeaps(1, &GPUDescriptorHeaps[frameIndex].handle);

        activeCommandLists.push_back(cmd);
        return cmd;
    }

    void D3D12GraphicsDevice::InsertDebugMarker(CommandList commandList, const char* name)
    {
        auto wideName = ToUtf16(name, strlen(name));
        PIXSetMarker(commandLists[commandList], PIX_COLOR_DEFAULT, wideName.c_str());
    }

    void D3D12GraphicsDevice::PushDebugGroup(CommandList commandList, const char* name)
    {
        auto wideName = ToUtf16(name, strlen(name));
        PIXBeginEvent(commandLists[commandList], PIX_COLOR_DEFAULT, wideName.c_str());
    }

    void D3D12GraphicsDevice::PopDebugGroup(CommandList commandList)
    {
        PIXEndEvent(commandLists[commandList]);
    }

    void D3D12GraphicsDevice::SetScissorRect(CommandList commandList, const Rect& scissorRect)
    {
        D3D12_RECT d3dScissorRect;
        d3dScissorRect.left = LONG(scissorRect.x);
        d3dScissorRect.top = LONG(scissorRect.y);
        d3dScissorRect.right = LONG(scissorRect.x + scissorRect.width);
        d3dScissorRect.bottom = LONG(scissorRect.y + scissorRect.height);
        commandLists[commandList]->RSSetScissorRects(1, &d3dScissorRect);
    }

    void D3D12GraphicsDevice::SetScissorRects(CommandList commandList, const Rect* scissorRects, uint32_t count)
    {
        D3D12_RECT d3dScissorRects[kMaxViewportAndScissorRects];
        for (uint32_t i = 0; i < count; ++i)
        {
            d3dScissorRects[i].left = LONG(scissorRects[i].x);
            d3dScissorRects[i].top = LONG(scissorRects[i].y);
            d3dScissorRects[i].right = LONG(scissorRects[i].x + scissorRects[i].width);
            d3dScissorRects[i].bottom = LONG(scissorRects[i].y + scissorRects[i].height);
        }
        commandLists[commandList]->RSSetScissorRects(count, d3dScissorRects);
    }

    void D3D12GraphicsDevice::SetViewport(CommandList commandList, const Viewport& viewport)
    {
        commandLists[commandList]->RSSetViewports(1, reinterpret_cast<const D3D12_VIEWPORT*>(&viewport));
    }

    void D3D12GraphicsDevice::SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count)
    {
        D3D12_VIEWPORT d3dViewports[kMaxViewportAndScissorRects];
        for (uint32_t i = 0; i < count; ++i)
        {
            d3dViewports[i].TopLeftX = viewports[i].x;
            d3dViewports[i].TopLeftY = viewports[i].y;
            d3dViewports[i].Width = viewports[i].width;
            d3dViewports[i].Height = viewports[i].height;
            d3dViewports[i].MinDepth = viewports[i].minDepth;
            d3dViewports[i].MaxDepth = viewports[i].maxDepth;
        }
        commandLists[commandList]->RSSetViewports(count, d3dViewports);
    }

    void D3D12GraphicsDevice::SetBlendColor(CommandList commandList, const Color& color)
    {
        commandLists[commandList]->OMSetBlendFactor(color.Data());
    }

    void D3D12GraphicsDevice::BindBuffer(CommandList commandList, uint32_t slot, GraphicsBuffer* buffer)
    {
        D3D12Buffer* d3dBuffer = static_cast<D3D12Buffer*>(buffer);

        uint64_t offset = 0;
        D3D12_GPU_VIRTUAL_ADDRESS bufferLocation = d3dBuffer->GetGpuVirtualAddress() + offset;

        const bool compute = false;
        if (compute) {
            commandLists[commandList]->SetComputeRootConstantBufferView(slot, bufferLocation);
        }
        else {
            commandLists[commandList]->SetGraphicsRootConstantBufferView(slot, bufferLocation);
        }
    }

    void D3D12GraphicsDevice::BindBufferData(CommandList commandList, uint32_t slot, const void* data, uint32_t size)
    {
        D3D12MapResult tempMem = AllocateGPUMemory(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        memcpy(tempMem.CPUAddress, data, size);
        commandLists[commandList]->SetGraphicsRootConstantBufferView(slot, tempMem.GPUAddress);
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

    D3D12MapResult D3D12GraphicsDevice::AllocateGPUMemory(uint64_t size, uint64_t alignment)
    {
        uint64_t allocSize = size + alignment;
        uint64_t offset = InterlockedAdd64(&tempFrameUsed, allocSize) - allocSize;
        if (alignment > 0)
            offset = AlignTo(offset, alignment);
        ALIMER_ASSERT(offset + size <= kTempBufferSize);

        D3D12MapResult result;
        result.CPUAddress = tempFrameCPUMem[frameIndex] + offset;
        result.GPUAddress = tempFrameGPUMem[frameIndex] + offset;
        result.ResourceOffset = offset;
        result.Resource = tempFrameBuffers[frameIndex];

        return result;
    }

    UploadContext D3D12GraphicsDevice::ResourceUploadBegin(uint64_t size)
    {
        size = AlignTo(size, 512);
        ALIMER_ASSERT(size <= kUploadBufferSize);
        ALIMER_ASSERT(size > 0);

        UploadSubmission* submission = nullptr;

        {
            AcquireSRWLockExclusive(&uploadSubmissionLock);

            ClearFinishedUploads(0);

            submission = AllocUploadSubmission(size);
            while (submission == nullptr)
            {
                ClearFinishedUploads(1);
                submission = AllocUploadSubmission(size);
            }

            ReleaseSRWLockExclusive(&uploadSubmissionLock);
        }

        ThrowIfFailed(submission->commandAllocator->Reset());
        ThrowIfFailed(submission->commandList->Reset(submission->commandAllocator, nullptr));

        UploadContext context;
        context.commandList = submission->commandList;
        context.Resource = uploadBuffer;
        context.CPUAddress = uploadBufferCPUAddr + submission->Offset;
        context.ResourceOffset = submission->Offset;
        context.Submission = submission;

        return context;
    }

    void D3D12GraphicsDevice::ResourceUploadEnd(UploadContext& context)
    {
        ALIMER_ASSERT(context.commandList != nullptr);
        ALIMER_ASSERT(context.Submission != nullptr);
        UploadSubmission* submission = reinterpret_cast<UploadSubmission*>(context.Submission);

        {
            AcquireSRWLockExclusive(&uploadQueueLock);

            // Finish off and execute the command list
            ThrowIfFailed(submission->commandList->Close());
            ID3D12CommandList* cmdLists[1] = { submission->commandList };
            uploadCommandQueue->ExecuteCommandLists(1, cmdLists);

            ++uploadFenceValue;
            uploadCommandQueue->Signal(uploadFence, uploadFenceValue);
            submission->FenceValue = uploadFenceValue;

            ReleaseSRWLockExclusive(&uploadQueueLock);
        }

        context = UploadContext();
    }

    void D3D12GraphicsDevice::HandleDeviceLost()
    {

    }

    void D3D12GraphicsDevice::CreateUIObjects()
    {
#if TODO
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

            D3D12_ROOT_PARAMETER1 rootParameters[2] = {};

            rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParameters[0].Descriptor.ShaderRegister = 0;
            rootParameters[0].Descriptor.RegisterSpace = 0;
            rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

            rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
            rootParameters[1].DescriptorTable.pDescriptorRanges = &descRange;
            rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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
            desc.Desc_1_1.NumParameters = _countof(rootParameters);
            desc.Desc_1_1.pParameters = rootParameters;
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
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.pRootSignature = uiRootSignature;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = backBufferFormat;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.NodeMask = 1;
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

            if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexShaderBlob, nullptr)))
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
#endif // TODO

    }

    void D3D12GraphicsDevice::CreateFontsTexture()
    {
        // Build texture atlas
       /* ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        // Upload texture to graphics system
        {
            TextureDescription textureDesc = {};
            textureDesc.name = "ImGui FontTexture";
            textureDesc.type = TextureType::Texture2D;
            textureDesc.format = PixelFormat::RGBA8UNorm;
            textureDesc.usage = TextureUsage::Sampled;
            textureDesc.width = width;
            textureDesc.height = height;
            textureDesc.mipLevels = 1u;
            textureDesc.sampleCount = 1u;
            fontTexture = CreateTexture(textureDesc, pixels);

        }

        // Store our identifier
        static_assert(sizeof(ImTextureID) >= sizeof(fontTexture), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
        io.Fonts->TexID = (ImTextureID)fontTexture.Get();*/
    }

    void D3D12GraphicsDevice::DestroyUIObjects()
    {

    }

    void D3D12GraphicsDevice::SetupRenderState(ImDrawData* drawData, CommandList commandList)
    {
        ImGuiViewportDataD3D12* render_data = (ImGuiViewportDataD3D12*)drawData->OwnerViewport->RendererUserData;

        // Setup orthographic projection matrix into our constant buffer
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
        ImGuiConstants vertex_constant_buffer;
        {
            float L = drawData->DisplayPos.x;
            float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
            float T = drawData->DisplayPos.y;
            float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
            Matrix4x4::CreateOrthographicOffCenter(L, R, B, T, -1.0f, 1.0f, &vertex_constant_buffer.mvp);
        }

        // Setup viewport
        SetViewport(commandList, Viewport(0.0f, 0.0f, drawData->DisplaySize.x, drawData->DisplaySize.y));

        GetCommandList(commandList)->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //GetCommandList(commandList)->SetPipelineState(uiPipelineState);
        //GetCommandList(commandList)->SetGraphicsRootSignature(uiRootSignature);
        BindBufferData(commandList, 0, &vertex_constant_buffer, sizeof(Matrix4x4));

        // Setup blend factor
        Color blendFactor(0.f, 0.f, 0.f, 0.f);
        SetBlendColor(commandList, blendFactor);
    }

    void D3D12GraphicsDevice::RenderDrawData(ImDrawData* drawData, CommandList commandList)
    {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        if (!drawData->TotalVtxCount || !drawData->TotalIdxCount)
            return;

        ImGuiViewportDataD3D12* render_data = (ImGuiViewportDataD3D12*)drawData->OwnerViewport->RendererUserData;

        // Get memory for vertex and index buffers
        const uint64_t vbSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
        const uint64_t ibSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
        D3D12MapResult vertexMem = AllocateGPUMemory(vbSize, 4);
        D3D12MapResult indexMem = AllocateGPUMemory(ibSize, 4);

        // Copy and convert all vertices into a single contiguous buffer
        ImDrawVert* vertexCPUMem = reinterpret_cast<ImDrawVert*>(vertexMem.CPUAddress);
        ImDrawIdx* indexCPUMem = reinterpret_cast<ImDrawIdx*>(indexMem.CPUAddress);
        for (int i = 0; i < drawData->CmdListsCount; i++)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];
            memcpy(vertexCPUMem, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(indexCPUMem, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vertexCPUMem += drawList->VtxBuffer.Size;
            indexCPUMem += drawList->IdxBuffer.Size;
        }

        D3D12_VERTEX_BUFFER_VIEW vbView = {};
        vbView.BufferLocation = vertexMem.GPUAddress;
        vbView.SizeInBytes = UINT(vbSize);
        vbView.StrideInBytes = sizeof(ImDrawVert);
        GetCommandList(commandList)->IASetVertexBuffers(0, 1, &vbView);

        // Bind shader and vertex buffers
        D3D12_INDEX_BUFFER_VIEW ibView = { };
        ibView.BufferLocation = indexMem.GPUAddress;
        ibView.SizeInBytes = UINT(ibSize);
        ibView.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        GetCommandList(commandList)->IASetIndexBuffer(&ibView);

        // Setup desired DX state
        SetupRenderState(drawData, commandList);

        ImVec2 clip_off = drawData->DisplayPos;
        ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        uint32_t vtxOffset = 0;
        uint32_t idxOffset = 0;
        for (int commandListIndex = 0; commandListIndex < drawData->CmdListsCount; commandListIndex++)
        {
            const ImDrawList* drawList = drawData->CmdLists[commandListIndex];
            for (int cmdIdx = 0; cmdIdx < drawList->CmdBuffer.Size; cmdIdx++)
            {
                const ImDrawCmd* pcmd = &drawList->CmdBuffer[cmdIdx];
                if (pcmd->UserCallback != nullptr)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                        SetupRenderState(drawData, commandList);
                    else
                        pcmd->UserCallback(drawList, pcmd);
                }
                else
                {
                    // Bind SRV
                    Texture* texture = (Texture*)pcmd->TextureId;
                    D3D12_CPU_DESCRIPTOR_HANDLE SRV = static_cast<D3D12Texture*>(texture)->GetSRV();
                    GetCommandList(commandList)->SetGraphicsRootDescriptorTable(1, CopyDescriptorsToGPUHeap(1, SRV));

                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clip_rect;
                    clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                    clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                    clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                    clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                    if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                    {
                        // Negative offsets are illegal for vkCmdSetScissor
                        if (clip_rect.x < 0.0f)
                            clip_rect.x = 0.0f;
                        if (clip_rect.y < 0.0f)
                            clip_rect.y = 0.0f;

                        Rect scissor;
                        scissor.x = clip_rect.x;
                        scissor.y = clip_rect.y;
                        scissor.width = clip_rect.z - clip_rect.x;
                        scissor.height = clip_rect.w - clip_rect.y;
                        SetScissorRect(commandList, scissor);

                        GetCommandList(commandList)->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset, 0);
                    }
                }
            }
            idxOffset += drawList->IdxBuffer.Size;
            vtxOffset += drawList->VtxBuffer.Size;
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
