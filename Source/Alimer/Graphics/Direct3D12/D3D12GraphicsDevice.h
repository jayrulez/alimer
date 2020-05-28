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

#include "graphics/GraphicsDevice.h"
#include "D3D12Backend.h"

namespace alimer
{
    class D3D12GraphicsProvider;
    class D3D12CommandQueue;
    class D3D12SwapChain;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        D3D12GraphicsDevice(FeatureLevel minFeatureLevel, bool enableDebugLayer);
        ~D3D12GraphicsDevice() override;

        IDXGIFactory4*      GetDXGIFactory() const { return dxgiFactory; }
        bool                IsTearingSupported() const { return isTearingSupported; }
        ID3D12Device*       GetHandle() const { return d3dDevice; }
        D3D12MA::Allocator* GetMemoryAllocator() const { return memoryAllocator; }

        D3D12CommandQueue*  GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;
        ID3D12CommandQueue* GetD3DCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;

    private:
        void GetAdapter(D3D_FEATURE_LEVEL d3dMinFeatureLevel, IDXGIAdapter1** ppAdapter);
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void Shutdown();

        void WaitForIdle();
        void HandleDeviceLost();
        bool BeginFrame() override;
        u64 EndFrame() override;

        RefPtr<Texture> CreateTexture(const TextureDescriptor* descriptor, const void* initialData) override;
        RefPtr<SwapChain> CreateSwapChain(void* windowHandle, const SwapChainDescriptor* descriptor) override;

        static uint32_t deviceCount;
        UINT dxgiFactoryFlags = 0;
        IDXGIFactory4* dxgiFactory = nullptr;
        bool isTearingSupported = false;

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* memoryAllocator = nullptr;
        /// Current supported feature level.
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        /// Root signature version
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        D3D12CommandQueue* graphicsCommandQueue;
        D3D12CommandQueue* computeCommandQueue;
        D3D12CommandQueue* copyCommandQueue;

        struct DescriptorHeap
        {
            ID3D12DescriptorHeap* Heap;
            D3D12_CPU_DESCRIPTOR_HANDLE CPUStart;
            D3D12_GPU_DESCRIPTOR_HANDLE GPUStart;
            uint32_t Size;
            uint32_t Capacity;
        };

        DescriptorHeap RTVHeap;
        DescriptorHeap DSVHeap;

        bool isLost = false;

        ID3D12Fence* frameFence;
        HANDLE frameFenceEvent;
        u64 frameCount = 0;
    };
}
