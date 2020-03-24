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
#include "D3D12Backend.h"

namespace alimer
{
    class D3D12CommandQueue;

    /// Direct3D12 GPU backend.
    class ALIMER_API D3D12GPUDevice final : public GPUDevice
    {
    public:
        static bool IsAvailable();

        /// Constructor.
        D3D12GPUDevice(bool validation);
        /// Destructor.
        ~D3D12GPUDevice() override;

        void Destroy();
        void WaitIdle() override;
        void CommitFrame() override;

        IDXGIFactory4*          GetDXGIFactory() const { return dxgiFactory; }
        bool                    IsTearingSupported() const { return isTearingSupported; }
        ID3D12Device*           GetD3DDevice() const { return d3dDevice; }
        D3D_FEATURE_LEVEL       GetDeviceFeatureLevel() const { return d3dFeatureLevel; }
        D3D12MA::Allocator*     GetMemoryAllocator() const { return allocator; }

        D3D12CommandQueue* GetGraphicsQueue(void) { return graphicsQueue; }
        D3D12CommandQueue* GetComputeQueue(void) { return computeQueue; }
        D3D12CommandQueue* GetCopyQueue(void) { return copyQueue; }

        D3D12CommandQueue* GetQueue(CommandQueueType queueType = CommandQueueType::Graphics) const;
        ID3D12CommandQueue* GetD3DCommandQueue(CommandQueueType queueType = CommandQueueType::Graphics) const;

    private:
        static constexpr D3D_FEATURE_LEVEL d3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        void CreateDeviceResources();
        bool GetAdapter(IDXGIAdapter1** ppAdapter);
        void InitCapabilities(IDXGIAdapter1* adapter);

        SharedPtr<SwapChain> CreateSwapChain(const SwapChainDescriptor* descriptor) override;
        SharedPtr<Texture> CreateTexture() override;
        GPUBuffer* CreateBufferCore(const BufferDescriptor* descriptor, const void* initialData) override;

        bool validation;
        UINT dxgiFactoryFlags = 0;
        SharedPtr<IDXGIFactory4> dxgiFactory;
        bool isTearingSupported = false;
        ID3D12Device* d3dDevice = nullptr;
        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        D3D12MA::Allocator* allocator = nullptr;

        D3D12CommandQueue* graphicsQueue;
        D3D12CommandQueue* computeQueue;
        D3D12CommandQueue* copyQueue;

        ID3D12Fence* frameFence = nullptr;
        HANDLE frameFenceEvent = nullptr;
        uint64_t numFrames=0;
    };
}
