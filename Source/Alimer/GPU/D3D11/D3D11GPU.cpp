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
    /* D3D11GPUTexture */
    D3D11GPUTexture::D3D11GPUTexture(D3D11GPUDevice* device_, const GPUTexture::Desc& desc, ID3D11Texture2D* externalHandle)
        : device(device_)
        , GPUTexture(desc)
    {
        if (externalHandle != nullptr)
        {
            handle = externalHandle;
        }
        else
        {
        }
    }

    D3D11GPUTexture::~D3D11GPUTexture()
    {
        SafeRelease(handle);
    }

    /* D3D11GPUSwapChain */
    D3D11GPUSwapChain::D3D11GPUSwapChain(D3D11GPUDevice* device, WindowHandle windowHandle, bool isFullscreen, PixelFormat colorFormat_, bool enableVSync)
        : device{ device }
        , colorFormat(colorFormat_)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        RECT rect = {};
        GetClientRect(windowHandle, &rect);
        width = static_cast<uint32_t>(rect.right - rect.left);
        height = static_cast<uint32_t>(rect.bottom - rect.top);
#else
#endif

        IDXGISwapChain1* swapChain = DXGICreateSwapchain(
            device->GetDXGIFactory(), device->GetDXGIFactoryCaps(),
            device->GetD3DDevice(),
            windowHandle,
            width, height,
            ToDXGISwapChainFormat(SRGBToLinearFormat(colorFormat)),
            kNumBackBuffers,
            isFullscreen
        );
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        handle = swapChain;
#else
        ThrowIfFailed(swapChain->QueryInterface(IID_PPV_ARGS(&handle)));
        SafeRelease(swapChain);
#endif

        device->viewports.push_back(this);
        AfterReset();
    }

    D3D11GPUSwapChain::~D3D11GPUSwapChain()
    {
        SafeRelease(handle);
        device->viewports.erase_first(this);
    }

    void D3D11GPUSwapChain::AfterReset()
    {
        backbufferTexture.reset();

        ID3D11Texture2D* resource;
        ThrowIfFailed(handle->GetBuffer(0, IID_PPV_ARGS(&resource)));

        GPUTexture::Desc textureDesc = {};
        textureDesc.type = TextureType::Texture2D;
        textureDesc.format = colorFormat;
        textureDesc.usage = TextureUsage::RenderTarget;
        textureDesc.width = width;
        textureDesc.height = height;
        backbufferTexture = new D3D11GPUTexture(device, textureDesc, resource);
    }

    void D3D11GPUSwapChain::Resize(uint32_t width, uint32_t height)
    {
        AfterReset();
    }

    /* D3D11GPUContext */
    D3D11GPUContext::D3D11GPUContext(D3D11GPUDevice* device_, ID3D11DeviceContext* context_)
        : device(device_)
    {
        ThrowIfFailed(context_->QueryInterface(IID_PPV_ARGS(&context)));
        ThrowIfFailed(context_->QueryInterface(IID_PPV_ARGS(&annotation)));
        context_->Release();
    }

    D3D11GPUContext::~D3D11GPUContext()
    {
        SafeRelease(annotation);
        SafeRelease(context);
    }

    /* D3D11GPUDevice */
    inline bool SdkLayersAvailable() noexcept
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
            nullptr,
            D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
            nullptr,                    // Any feature level will do.
            0,
            D3D11_SDK_VERSION,
            nullptr,                    // No need to keep the D3D device reference.
            nullptr,                    // No need to know the feature level.
            nullptr                     // No need to keep the D3D device context reference.
        );

        return SUCCEEDED(hr);
    }

    D3D11GPUDevice::D3D11GPUDevice(D3D11GPU* gpu, IDXGIAdapter1* adapter, WindowHandle windowHandle, const GPUDevice::Desc& desc)
        : gpu{ gpu }
    {
        if (any(gpu->GetDXGIFactoryCaps() & DXGIFactoryCaps::Tearing))
        {
            presentFlagsNoVSync = DXGI_PRESENT_ALLOW_TEARING;
        }

        // Create device and main context.
        {
            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

            if (any(desc.flags & GPUDeviceFlags::DebugRuntime)
                || any(desc.flags & GPUDeviceFlags::GPUBaseValidation))
            {
                if (SdkLayersAvailable())
                {
                    // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
                }
                else
                {
                    OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
                }
            }

            // Determine DirectX hardware feature levels this app will support.
            static const D3D_FEATURE_LEVEL s_featureLevels[] =
            {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0,
                D3D_FEATURE_LEVEL_9_3,
                D3D_FEATURE_LEVEL_9_2,
                D3D_FEATURE_LEVEL_9_1,
            };

            // Create the Direct3D 11 API device object and a corresponding context.
            ID3D11Device* device;
            ID3D11DeviceContext* immediateContext;

            HRESULT hr = E_FAIL;
            if (adapter)
            {
                hr = D3D11CreateDevice(
                    adapter,
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &device,
                    &d3dFeatureLevel,
                    &immediateContext
                );
            }
#if defined(NDEBUG)
            else
            {
                throw std::exception("No Direct3D hardware device found");
            }
#else
            if (FAILED(hr))
            {
                // If the initialization fails, fall back to the WARP device.
                // For more information on WARP, see:
                // http://go.microsoft.com/fwlink/?LinkId=286690
                hr = D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                    nullptr,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &device,
                    &d3dFeatureLevel,
                    &immediateContext
                );

                if (SUCCEEDED(hr))
                {
                    OutputDebugStringA("Direct3D Adapter - WARP\n");
                }
            }
#endif

            ThrowIfFailed(hr);

            if (any(desc.flags & GPUDeviceFlags::DebugRuntime)
                || any(desc.flags & GPUDeviceFlags::GPUBaseValidation))
            {
                ID3D11Debug* d3dDebug = nullptr;
                if (SUCCEEDED(device->QueryInterface(_uuidof(ID3D11Debug), (void**)&d3dDebug)))
                {
                    ID3D11InfoQueue* d3dInfoQueue = nullptr;
                    if (SUCCEEDED(d3dDebug->QueryInterface(_uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
                    {
#ifdef _DEBUG
                        d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                        d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
                        D3D11_MESSAGE_ID hide[] =
                        {
                            D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                        };
                        D3D11_INFO_QUEUE_FILTER filter = {};
                        filter.DenyList.NumIDs = _countof(hide);
                        filter.DenyList.pIDList = hide;
                        d3dInfoQueue->AddStorageFilterEntries(&filter);
                    }
                    SafeRelease(d3dInfoQueue);
                }

                SafeRelease(d3dDebug);
            }

            ThrowIfFailed(device->QueryInterface(&d3dDevice));
            mainContext = new D3D11GPUContext(this, immediateContext);
            SafeRelease(device);
        }

        // Init capabilities
        {
            DXGI_ADAPTER_DESC1 adapterDesc;
            ThrowIfFailed(adapter->GetDesc1(&adapterDesc));

            eastl::wstring deviceName(adapterDesc.Description);
            caps.backendType = RendererType::Direct3D11;
            caps.adapterName = ToUtf8(deviceName);
            caps.deviceId = adapterDesc.DeviceId;
            caps.vendorId = adapterDesc.VendorId;

            // Detect adapter type.
            if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                caps.adapterType = GPUAdapterType::CPU;
            }
            else
            {
                caps.adapterType = GPUAdapterType::DiscreteGPU;
            }
        }

        if (windowHandle != nullptr)
        {
            mainViewport = eastl::make_unique<D3D11GPUSwapChain>(this, windowHandle, desc.isFullscreen, desc.colorFormat, desc.enableVSync);
        }

        SafeRelease(adapter);
    }

    D3D11GPUDevice::~D3D11GPUDevice()
    {
        mainViewport.reset();
        mainContext.reset();

        ULONG refCount = d3dDevice->Release();
#ifdef _DEBUG
        if (refCount > 0)
        {
            //LOG("Direct3D11: There are {} unreleased references left on the device", refCount);

            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(d3dDevice->QueryInterface(&d3dDebug)))
            {
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY);
                d3dDebug->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif
    }

    bool D3D11GPUDevice::BeginFrame()
    {
        return true;
    }

    void D3D11GPUDevice::EndFrame()
    {
        HRESULT hr = S_OK;

        for (uint32_t i = 1; i < viewports.size(); i++)
        {
            hr = viewports[i]->handle->Present(0, presentFlagsNoVSync);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // TODO: Handle device lost.
                isLost = true;
                break;
            }
        }

        // Main viewport is presented with vertical sync on
        hr = mainViewport->handle->Present(1, 0);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // TODO: Handle device lost.
            isLost = true;
        }

        // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
        if (!gpu->GetDXGIFactory()->IsCurrent())
        {
            gpu->CreateFactory();
        }
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
        CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(dxgiLib, "CreateDXGIFactory1");
        if (CreateDXGIFactory1 == nullptr) {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
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

    GPUDevice* D3D11GPU::CreateDevice(WindowHandle windowHandle, const GPUDevice::Desc& desc)
    {
        IDXGIAdapter1* dxgiAdapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6 = nullptr;
        HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (any(desc.flags & GPUDeviceFlags::LowPowerPreference))
            {
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
