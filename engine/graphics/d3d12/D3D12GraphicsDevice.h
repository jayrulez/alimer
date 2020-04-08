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

#include "graphics/GraphicsDevice.h"
#include "D3D12CommandQueue.h"

namespace alimer
{
    class D3D12GraphicsAdapter;
    class D3D12SwapChain;
    class D3D12GraphicsContext;

    /// Direct3D12 GPU backend.
    class ALIMER_API D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        /// Constructor.
        D3D12GraphicsDevice(GraphicsSurface* surface, const Desc& desc);
        /// Destructor.
        ~D3D12GraphicsDevice() override;

        template<typename T> void DeferredRelease(T*& resource, bool forceDeferred = false)
        {
            IUnknown* base = resource;
            DeferredRelease_(base, forceDeferred);
            resource = nullptr;
        }

        IDXGIFactory4* GetDXGIFactory() const { return dxgiFactory; }
        bool IsTearingSupported() const { return isTearingSupported; }
        ID3D12Device* GetD3DDevice() const { return d3dDevice; }
        D3D12MA::Allocator* GetMemoryAllocator() const { return allocator; }

        D3D12CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT)
        {
            switch (type)
            {
            case D3D12_COMMAND_LIST_TYPE_COMPUTE: return computeQueue;
            case D3D12_COMMAND_LIST_TYPE_COPY: return copyQueue;
            default: return graphicsQueue;
            }
        }

        ID3D12CommandQueue* GetD3D12GraphicsQueue(void) { return graphicsQueue.GetHandle(); }

    private:
        void Shutdown();
        void ProcessDeferredReleases(uint64_t frameIndex);
        void DeferredRelease_(IUnknown* resource, bool forceDeferred = false);

        void CreateDeviceResources();
        void InitCapabilities(IDXGIAdapter1* adapter);

        void WaitForIdle() override;
        bool BeginFrame() override;
        void PresentFrame() override;
        GraphicsContext* GetMainContext() const override;

        //GPUBuffer* CreateBufferCore(const BufferDescriptor* descriptor, const void* initialData) override;

        UINT dxgiFactoryFlags = 0;
        IDXGIFactory4* dxgiFactory = nullptr;
        bool isTearingSupported = false;

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* allocator = nullptr;
        D3D_FEATURE_LEVEL featureLevel;

        D3D12CommandQueue graphicsQueue;
        D3D12CommandQueue computeQueue;
        D3D12CommandQueue copyQueue;

        std::unique_ptr<D3D12SwapChain> swapChain;
        uint32_t renderLatency = 2u;
        D3D12GPUFence frameFence;
        uint64_t currentCPUFrame = 0;
        uint64_t currentGPUFrame = 0;
        uint64_t currentFrameIndex = 0;
        bool shuttingDown = false;

        /* Descriptor heaps */
        D3D12DescriptorHeap RTVDescriptorHeap;
        D3D12DescriptorHeap DSVDescriptorHeap;

        std::unique_ptr<D3D12GraphicsContext> mainContext;

        std::vector<IUnknown*> deferredReleases[kMaxFrameLatency];
    };
}
