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
#include <vector>
#include <queue>
#include <mutex>

namespace alimer
{
    class D3D12GraphicsProvider;
    class D3D12CommandQueue;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        D3D12GraphicsDevice(bool validation);
        ~D3D12GraphicsDevice() override;

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
        void WaitForFenceValue(uint64_t fenceValue);

        IDXGIFactory4*      GetDXGIFactory() const { return dxgiFactory; }
        bool                IsTearingSupported() const { return isTearingSupported; }
        ID3D12Device*       GetHandle() const { return d3dDevice; }
        D3D12MA::Allocator* GetMemoryAllocator() const { return memoryAllocator; }

        D3D12CommandQueue*  GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;
        ID3D12CommandQueue* GetD3DCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;
        bool SupportsRenderPass() const { return supportsRenderPass; }

    private:
        void GetAdapter(D3D_FEATURE_LEVEL d3dMinFeatureLevel, IDXGIAdapter1** ppAdapter);
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void Shutdown();

        void WaitForIdle();
        void HandleDeviceLost();

        GraphicsContext* CreateContext(const GraphicsContextDescription& desc) override;
        Texture* CreateTexture(const TextureDescription& desc, const void* initialData) override;

        static uint32_t deviceCount;

        D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        UINT dxgiFactoryFlags = 0;
        IDXGIFactory4* dxgiFactory = nullptr;
        bool isTearingSupported = false;
        bool supportsRenderPass = false;

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

        DescriptorHeap RTVHeap{};
        DescriptorHeap DSVHeap{};
    };

    class D3D12GraphicsDeviceFactory final : public GraphicsDeviceFactory
    {
    public:
        BackendType GetBackendType() const override { return BackendType::Direct3D12; }
        GraphicsDevice* CreateDevice(bool validation) override;
    };
}
