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

#include "D3D11GPU.h"

namespace alimer
{
    /* D3D11GPUSwapChain */
    D3D11GPUSwapChain::D3D11GPUSwapChain(D3D11GPUDevice* device, WindowHandle windowHandle, bool fullscreen, PixelFormat backbufferFormat, bool enableVSync)
        : device{ device }
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        RECT rect = {};
        GetClientRect(windowHandle, &rect);
        width = static_cast<uint32_t>(rect.right - rect.left);
        height = static_cast<uint32_t>(rect.bottom - rect.top);
#else
#endif

        backbufferFormat = SRGBToLinearFormat(backbufferFormat);
        syncInterval = 1;
        presentFlags = 0;

        if (!enableVSync)
        {
            syncInterval = 0;
            if (any(device->GetDXGIFactoryCaps() & DXGIFactoryCaps::Tearing)) {
                presentFlags = DXGI_PRESENT_ALLOW_TEARING;
            }
        }

        IDXGISwapChain1* tempSwapChain = DXGICreateSwapchain(
            device->GetDXGIFactory(), device->GetDXGIFactoryCaps(),
            device->GetD3DDevice(),
            windowHandle,
            width, height,
            ToDXGISwapChainFormat(backbufferFormat),
            kNumBackBuffers,
            fullscreen
        );

        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle)));
        SafeRelease(tempSwapChain);
        AfterReset();
    }

    D3D11GPUSwapChain::~D3D11GPUSwapChain()
    {

    }

    void D3D11GPUSwapChain::AfterReset()
    {
    }

    void D3D11GPUSwapChain::Resize(uint32_t width, uint32_t height)
    {
        AfterReset();
    }

    void D3D11GPUSwapChain::Present()
    {
        ThrowIfFailed(handle->Present(syncInterval, presentFlags));
    }

    /* D3D11GPUDevice */
    D3D11GPUDevice::D3D11GPUDevice(D3D11GPU* gpu, IDXGIAdapter1* adapter, WindowHandle windowHandle, const GPUDevice::Desc& desc)
        : gpu{ gpu }
    {
        if (windowHandle != nullptr)
        {
            //swapChain = eastl::make_unique<D3D11GPUSwapChain>(windowHandle);
        }

        SafeRelease(adapter);
    }

    D3D11GPUDevice::~D3D11GPUDevice()
    {

    }

    bool D3D11GPUDevice::BeginFrame()
    {
        return true;
    }

    void D3D11GPUDevice::EndFrame()
    {

    }

    DXGIFactoryCaps D3D11GPUDevice::GetDXGIFactoryCaps() const
    {
        return gpu->GetDXGIFactoryCaps();
    }

    IDXGIFactory2* D3D11GPUDevice::GetDXGIFactory() const
    {
        return gpu->GetDXGIFactory();
    }

    /* D3D12GPU */
    bool D3D11GPU::IsAvailable()
    {
        static bool available_initialized = false;
        static bool available = false;
        if (available_initialized) {
            return available;
        }

        available_initialized = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        static HMODULE dxgiLib = LoadLibraryA("dxgi.dll");
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        if (CreateDXGIFactory2 == nullptr) {
            return false;
        }
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");

        static HMODULE d3d11Lib = LoadLibraryA("d3d11.dll");
        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11Lib, "D3D11CreateDevice");
#endif

        available = true;
        return true;
    }

    D3D11GPU::D3D11GPU()
    {
        ALIMER_VERIFY(IsAvailable());

        CreateFactory();
    }

    D3D11GPU::~D3D11GPU()
    {
        // Release factory at last.
        SafeRelease(dxgiFactory);

#ifdef _DEBUG
        {
            IDXGIDebug1* dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug->Release();
            }
        }
#endif
    }

    void D3D11GPU::CreateFactory()
    {
        SafeRelease(dxgiFactory);

#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
        bool debugDXGI = false;
        {
            IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1 != nullptr && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#endif
            {
                debugDXGI = true;

                ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)));

                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(g_DXGI_DEBUG_DXGI, &filter);
                SafeRelease(dxgiInfoQueue);
            }
        }

        if (!debugDXGI)
#endif
        {
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
        }

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            IDXGIFactory5* factory5 = nullptr;
            HRESULT hr = dxgiFactory->QueryInterface(&factory5);
            if (SUCCEEDED(hr))
            {
                hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                dxgiFactoryCaps |= DXGIFactoryCaps::Tearing;
            }

            SafeRelease(factory5);
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        // Disable FLIP if not on a supporting OS
        {
            IDXGIFactory4* factory4 = nullptr;
            if (FAILED(dxgiFactory->QueryInterface(&factory4)))
            {
#ifdef _DEBUG
                OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
            }
            else
            {
                dxgiFactoryCaps |= DXGIFactoryCaps::FlipPresent;
            }

            SafeRelease(factory4);
        }

        // Disable HDR if we are on an OS that can't support FLIP swap effects
        {
            IDXGIFactory5* factory5 = nullptr;
            if (FAILED(dxgiFactory->QueryInterface(&factory5)))
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: HDR swap chains not supported");
#endif
            }
            else
            {
                dxgiFactoryCaps |= DXGIFactoryCaps::HDR;
            }

            SafeRelease(factory5);
        }

#else
        dxgiFactoryCaps |= DXGIFactoryCaps::HDR;
        dxgiFactoryCaps |= DXGIFactoryCaps::FlipPresent;
#endif
    }

    eastl::intrusive_ptr<GPUDevice> D3D11GPU::CreateDevice(WindowHandle windowHandle, const GPUDevice::Desc& desc)
    {
        IDXGIAdapter1* dxgiAdapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6 = nullptr;
        HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (desc.powerPreference == PowerPreference::LowPower) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    gpuPreference,
                    IID_PPV_ARGS(&dxgiAdapter)));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    dxgiAdapter->Release();
                    continue;
                }

                break;
            }
        }
        SafeRelease(dxgiFactory6);
#endif

        if (!dxgiAdapter)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, &dxgiAdapter)); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    dxgiAdapter->Release();
                    continue;
                }

                break;
            }
        }

        if (!dxgiAdapter)
        {
            LOGE("No Direct3D 12 device found");
        }

        return new D3D11GPUDevice(this, dxgiAdapter, windowHandle, desc);
    }

    D3D11GPU* D3D11GPU::Get()
    {
        static D3D11GPU d3d11;
        return &d3d11;
    }
}
