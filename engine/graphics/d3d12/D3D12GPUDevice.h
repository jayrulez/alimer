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

#pragma once

#include "graphics/GPUDevice.h"
#include "D3D12CommandQueue.h"

namespace alimer
{
    class D3D12GPUProvider;
    class D3D12GPUAdapter;

    /// Direct3D12 GPU backend.
    class ALIMER_API D3D12GPUDevice final : public GPUDevice
    {
    public:
        /// Constructor.
        D3D12GPUDevice(D3D12GPUProvider* provider, D3D12GPUAdapter* adapter);
        /// Destructor.
        ~D3D12GPUDevice() override;

        template<typename T> void DeferredRelease(T*& resource, bool forceDeferred = false)
        {
            IUnknown* base = resource;
            DeferredRelease_(base, forceDeferred);
            resource = nullptr;
        }

        SwapChain* CreateSwapChainCore(const SwapChainDescriptor* descriptor) override;

        ID3D12Device* GetD3DDevice() const { return d3dDevice.Get(); }
        D3D12MA::Allocator* GetMemoryAllocator() const { return allocator; }

        /*D3D12CommandQueue& GetQueue(QueueType type)
        {
            switch (type)
            {
            case QueueType::Compute:
                return computeQueue;
            case QueueType::Copy:
                return copyQueue;
            default:
                return graphicsQueue;
            }
        }

        D3D12CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE type)
        {
            switch (type)
            {
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                return computeQueue;
            case D3D12_COMMAND_LIST_TYPE_COPY:
                return copyQueue;
            default:
                return graphicsQueue;
            }
        }*/

        // The CPU will wait for a fence to reach a specified value
        void WaitForFence(uint64_t fenceValue);

        /* Access to resources. */
        //d3d12::Resource* GetTexture(GPUTexture handle);

    private:
        void Shutdown();
        void InitCapabilities();

        void ExecuteDeferredReleases();
        void DeferredRelease_(IUnknown* resource, bool forceDeferred = false);

        void WaitForIdle() override;
#if TODO
        uint64_t PresentFrame(uint32_t count, const GpuSwapchain* pSwapchains) override;

        /* Fence */
        void CreateFence(d3d12::Fence* fence);
        void DestroyFence(d3d12::Fence* fence);
        uint64_t Signal(d3d12::Fence* fence, ID3D12CommandQueue* queue);
        void SignalAndWait(d3d12::Fence* fence, ID3D12CommandQueue* queue);
        void Wait(d3d12::Fence* fence, uint64_t value);

        /* Swapchain */
        GpuSwapchain CreateSwapChain(void* nativeHandle, uint32_t width, uint32_t height, PresentMode presentMode) override;
        void DestroySwapChain(GpuSwapchain handle) override;
        uint32_t GetImageCount(GpuSwapchain handle) override;
        GPUTexture GetTexture(GpuSwapchain handle, uint32_t index) override;
        uint32_t GetNextTexture(GpuSwapchain handle) override;

        /* Texture */
        GPUTexture CreateExternalTexture(ID3D12Resource* resource);
        void DestroyTexture(GPUTexture handle) override;


        /* CommandBuffer */
        GpuCommandBuffer* CreateCommandBuffer(QueueType type) override;
        void DestroyCommandBuffer(GpuCommandBuffer* handle) override;

        GraphicsProviderFlags flags;
        GPUPowerPreference powerPreference;
#endif // TODO


        D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        ComPtr<ID3D12Device> d3dDevice = nullptr;
        D3D12MA::Allocator* allocator = nullptr;
        D3D_FEATURE_LEVEL featureLevel{};
        /// Root signature version
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion{ D3D_ROOT_SIGNATURE_VERSION_1_1 };

        static constexpr uint32_t kRenderLatency = 2;
        uint64_t frameCount = 0;
        bool shuttingDown = false;
        bool isLost = false;

        /* Descriptor heaps */
        D3D12DescriptorHeap RTVDescriptorHeap;
        D3D12DescriptorHeap DSVDescriptorHeap;
        
        //Pool<d3d12::Swapchain, kMaxSwapchains> swapchains;
        //Pool<d3d12::Resource, kMaxTextures> textures;
        //Pool<d3d12::Resource, kMaxBuffers> buffers;

        //D3D12CommandQueue graphicsQueue;
        //D3D12CommandQueue computeQueue;
        //D3D12CommandQueue copyQueue;

        //d3d12::Fence frameFence;

        //std::unique_ptr<D3D12GraphicsContext> mainContext;

        struct ResourceRelease
        {
            uint64_t frameIndex;
            IUnknown* handle;
        };

        std::queue<ResourceRelease> deferredReleases;
    };
}
