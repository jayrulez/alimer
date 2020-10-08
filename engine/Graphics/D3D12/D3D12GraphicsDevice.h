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
#include <queue>
#include <mutex>

#if !defined(ALIMER_DISABLE_SHADER_COMPILER)
#include <dxcapi.h>
#endif

namespace Alimer
{
    class D3D12Texture;
    class D3D12DescriptorHeap;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();
        D3D12GraphicsDevice(const PresentationParameters& presentationParameters, GraphicsDeviceFlags flags);
        ~D3D12GraphicsDevice();

        bool BeginFrame() override;
        void EndFrame();
        void SetDeviceLost();

        // The CPU will wait for a fence to reach a specified value
        void WaitForFence(uint64_t fenceValue);

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 count);

        IDXGIFactory4* GetDXGIFactory() const noexcept { return dxgiFactory.Get(); }
        bool IsTearingSupported() const noexcept { return isTearingSupported; }
        //DeviceHandle GetHandle() const noexcept { return d3dDevice; }

        void ReleaseResource(IUnknown* resource);
        template<typename T> void ReleaseResource(T*& resource)
        {
            IUnknown* base = resource;
            ReleaseResource(base);
            resource = nullptr;
        }

        GraphicsDeviceCaps caps{};

    private:
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void GetAdapter(bool lowPower, IDXGIAdapter1** ppAdapter);
        void ExecuteDeferredReleases();

        static constexpr uint32_t kRenderLatency = 2u;

        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
        D3D_FEATURE_LEVEL d3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        DWORD dxgiFactoryFlags = 0;
        ComPtr<IDXGIFactory4> dxgiFactory;
        bool isTearingSupported = false;
        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* allocator = nullptr;
        bool isLost = false;
        bool shuttingDown = false;

        D3D12DescriptorHeap* rtvHeap;
        D3D12DescriptorHeap* dsvHeap;
        D3D12DescriptorHeap* cbvSrvUavCpuHeap;

        struct ResourceRelease
        {
            uint64 frameID;
            IUnknown* resource;
        };
        std::queue<ResourceRelease> deferredReleases;
        D3D12Fence* frameFence;
        uint64 frameCount = 0;
        bool deviceLost = false;
    };
}
