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
#include <atomic>
#include <queue>
#include <mutex>
#include <memory>
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

    class D3D12CommandContext;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();
        D3D12GraphicsDevice(GPUFlags flags);
        ~D3D12GraphicsDevice();

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
        void AllocateGPUDescriptors(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE* OutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGPUHandle);
        D3D12MapResult AllocateGPUMemory(uint64_t size, uint64_t alignment);

        // Resource upload/init
        UploadContext ResourceUploadBegin(uint64_t size);
        void ResourceUploadEnd(UploadContext& context);

        
        //void CommitCommandBuffer(D3D12CommandBuffer* commandBuffer, bool waitForCompletion);

        void WaitForGPU();
        bool BeginFrame() override;
        void EndFrame() override;
        void HandleDeviceLost();

        IDXGIFactory4* GetDXGIFactory() const { return dxgiFactory.Get(); }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }

        ID3D12Device* GetD3DDevice() const { return d3dDevice; }
        D3D12MA::Allocator* GetAllocator() const { return allocator; }
        bool SupportsRenderPass() const { return supportsRenderPass; }
        ID3D12CommandQueue* GetGraphicsQueue() const { return graphicsQueue; }

    private:
        void GetAdapter(IDXGIAdapter1** ppAdapter, bool lowPower = false);
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);

        //CommandContext* GetImmediateContext() const override;

        /* Resource creation methods */
        SharedPtr<SwapChain> CreateSwapChain(const SwapChainDescriptor& descriptor) override;
        
        void InitializeUpload();
        void ShutdownUpload();
        void EndFrameUpload();
        void ClearFinishedUploads(uint64_t flushCount);

        // ImGui
        ID3D12RootSignature* imguiRootSignature = nullptr;
        ID3D12PipelineState* imguiPipelineState = nullptr;
        ID3D12Resource* imguiFontTextureResource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE imguiFontTextureSRV;

        void InitImGui();
        void ShutdownImgui(bool all);
        bool CreateImguiObjects();
        void CreateImguiFontsTexture();
        void RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* graphics_command_list);

        D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptorsToGPUHeap(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE srcBaseHandle);

        DWORD dxgiFactoryFlags = 0;
        ComPtr<IDXGIFactory4> dxgiFactory = nullptr;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;

        D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* allocator = nullptr;
        /// Current supported feature level.
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        /// Root signature version
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        bool supportsRenderPass = false;

        ID3D12CommandQueue* graphicsQueue;
        ID3D12CommandQueue* computeQueue;
        ID3D12CommandQueue* copyQueue;
        std::unique_ptr<D3D12CommandContext> immediateContext;

        std::atomic<uint32_t> commandBufferCount{ 0 };
        std::mutex commandBufferAllocationMutex;
        std::queue<D3D12CommandContext*> commandBufferQueue;
        std::vector<std::unique_ptr<D3D12CommandContext>> commandBufferPool;
        std::vector<ID3D12CommandList*> pendingCommandLists;

        /// Current active frame index
        uint32_t frameIndex{ 0 };

        /* Descriptor heaps */
        DescriptorHeap RtvHeap{};
        DescriptorHeap DsvHeap{};
        DescriptorHeap CbvSrvUavCpuHeap;
        DescriptorHeap CbvSrvUavGpuHeaps[kInflightFrameCount];

        /* Main swap chain */
        DXGI_FORMAT backbufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

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
        static constexpr uint64_t kTempBufferSize = 4 * 1024 * 1024;

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

        D3D12MA::Allocation* tempBufferAllocations[kInflightFrameCount] = { };
        ID3D12Resource* tempFrameBuffers[kInflightFrameCount] = { };
        uint8_t* tempFrameCPUMem[kInflightFrameCount] = { };
        uint64_t tempFrameGPUMem[kInflightFrameCount] = { };
        volatile int64_t tempFrameUsed = 0;
    };
}
