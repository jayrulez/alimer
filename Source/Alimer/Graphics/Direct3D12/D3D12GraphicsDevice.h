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

namespace Alimer
{
    class D3D12GraphicsProvider;
    class D3D12CommandQueue;
    class D3D12SwapChain;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        D3D12GraphicsDevice(D3D12GraphicsProvider* provider, const std::shared_ptr<GraphicsAdapter>& adapter);
        ~D3D12GraphicsDevice() override;

        IDXGIFactory4*      GetDXGIFactory() const { return dxgiFactory; }
        bool                IsTearingSupported() const { return isTearingSupported; }
        ID3D12Device*       GetHandle() const { return d3dDevice; }
        //D3D12CommandQueue*  GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;

        template<typename T> void DeferredRelease(T*& resource, bool forceDeferred = false)
        {
            IUnknown* base = resource;
            DeferredRelease_(base, forceDeferred);
            resource = nullptr;
        }

    private:
        void InitCapabilities();
        void Shutdown();
        void ProcessDeferredReleases(uint64_t frameIndex);
        void DeferredRelease_(IUnknown* resource, bool forceDeferred = false);

        void WaitForIdle();
        //void BeginFrame() override;
        //void PresentFrame() override;
        void HandleDeviceLost();

        bool validation;
        IDXGIFactory4* dxgiFactory;
        bool isTearingSupported;

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* memoryAllocator = nullptr;
        /// Current supported feature level.
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        /// Root signature version
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        UINT syncInterval = 1u;
        UINT presentFlags = 0u;
        bool shuttingDown = false;
        bool isLost = false;

        //std::unique_ptr<D3D12CommandQueue> graphicsQueue;
        //std::unique_ptr<D3D12CommandQueue> computeQueue;
        //std::unique_ptr<D3D12CommandQueue> copyQueue;

        /* Frame data and defer release data */
        uint64_t currentCPUFrame = 0;
        uint64_t currentGPUFrame = 0;
        uint64_t currentFrameIndex = 0;
        FenceD3D12 frameFence;
        //std::vector<IUnknown*> deferredReleases[kMaxFrameLatency];
    };
}
