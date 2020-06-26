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
    class D3D12Texture;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        D3D12GraphicsDevice(WindowHandle window, const Desc& desc);
        ~D3D12GraphicsDevice();

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count, bool shaderVisible);
        void AllocateGPUDescriptors(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE* OutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGPUHandle);

        void HandleDeviceLost();

        IDXGIFactory4* GetDXGIFactory() const { return dxgiFactory; }
        bool IsTearingSupported() const { return tearingSupported; }

        ID3D12Device* GetD3DDevice() const { return d3dDevice; }
        D3D12MA::Allocator* GetMemoryAllocator() const { return memoryAllocator; }
        ID3D12CommandQueue* GetDirectCommandQueue() const { return directCommandQueue.Get(); }

        bool SupportsRenderPass() const { return supportsRenderPass; }

    private:
        void GetAdapter(IDXGIAdapter1** ppAdapter);
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void Shutdown() override;
        void WaitForGPU() override;
        bool BeginFrameImpl() override;
        void EndFrameImpl() override;
        RefPtr<Texture> CreateTexture(const TextureDescription& desc, const void* initialData) override;

        void InitDescriptorHeap(DescriptorHeap* heap, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);
        void CreateUIObjects();
        void CreateFontsTexture();
        void DestroyUIObjects();
        void SetupRenderState(ImDrawData* drawData, ID3D12GraphicsCommandList* commandList);
        void RenderDrawData(ImDrawData* drawData, ID3D12GraphicsCommandList* commandList);
        D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptorsToGPUHeap(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE srcBaseHandle);

        bool supportsRenderPass = false;

        IDXGIFactory4* dxgiFactory = nullptr;
        DWORD dxgiFactoryFlags = 0;
        bool tearingSupported = false;
        D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* memoryAllocator = nullptr;
        /// Current supported feature level.
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        /// Root signature version
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        /* Command queues */
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> directCommandQueue;

        /* Descriptor heaps */
        DescriptorHeap RTVHeap{};
        DescriptorHeap DSVHeap{};
        DescriptorHeap CPUDescriptorHeap;
        DescriptorHeap GPUDescriptorHeaps[2];

        /* Main swap chain */
        DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        uint32_t maxInflightFrames = 2u;
        uint32_t backBufferCount = 2u;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
        ID3D12Resource* swapChainRenderTargets[3] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE swapChainRenderTargetDescriptor[3] = {};

        ID3D12CommandAllocator* commandAllocators[2] = {};
        ID3D12GraphicsCommandList* commandList = nullptr;

        // Presentation fence objects.
        ID3D12Fence* frameFence = nullptr;
        HANDLE frameFenceEvent;
        uint64_t frameCount{ 0 };

        // Imgui objects.
        ID3D12RootSignature* uiRootSignature = nullptr;
        ID3D12PipelineState* uiPipelineState = nullptr;
        RefPtr<D3D12Texture> fontTexture;
        D3D12_CPU_DESCRIPTOR_HANDLE FontSRV;
    };
}
