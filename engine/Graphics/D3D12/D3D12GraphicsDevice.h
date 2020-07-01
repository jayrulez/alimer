//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#pragma once

#include "Graphics/GraphicsDevice.h"
#include "D3D12Backend.h"
#include <vector>
#include <queue>
#include <mutex>
struct ImDrawData;

namespace alimer
{
    struct UploadContext
    {
        ID3D12GraphicsCommandList* commandList;
        void* CPUAddress = nullptr;
        uint64_t ResourceOffset = 0;
        ID3D12Resource* Resource = nullptr;
        void* Submission = nullptr;
    };


    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        D3D12GraphicsDevice(bool enableValidationLayer);
        ~D3D12GraphicsDevice();

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count, bool shaderVisible);
        void AllocateGPUDescriptors(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE* OutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGPUHandle);
        // Temporary CPU-writable buffer memory
        D3D12MapResult AllocateGPUMemory(uint64_t size, uint64_t alignment);

        // Resource upload/init
        UploadContext ResourceUploadBegin(uint64_t size);
        void ResourceUploadEnd(UploadContext& context);

        void WaitForGPU() override;
        void HandleDeviceLost();

        IDXGIFactory4* GetDXGIFactory() const { return dxgiFactory; }
        bool IsTearingSupported() const { return tearingSupported; }

        ID3D12Device* GetD3DDevice() const { return d3dDevice; }
        D3D12MA::Allocator* GetAllocator() const { return allocator; }
        ID3D12CommandQueue* GetGraphicsQueue() const { return graphicsQueue; }
        bool SupportsRenderPass() const { return supportsRenderPass; }

    private:
        void GetAdapter(IDXGIAdapter1** ppAdapter, bool lowPower = false);
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        bool BackendInitialize(const PresentationParameters& presentationParameters) override;
        void Shutdown() override;
        bool BeginFrameImpl() override;
        void EndFrameImpl() override;

        void InitializeUpload();
        void ShutdownUpload();
        void EndFrameUpload();
        void ClearFinishedUploads(uint64_t flushCount);

        // Resource creation methods.
        RefPtr<Texture> CreateTexture(const TextureDescription& desc, const void* initialData) override;

        CommandList BeginCommandList(const char* name) override;
        void InsertDebugMarker(CommandList commandList, const char* name) override;
        void PushDebugGroup(CommandList commandList, const char* name) override;
        void PopDebugGroup(CommandList commandList) override;

        void SetScissorRect(CommandList commandList, const Rect& scissorRect) override;
        void SetScissorRects(CommandList commandList, const Rect* scissorRects, uint32_t count) override;
        void SetViewport(CommandList commandList, const Viewport& viewport) override;
        void SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count) override;
        void SetBlendColor(CommandList commandList, const Color& color) override;

        void BindBuffer(CommandList commandList, uint32_t slot, GraphicsBuffer* buffer) override;
        void BindBufferData(CommandList commandList, uint32_t slot, const void* data, uint32_t size) override;

        void InitDescriptorHeap(DescriptorHeap* heap, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);
        void CreateUIObjects();
        void CreateFontsTexture();
        void DestroyUIObjects();
        void SetupRenderState(ImDrawData* drawData, CommandList commandList);
        void RenderDrawData(ImDrawData* drawData, CommandList commandList);
        D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptorsToGPUHeap(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE srcBaseHandle);

        static constexpr uint64_t kRenderLatency = 2;

        bool supportsRenderPass = false;

        IDXGIFactory4* dxgiFactory = nullptr;
        DWORD dxgiFactoryFlags = 0;
        bool tearingSupported = false;
        D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* allocator = nullptr;
        /// Current supported feature level.
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        /// Root signature version
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        /* Command queues */
        ID3D12CommandQueue* graphicsQueue = nullptr;

        /* Descriptor heaps */
        DescriptorHeap RTVHeap{};
        DescriptorHeap DSVHeap{};
        DescriptorHeap CPUDescriptorHeap;
        DescriptorHeap GPUDescriptorHeaps[kRenderLatency];

        /* Main swap chain */
        DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        uint32_t maxInflightFrames = 2u;
        uint32_t backBufferCount = 2u;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
        ID3D12Resource* swapChainRenderTargets[3] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE swapChainRenderTargetDescriptor[3] = {};

        std::atomic<uint8_t> commandlistCount{ 0 };
        ThreadSafeRingBuffer<CommandList, kMaxCommandLists> freeCommandLists;
        ThreadSafeRingBuffer<CommandList, kMaxCommandLists> activeCommandLists;

        struct Frame
        {
            ID3D12CommandAllocator* commandAllocators[kMaxCommandLists] = {};
        };
        Frame frames[kRenderLatency];
        Frame& frame() { return frames[frameIndex]; }

        ID3D12GraphicsCommandList* commandLists[kMaxCommandLists] = {};
        inline ID3D12GraphicsCommandList* GetCommandList(CommandList cmd) { return commandLists[cmd]; }

        // Presentation fence objects.
        ID3D12Fence* frameFence = nullptr;
        HANDLE frameFenceEvent;
        uint64_t frameCount{ 0 };

        /* Upload data */
        struct UploadSubmission
        {
            ID3D12CommandAllocator* commandAllocator = nullptr;
            ID3D12GraphicsCommandList1* commandList = nullptr;
            uint64_t Offset = 0;
            uint64_t Size = 0;
            uint64_t FenceValue = 0;
            uint64_t Padding = 0;

            void Reset()
            {
                Offset = 0;
                Size = 0;
                FenceValue = 0;
                Padding = 0;
            }
        };


        UploadSubmission* AllocUploadSubmission(uint64_t size);

        ID3D12CommandQueue* uploadCommandQueue = nullptr;
        ID3D12Fence* uploadFence = nullptr;
        HANDLE uploadFenceEvent;
        uint64_t uploadFenceValue = 0;

        static constexpr uint64_t kUploadBufferSize = 256 * 1024 * 1024;
        static constexpr uint64_t kMaxUploadSubmissions = 16;

        // These are protected by UploadSubmissionLock
        uint64_t uploadBufferStart = 0;
        uint64_t uploadBufferUsed = 0;
        UploadSubmission uploadSubmissions[kMaxUploadSubmissions];
        uint64_t uploadSubmissionStart = 0;
        uint64_t uploadSubmissionUsed = 0;

        D3D12MA::Allocation* uploadBufferAllocation = nullptr;
        ID3D12Resource* uploadBuffer = nullptr;
        uint8_t* uploadBufferCPUAddr = nullptr;
        SRWLOCK uploadSubmissionLock = SRWLOCK_INIT;
        SRWLOCK uploadQueueLock = SRWLOCK_INIT;

        const uint64_t tempBufferSize = 2 * 1024 * 1024;
        D3D12MA::Allocation* tempBufferAllocations[kRenderLatency] = { };
        ID3D12Resource* tempFrameBuffers[kRenderLatency] = { };
        uint8_t* tempFrameCPUMem[kRenderLatency] = { };
        uint64_t tempFrameGPUMem[kRenderLatency] = { };
        volatile int64_t tempFrameUsed = 0;

        // Imgui objects.
        ID3D12RootSignature* uiRootSignature = nullptr;
        ID3D12PipelineState* uiPipelineState = nullptr;
        RefPtr<Texture> fontTexture;
    };
}
