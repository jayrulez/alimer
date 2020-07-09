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

#if defined(VGPU_DRIVER_D3D11)

#include "vgpu_d3d_common.h"
#define D3D11_NO_HELPERS
#include <d3d11_1.h>

namespace vgpu
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    struct BufferD3D11 {
        enum { MAX_COUNT = 4096 };

        ID3D11Buffer* handle;
    };

    struct TextureD3D11 {
        enum { MAX_COUNT = 4096 };

        union {
            ID3D11Resource* handle;
            ID3D11Texture2D* tex2D;
            ID3D11Texture3D* tex3D;
        };
    };

    struct ViewportD3D11 {
        IDXGISwapChain1* swapchain;
        uint32_t width;
        uint32_t height;
        PixelFormat colorFormat;
        PixelFormat depthStencilFormat;
        ID3D11RenderTargetView* rtv;
        ID3D11DepthStencilView* dsv;
    };

    /* Global data */
    static struct {
        bool available_initialized;
        bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HMODULE dxgiDLL;
        HMODULE d3d11DLL;
#endif
        bool debug;
        Caps caps;

        Pool<BufferD3D11, BufferD3D11::MAX_COUNT> buffers;
        Pool<TextureD3D11, TextureD3D11::MAX_COUNT> textures;

        IDXGIFactory2* factory;
        DXGIFactoryCaps factoryCaps;

        ID3D11Device1* device;
        D3D_FEATURE_LEVEL featureLevel;
        ID3D11DeviceContext1* contexts[kMaxCommandLists] = {};
        ID3DUserDefinedAnnotation* annotations[kMaxCommandLists] = {};

        uint32_t backbufferCount = 2;
        ViewportD3D11*              mainViewport;   // Guaranteed to be == Viewports[0]
        std::vector<ViewportD3D11*> viewports;      // Main viewports, followed by all secondary viewports.
    } d3d11;

    // Check for SDK Layer support.
    static bool d3d11_SdkLayersAvailable() noexcept
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

    static void d3d11_CreateFactory()
    {
        SAFE_RELEASE(d3d11.factory);

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

                VHR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&d3d11.factory)));

                dxgiInfoQueue->SetBreakOnSeverity(vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(vgpu_DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }

        if (!debugDXGI)
#endif
        {
            VHR(CreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory)));
        }

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            IDXGIFactory5* factory5 = nullptr;
            HRESULT hr = d3d11.factory->QueryInterface(&factory5);
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
                d3d11.factoryCaps |= DXGIFactoryCaps::Tearing;
            }

            SAFE_RELEASE(factory5);
        }

        // Disable HDR if we are on an OS that can't support FLIP swap effects
        {
            IDXGIFactory5* factory5 = nullptr;
            if (FAILED(d3d11.factory->QueryInterface(&factory5)))
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: HDR swap chains not supported");
#endif
            }
            else
            {
                d3d11.factoryCaps |= DXGIFactoryCaps::HDR;
            }

            SAFE_RELEASE(factory5);
        }

        // Disable FLIP if not on a supporting OS
        {
            IDXGIFactory4* factory4 = nullptr;
            if (FAILED(d3d11.factory->QueryInterface(&factory4)))
            {
#ifdef _DEBUG
                OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
            }
            else
            {
                d3d11.factoryCaps |= DXGIFactoryCaps::FlipPresent;
            }

            SAFE_RELEASE(factory4);
        }
    }

    static IDXGIAdapter1* d3d11_GetHardwareAdapter(bool lowPower)
    {
        IDXGIAdapter1* adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* factory6 = nullptr;
        HRESULT hr = d3d11.factory->QueryInterface(&factory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = lowPower ? DXGI_GPU_PREFERENCE_MINIMUM_POWER : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;

            for (UINT i = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(&adapter))); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", i, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif

                break;
            }
        }
        SAFE_RELEASE(factory6);
#endif

        if (!adapter)
        {
            for (UINT i = 0; SUCCEEDED(d3d11.factory->EnumAdapters1(i, &adapter)); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();
                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", i, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif

                break;
            }
        }

        return adapter;
    }

    static void d3d11_updateViewport(ViewportD3D11* viewport)
    {
        ID3D11Texture2D* resource;
        VHR(viewport->swapchain->GetBuffer(0, IID_PPV_ARGS(&resource)));
        //_backbufferTextures.Push(new D3D11Texture(_device, resource));

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = ToDXGIFormat(viewport->colorFormat);
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        VHR(d3d11.device->CreateRenderTargetView(
            resource,
            &rtvDesc,
            &viewport->rtv
        ));

        resource->Release();
    }

    static void d3d11_DestroyViewport(ViewportD3D11* viewport)
    {
        viewport->rtv->Release();
        viewport->swapchain->Release();
        delete viewport;
    }

    static bool d3d11_init(InitFlags flags, const PresentationParameters& presentationParameters)
    {
        if (any(flags & InitFlags::DebugRutime) || any(flags & InitFlags::GPUBasedValidation))
        {
            d3d11.debug = true;
        }

        d3d11.buffers.init();
        d3d11.textures.init();

        // Create factory first.
        d3d11_CreateFactory();

        // Get adapter and create device.
        {
            IDXGIAdapter1* adapter = d3d11_GetHardwareAdapter(any(flags & InitFlags::GPUPreferenceLowPower));

            // Create the Direct3D 11 API device object and a corresponding context.
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

            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

            if (d3d11.debug)
            {
                if (d3d11_SdkLayersAvailable())
                {
                    // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
                }
                else
                {
                    OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
                }
            }

            ID3D11Device* device;
            ID3D11DeviceContext* context;

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
                    &d3d11.featureLevel,
                    &context
                );
            }
#if defined(NDEBUG)
            else
            {
                vgpu::logError("No Direct3D hardware device found");
                return false;
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
                    &d3d11.featureLevel,
                    &context
                );

                if (SUCCEEDED(hr))
                {
                    OutputDebugStringA("Direct3D Adapter - WARP\n");
                }
            }
#endif

            VHR(hr);

#ifndef NDEBUG
            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(device->QueryInterface(&d3dDebug)))
            {
                ID3D11InfoQueue* d3dInfoQueue;
                if (SUCCEEDED(d3dDebug->QueryInterface(&d3dInfoQueue)))
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
                    d3dInfoQueue->Release();
                }
                d3dDebug->Release();
            }
#endif

            VHR(device->QueryInterface(&d3d11.device));
            VHR(context->QueryInterface(&d3d11.contexts[0]));
            VHR(context->QueryInterface(&d3d11.annotations[0]));

            SAFE_RELEASE(context);
            SAFE_RELEASE(device);
            SAFE_RELEASE(adapter);
        }

        // Create swap chain if not running in headless
        if (presentationParameters.windowHandle != nullptr)
        {
            d3d11.mainViewport = new ViewportD3D11();
            d3d11.mainViewport->colorFormat = presentationParameters.backbufferFormat;
            d3d11.mainViewport->depthStencilFormat = presentationParameters.depthStencilFormat;

            d3d11.mainViewport->swapchain = d3dCreateSwapchain(
                d3d11.factory,
                d3d11.factoryCaps,
                d3d11.device,
                presentationParameters.windowHandle,
                presentationParameters.backbufferWidth,
                presentationParameters.backbufferHeight,
                presentationParameters.backbufferFormat,
                d3d11.backbufferCount);

            // Get dimension from SwapChain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
            d3d11.mainViewport->swapchain->GetDesc1(&swapChainDesc);
            d3d11.mainViewport->width = swapChainDesc.Width;
            d3d11.mainViewport->height = swapChainDesc.Height;
            d3d11_updateViewport(d3d11.mainViewport);
            d3d11.viewports.push_back(d3d11.mainViewport);
        }

        return true;
    }

    static void d3d11_shutdown(void)
    {
        for (size_t i = 0; i < d3d11.viewports.size(); i++)
        {
            d3d11_DestroyViewport(d3d11.viewports[i]);
        }

        d3d11.viewports.clear();

        for (uint32_t i = 0; i < _countof(d3d11.contexts); i++)
        {
            SAFE_RELEASE(d3d11.annotations[i]);
            SAFE_RELEASE(d3d11.contexts[i]);
        }

        ULONG refCount = d3d11.device->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            vgpu::logError("Direct3D11: There are %d unreleased references left on the device", refCount);

            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&d3dDebug))))
            {
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
                d3dDebug->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        SAFE_RELEASE(d3d11.factory);

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (DXGIGetDebugInterface1 && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#else
        if (SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#endif
        {
            dxgiDebug1->ReportLiveObjects(vgpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug1->Release();
        }
#endif

        memset(&d3d11, 0, sizeof(d3d11));
    }

    static void d3d11_beginFrame(void)
    {
        if (d3d11.mainViewport)
        {
            d3d11.contexts[0]->OMSetRenderTargets(1, &d3d11.mainViewport->rtv, d3d11.mainViewport->dsv);
            cmdSetViewport(0, 0.0f, 0.0f, static_cast<float>(d3d11.mainViewport->width), static_cast<float>(d3d11.mainViewport->height));
            cmdSetScissor(0, 0, 0, d3d11.mainViewport->width, d3d11.mainViewport->height);
        }
    }

    static void d3d11_endFrame(void)
    {
        HRESULT hr = S_OK;
        for (uint32_t i = 1; i < d3d11.viewports.size(); i++) {

        }

        if (d3d11.mainViewport)
        {
            hr = d3d11.mainViewport->swapchain->Present(1, 0);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // TODO: Handle device lost.
            }
        }

        VHR(hr);

        if (!d3d11.factory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            d3d11_CreateFactory();
        }
    }

    static const Caps* d3d11_queryCaps(void) {
        return &d3d11.caps;
    }

    /* Commands */
    static void d3d11_cmdSetViewport(CommandList commandList, float x, float y, float width, float height, float min_depth, float max_depth)
    {
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = x;
        viewport.TopLeftY = y;
        viewport.Width = width;
        viewport.Height = height;
        viewport.MinDepth = min_depth;
        viewport.MaxDepth = max_depth;

        d3d11.contexts[commandList]->RSSetViewports(1, &viewport);
    }

    static void d3d11_cmdSetScissor(CommandList commandList, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        D3D11_RECT scissorRect = {};
        scissorRect.left = (LONG)x;
        scissorRect.top = (LONG)y;
        scissorRect.right = LONG(x + width);
        scissorRect.bottom = LONG(y + height);
        d3d11.contexts[commandList]->RSSetScissorRects(1, &scissorRect);
    }

    static bool d3d11_isSupported(void) {
        if (d3d11.available_initialized) {
            return d3d11.available;
        }

        d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        d3d11.dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!d3d11.dxgiDLL) {
            return false;
        }

        CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory1");
        if (CreateDXGIFactory1 == nullptr)
            return false;

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgiDLL, "DXGIGetDebugInterface1");

        d3d11.d3d11DLL = LoadLibraryA("d3d11.dll");
        if (!d3d11.d3d11DLL) {
            return false;
        }

        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.d3d11DLL, "D3D11CreateDevice");
        if (D3D11CreateDevice == nullptr) {
            return false;
        }

#endif

        // Determine DirectX hardware feature levels this app will support.
        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            s_featureLevels,
            _countof(s_featureLevels),
            D3D11_SDK_VERSION,
            nullptr,
            nullptr,
            nullptr
        );

        if (FAILED(hr))
        {
            return false;
        }

        d3d11.available = true;
        return true;
    };

    static Renderer* d3d11_initRenderer(void) {
        static Renderer renderer = { nullptr };
        renderer.init = d3d11_init;
        renderer.shutdown = d3d11_shutdown;
        renderer.beginFrame = d3d11_beginFrame;
        renderer.endFrame = d3d11_endFrame;
        renderer.queryCaps = d3d11_queryCaps;

        //renderer.create_texture = vulkan_texture_create;
        //renderer.texture_destroy = vulkan_texture_destroy;

        /* Commands */
        renderer.cmdSetViewport = d3d11_cmdSetViewport;
        renderer.cmdSetScissor = d3d11_cmdSetScissor;

        return &renderer;
    }

    Driver d3d11_driver = {
        BackendType::Direct3D11,
        d3d11_isSupported,
        d3d11_initRenderer
    };
}

#endif /* defined(VGPU_DRIVER_D3D12) */
