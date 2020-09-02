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
#include "D3D11Backend.h"
#include <queue>
#include <mutex>

namespace Alimer::Graphics
{
    class D3D11CommandBuffer;
    class D3D11SwapChain;

    class D3D11GraphicsDevice final : public GraphicsDevice
    {
    public:
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        // Functions from dxgi.dll
        using PFN_DXGI_GET_DEBUG_INTERFACE1 = HRESULT(WINAPI*)(UINT Flags, REFIID riid, _COM_Outptr_ void** pDebug);
        using PFN_CREATE_DXGI_FACTORY1 = HRESULT(WINAPI*)(REFIID riid, _COM_Outptr_ void** ppFactory);
        using PFN_CREATE_DXGI_FACTORY2 = HRESULT(WINAPI*)(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);

        PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
        PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1 = nullptr;
        PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;

        // Functions from d3d11.dll
        PFN_D3D11_CREATE_DEVICE D3D11CreateDevice = nullptr;
#endif

        D3D11GraphicsDevice(Window* window, const GraphicsDeviceSettings& settings);
        ~D3D11GraphicsDevice() override;

        void Shutdown();
        void HandleDeviceLost();

        void CommitCommandBuffer(D3D11CommandBuffer* commandBuffer);
        void SubmitCommandBuffer(D3D11CommandBuffer* commandBuffer);
        void SubmitCommandBuffers();

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory.Get(); }
        bool IsTearingSupported() const { return any(dxgiFactoryCaps & DXGIFactoryCaps::Tearing); }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ID3D11Device1* GetD3DDevice() const { return d3dDevice; }
        bool IsLost() const { return isLost; }
        //Texture* GetBackbufferTexture() const override;

    private:
        void CreateFactory();
#if defined(_DEBUG)
        bool IsSdkLayersAvailable() noexcept;
#endif

        void InitCapabilities(IDXGIAdapter1* adapter);
        void WaitForGPU() override;
        uint64 Frame() override;
        //CommandBuffer* RequestCommandBufferCore(const char* name, bool profile) override;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HMODULE dxgiDLL = nullptr;
        HMODULE d3d11DLL = nullptr;
#endif

        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::None;

        ID3D11Device1*  d3dDevice = nullptr;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext1> immediateContext;
        Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> d3dAnnotation;

        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool isLost = false;
        /// Number of frame count
        uint64 frameCount{ 0 };

        D3D11SwapChain* swapChain = nullptr;

        //std::vector<std::unique_ptr<D3D11CommandBuffer>> cmdBuffersPool;
        std::queue<D3D11CommandBuffer*> availableCommandBuffers;
        std::mutex cmdBuffersAllocationMutex;
        std::queue<D3D11CommandBuffer*> commitCommandBuffers;
    };
}
