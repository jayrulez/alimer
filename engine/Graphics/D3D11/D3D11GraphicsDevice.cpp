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

#if TODO
#include "D3D11GraphicsDevice.h"
#include "Graphics/Texture.h"
#include "core/String.h"

namespace alimer
{
    namespace
    {
#if defined(_DEBUG)
        // Check for SDK Layer support.
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
#endif
        inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt) noexcept
        {
            switch (fmt)
            {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
            default:                                return fmt;
            }
        }

        inline void GetHardwareAdapter(IDXGIFactory2* factory, IDXGIAdapter1** ppAdapter, DXGI_GPU_PREFERENCE gpuPreference)
        {
            *ppAdapter = nullptr;

            RefPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
            if (gpuPreference != DXGI_GPU_PREFERENCE_UNSPECIFIED)
            {
                IDXGIFactory6* dxgiFactory6 = nullptr;
                HRESULT hr = factory->QueryInterface(&dxgiFactory6);
                if (SUCCEEDED(hr))
                {
                    const bool lowPower = false;
                    DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
                    if (lowPower) {
                        gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
                    }

                    for (UINT adapterIndex = 0;
                        SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
                            adapterIndex,
                            gpuPreference,
                            IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
                        adapterIndex++)
                    {
                        DXGI_ADAPTER_DESC1 desc;
                        ThrowIfFailed(adapter->GetDesc1(&desc));

                        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                        {
                            // Don't select the Basic Render Driver adapter.
                            continue;
                        }

                        break;
                    }
                }
                SAFE_RELEASE(dxgiFactory6);
            }
#endif

            if (!adapter)
            {
                for (UINT adapterIndex = 0; SUCCEEDED(factory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf())); ++adapterIndex)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    ThrowIfFailed(adapter->GetDesc1(&desc));

                    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    {
                        // Don't select the Basic Render Driver adapter.
                        continue;
                    }

                    break;
                }
            }

            *ppAdapter = adapter.Detach();
        }
    }


    bool GraphicsDevice_D3D11::IsAvailable()
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
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");
        if (CreateDXGIFactory2 == nullptr) {
            return false;
        }

        static HMODULE d3d11Lib = LoadLibraryA("d3d11.dll");
        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11Lib, "D3D11CreateDevice");
        if (D3D11CreateDevice == nullptr) {
            return false;
        }
#endif

        available = true;
        return true;
    }

    GraphicsDevice_D3D11::GraphicsDevice_D3D11(const Desc& desc)
        : GraphicsDevice(desc)
        , d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1)
    {
        CreateDeviceResources();
    }

    GraphicsDevice_D3D11::~GraphicsDevice_D3D11()
    {
        Shutdown();
    }

    void GraphicsDevice_D3D11::CreateDeviceResources()
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (SdkLayersAvailable())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }
#endif

        CreateFactory();

        RefPtr<IDXGIAdapter1> adapter;
        GetHardwareAdapter(dxgiFactory, adapter.GetAddressOf(), DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE);

        {
            // Determine DirectX hardware feature levels this app will support.
            static const D3D_FEATURE_LEVEL s_featureLevels[] =
            {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0
            };

            // Create the Direct3D 11 API device object and a corresponding context.
            ID3D11Device* device;
            ID3D11DeviceContext* context;

            HRESULT hr = E_FAIL;
            if (adapter)
            {
                hr = D3D11CreateDevice(
                    adapter.Get(),
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &device,
                    &d3dFeatureLevel,
                    &context
                );
            }
#if defined(NDEBUG)
            else
            {
                LOG_ERROR("No Direct3D hardware device found");
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
                    &context
                );

                if (SUCCEEDED(hr))
                {
                    OutputDebugStringA("Direct3D Adapter - WARP\n");
                }
            }
#endif

            ThrowIfFailed(hr);

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

            ThrowIfFailed(device->QueryInterface(&d3dDevice));
            ThrowIfFailed(context->QueryInterface(&deviceContexts[0]));
            ThrowIfFailed(context->QueryInterface(&userDefinedAnnotations[0]));
            SAFE_RELEASE(context);
            SAFE_RELEASE(device);
        }

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
        // ensures that the application will only render after each VSync, minimizing power consumption.
        IDXGIDevice3* dxgiDevice;
        ThrowIfFailed(d3dDevice->QueryInterface(&dxgiDevice));
        ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));
#endif

        InitCapabilities(adapter.Get());
    }

    void GraphicsDevice_D3D11::CreateWindowSizeDependentResources()
    {

#if TODO
        if (swapChain)
        {
            // If the swap chain already exists, resize it.
            UINT swapChainFlags = 0u;

            if (!syncInterval && tearingSupported) {
                swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

            HRESULT hr = swapChain->ResizeBuffers(
                backBufferCount,
                backBufferWidth,
                backBufferHeight,
                noSRGBFormat,
                swapChainFlags
            );

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
#ifdef _DEBUG
                char buff[64] = {};
                sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
                    static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
                OutputDebugStringA(buff);
#endif
                // If the device was removed for any reason, a new device and swap chain will need to be created.
                HandleDeviceLost();

                // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
                // and correctly set up the new device.
                return;
            }
            else
            {
                ThrowIfFailed(hr);
            }
        }
        else
        {
    }
#endif // TODO


}

    void GraphicsDevice_D3D11::AfterReset(SwapChainHandle handle)
    {
        SwapChainD3D11& swapChain = swapChains[handle.id];



        bool isDisplayHDR10 = false;

#if defined(NTDDI_WIN10_RS2)
        if (swapChain.handle)
        {
            RefPtr<IDXGIOutput> output;
            if (SUCCEEDED(swapChain.handle->GetContainingOutput(output.GetAddressOf())))
            {
                RefPtr<IDXGIOutput6> output6;
                if (SUCCEEDED(output->QueryInterface(IID_PPV_ARGS(output6.GetAddressOf()))))
                {
                    DXGI_OUTPUT_DESC1 desc;
                    ThrowIfFailed(output6->GetDesc1(&desc));

                    if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                    {
                        // Display output is HDR10.
                        isDisplayHDR10 = true;
                    }
                }
            }
        }
#endif

        DXGI_COLOR_SPACE_TYPE newColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
        if (isDisplayHDR10)
        {
            switch (swapChain.colorFormat)
            {
            case PixelFormat::RGB10A2UNorm:
                // The application creates the HDR10 signal.
                newColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                break;

            case PixelFormat::RGBA16Float:
                // The system creates the HDR10 signal; application uses linear values.
                newColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
                break;

            default:
                break;
            }
        }

        swapChain.colorSpace = newColorSpace;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        RefPtr<IDXGISwapChain3> swapChain3;
        if (SUCCEEDED(swapChain.handle->QueryInterface(swapChain3.GetAddressOf())))
        {
            UINT colorSpaceSupport = 0;
            if (SUCCEEDED(swapChain3->CheckColorSpaceSupport(newColorSpace, &colorSpaceSupport))
                && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
            {
                ThrowIfFailed(swapChain3->SetColorSpace1(newColorSpace));
            }
        }
#else
        UINT colorSpaceSupport = 0;
        if (SUCCEEDED(swapChain->CheckColorSpaceSupport(newColorSpace, &colorSpaceSupport))
            && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
        {
            ThrowIfFailed(swapChain->SetColorSpace1(newColorSpace));
        }
#endif
    }

    void GraphicsDevice_D3D11::Shutdown()
    {
        mainSwapChain.Reset();

        // Release leaked resources.
        ReleaseTrackedResources();

        for (uint32_t i = 0; i < kTotalCommandContexts; i++)
        {
            SAFE_RELEASE(userDefinedAnnotations[i]);
            SAFE_RELEASE(deviceContexts[i]);
        }

        ULONG refCount = d3dDevice->Release();

#ifdef _DEBUG
        if (refCount > 0)
        {
            LOG_WARN("There are %d unreleased references left on the ID3D11Device", refCount);

            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(d3dDevice->QueryInterface(&d3dDebug)))
            {
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
                d3dDebug->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif
        SAFE_RELEASE(dxgiFactory);

#ifdef _DEBUG
        {
            IDXGIDebug1* dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug->Release();
            }
        }
#endif
        }

    void GraphicsDevice_D3D11::CreateFactory()
    {
        SAFE_RELEASE(dxgiFactory);

#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
        bool debugDXGI = false;
        {
            IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1 != nullptr && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue))))
#endif
            {
                debugDXGI = true;

                ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)));

                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }

        if (!debugDXGI)
#endif
        {
            ThrowIfFailed(CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&dxgiFactory));
        }

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            tearingSupported = true;
            BOOL allowTearing = FALSE;

            RefPtr<IDXGIFactory5> factory5;
            HRESULT hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory5), (void**)factory5.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
                tearingSupported = false;
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
        }

        // Disable FLIP if not on a supporting OS
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        {
            flipPresentSupported = true;

            RefPtr<IDXGIFactory4> factory4;
            if (FAILED(dxgiFactory->QueryInterface(__uuidof(IDXGIFactory4), (void**)factory4.GetAddressOf())))
            {
                flipPresentSupported = false;
#ifdef _DEBUG
                OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
            }
        }
#endif
    }

    void GraphicsDevice_D3D11::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        // Init capabilities
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

            caps.backendType = BackendType::Direct3D12;
            caps.vendorId = static_cast<GPUVendorId>(desc.VendorId);
            caps.deviceId = desc.DeviceId;

            std::wstring deviceName(desc.Description);
            caps.adapterName = alimer::ToUtf8(deviceName);

            // Features
            caps.features.independentBlend = true;
            caps.features.computeShader = true;
            caps.features.geometryShader = true;
            caps.features.tessellationShader = true;
            caps.features.logicOp = true;
            caps.features.multiViewport = true;
            caps.features.fullDrawIndexUint32 = true;
            caps.features.multiDrawIndirect = true;
            caps.features.fillModeNonSolid = true;
            caps.features.samplerAnisotropy = true;
            caps.features.textureCompressionETC2 = false;
            caps.features.textureCompressionASTC_LDR = false;
            caps.features.textureCompressionBC = true;
            caps.features.textureCubeArray = true;
            caps.features.raytracing = false;

            // Limits
            caps.limits.maxVertexAttributes = kMaxVertexAttributes;
            caps.limits.maxVertexBindings = kMaxVertexAttributes;
            caps.limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
            caps.limits.maxVertexBindingStride = kMaxVertexBufferStride;

            //caps.limits.maxTextureDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
            caps.limits.maxTextureDimension2D = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            caps.limits.maxTextureDimension3D = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            caps.limits.maxTextureDimensionCube = D3D11_REQ_TEXTURECUBE_DIMENSION;
            caps.limits.maxTextureArrayLayers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            caps.limits.maxColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
            caps.limits.maxUniformBufferSize = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
            caps.limits.minUniformBufferOffsetAlignment = 256u;
            caps.limits.maxStorageBufferSize = UINT32_MAX;
            caps.limits.minStorageBufferOffsetAlignment = 16;
            caps.limits.maxSamplerAnisotropy = D3D11_MAX_MAXANISOTROPY;
            caps.limits.maxViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            caps.limits.maxViewportWidth = D3D11_VIEWPORT_BOUNDS_MAX;
            caps.limits.maxViewportHeight = D3D11_VIEWPORT_BOUNDS_MAX;
            caps.limits.maxTessellationPatchSize = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
            caps.limits.pointSizeRangeMin = 1.0f;
            caps.limits.pointSizeRangeMax = 1.0f;
            caps.limits.lineWidthRangeMin = 1.0f;
            caps.limits.lineWidthRangeMax = 1.0f;
            caps.limits.maxComputeSharedMemorySize = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
            caps.limits.maxComputeWorkGroupCountX = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            caps.limits.maxComputeWorkGroupCountY = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            caps.limits.maxComputeWorkGroupCountZ = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            caps.limits.maxComputeWorkGroupInvocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
            caps.limits.maxComputeWorkGroupSizeX = D3D11_CS_THREAD_GROUP_MAX_X;
            caps.limits.maxComputeWorkGroupSizeY = D3D11_CS_THREAD_GROUP_MAX_Y;
            caps.limits.maxComputeWorkGroupSizeZ = D3D11_CS_THREAD_GROUP_MAX_Z;

            /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
            UINT dxgi_fmt_caps = 0;
            for (uint32_t format = (static_cast<uint32_t>(PixelFormat::Unknown) + 1); format < static_cast<uint32_t>(PixelFormat::Count); format++)
            {
                const DXGI_FORMAT dxgiFormat = ToDXGIFormat((PixelFormat)format);

                if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
                    continue;

                UINT formatSupport = 0;
                if (FAILED(d3dDevice->CheckFormatSupport(dxgiFormat, &formatSupport))) {
                    continue;
                }

                D3D11_FORMAT_SUPPORT tf = (D3D11_FORMAT_SUPPORT)formatSupport;
            }
        }
    }

    void GraphicsDevice_D3D11::BeginFrame()
    {
        if (!isLost)
            return;
    }

    void GraphicsDevice_D3D11::EndFrame()
    {
        if (!isLost)
            return;


        if (!dxgiFactory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            CreateFactory();
        }
    }

    void GraphicsDevice_D3D11::HandleDeviceLost()
    {
        if (events)
        {
            events->OnDeviceLost();
        }

        // Release leaked resources.
        ReleaseTrackedResources();

        for (uint32_t i = 0; i < kTotalCommandContexts; i++)
        {
            SAFE_RELEASE(userDefinedAnnotations[i]);
            SAFE_RELEASE(deviceContexts[i]);
        }

        ULONG refCount = d3dDevice->Release();
#ifdef _DEBUG
        if (refCount > 0)
        {
            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(d3dDevice->QueryInterface(&d3dDebug)))
            {
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
                d3dDebug->Release();
            }
        }
#endif

        SAFE_RELEASE(dxgiFactory);

        CreateDeviceResources();
        CreateWindowSizeDependentResources();

        if (events)
        {
            events->OnDeviceRestored();
        }
    }

    SwapChainHandle GraphicsDevice_D3D11::CreateSwapChain(const SwapChainDesc& desc)
    {
        if (swapChains.isFull()) {
            LOG_ERROR("D3D11: Not enough free SwapChain slots.");
            return kInvalidSwapChain;
        }
        const int id = swapChains.alloc();
        ALIMER_ASSERT(id >= 0);

        SwapChainD3D11& swapChain = swapChains[id];
        swapChain.backbufferCount = 2u;
        switch (desc.presentMode)
        {
        case PresentMode::Immediate:
            swapChain.syncInterval = 0u;
            if (tearingSupported)
            {
                swapChain.presentFlags = DXGI_PRESENT_ALLOW_TEARING;
            }
            break;

        case PresentMode::Mailbox:
            swapChain.syncInterval = 2u;
            swapChain.presentFlags = 0u;
            break;

        default:
            swapChain.syncInterval = 1u;
            swapChain.presentFlags = 0u;
            break;
        }

        swapChain.colorFormat = desc.colorFormat;
        swapChain.depthStencilFormat = desc.depthStencilFormat;

        // Determine the render target size in pixels.
        const DXGI_FORMAT backBufferFormat = ToDXGIFormat(srgbToLinearFormat(desc.colorFormat));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = desc.width;
        swapChainDesc.Height = desc.height;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = swapChain.backbufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (!flipPresentSupported) {
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        }
#else
        swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
#endif
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        if (tearingSupported) {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = !desc.isFullscreen;

        // Create a SwapChain from a Win32 window.
        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            d3dDevice,
            (HWND)desc.windowHandle,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            &swapChain.handle
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(dxgiFactory->MakeWindowAssociation((HWND)desc.windowHandle, DXGI_MWA_NO_ALT_ENTER));
#else
        // Create a swap chain for the window.
        RefPtr<IDXGISwapChain1> tempSwapChain;
        ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
            d3dDevice,
            window,
            &swapChainDesc,
            nullptr,
            tempSwapChain.GetAddressOf()
        ));

        ThrowIfFailed(tempSwapChain->QueryInterface(&swapChain.handle));
#endif

        SwapChainHandle handle = { (uint32_t)id };
        AfterReset(handle);

        return handle;
        }

    void GraphicsDevice_D3D11::DestroySwapChain(SwapChainHandle handle)
    {
        if (!handle.isValid())
            return;

        SwapChainD3D11& swapChain = swapChains[handle.id];
        SAFE_RELEASE(swapChain.handle);
        swapChains.dealloc(handle.id);
    }

    uint32_t GraphicsDevice_D3D11::GetBackbufferCount(SwapChainHandle handle)
    {
        // Under Direct3D11 we only use first buffer.
        ALIMER_UNUSED(handle);
        return 1;
    }

    uint64_t GraphicsDevice_D3D11::GetBackbufferTexture(SwapChainHandle handle, uint32_t index)
    {
        ALIMER_ASSERT(index == 0);
        ID3D11Texture2D* backbuffer;
        ThrowIfFailed(swapChains[handle.id].handle->GetBuffer(0, IID_PPV_ARGS(&backbuffer)));

        return (uint64_t)backbuffer;
    }

    uint32_t GraphicsDevice_D3D11::Present(SwapChainHandle handle)
    {
        SwapChainD3D11& swapChain = swapChains[handle.id];
        HRESULT hr = swapChain.handle->Present(swapChain.syncInterval, swapChain.presentFlags);

        // If the device was removed either by a disconnection or a driver upgrade, we
        // must recreate all device resources.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif

            isLost = true;

            HandleDeviceLost();
        }
        else
        {
            ThrowIfFailed(hr);
        }

        return 0;
    }

    TextureHandle GraphicsDevice_D3D11::AllocTextureHandle()
    {
        if (textures.isFull()) {
            LOG_ERROR("D3D11: Not enough free texture slots.");
            return kInvalidTexture;
        }
        const int id = textures.alloc();
        ALIMER_ASSERT(id >= 0);

        TextureD3D11& texture = textures[id];
        texture.handle = nullptr;
        return { (uint32_t)id };
    }

    TextureHandle GraphicsDevice_D3D11::CreateTexture(const TextureDescription& desc, uint64_t nativeHandle, const void* pData, bool autoGenerateMipmaps)
    {
        TextureHandle handle = AllocTextureHandle();
        TextureD3D11& texture = textures[handle.id];
        texture.dxgiFormat = ToDXGIFormatWitUsage(desc.format, desc.usage);
        texture.width = desc.width;
        texture.height = desc.height;
        texture.mipLevels = desc.mipLevels;

        HRESULT hr = S_OK;
        if (nativeHandle)
        {
            texture.handle = (ID3D11Texture2D*)nativeHandle;
        }
        else
        {
            D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
            UINT bindFlags = 0;
            UINT CPUAccessFlags = 0;
            UINT miscFlags = 0;
            if (desc.type == TextureType::TypeCube) {
                miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
            }

            if ((desc.usage & TextureUsage::Sampled) != TextureUsage::None) {
                bindFlags |= D3D11_BIND_SHADER_RESOURCE;
            }
            if ((desc.usage & TextureUsage::Storage) != TextureUsage::None) {
                bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            if ((desc.usage & TextureUsage::RenderTarget) != TextureUsage::None) {
                if (IsDepthStencilFormat(desc.format)) {
                    bindFlags |= D3D11_BIND_DEPTH_STENCIL;
                }
                else {
                    bindFlags |= D3D11_BIND_RENDER_TARGET;
                }
            }

            if (autoGenerateMipmaps)
            {
                UINT formatSupport = 0;
                if (FAILED(d3dDevice->CheckFormatSupport(texture.dxgiFormat, &formatSupport))) {
                    textures.dealloc(handle.id);
                    return kInvalidTexture;
                }

                if ((formatSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN) == 0) {

                }

                bindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                miscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
            }

            if (desc.type == TextureType::Type2D || desc.type == TextureType::TypeCube) {
                D3D11_TEXTURE2D_DESC d3d11_desc = {};
                d3d11_desc.Width = desc.width;
                d3d11_desc.Height = desc.height;
                d3d11_desc.MipLevels = desc.mipLevels;
                d3d11_desc.ArraySize = desc.arraySize;
                d3d11_desc.Format = texture.dxgiFormat;
                d3d11_desc.SampleDesc.Count = 1;
                d3d11_desc.SampleDesc.Quality = 0;
                d3d11_desc.Usage = usage;
                d3d11_desc.BindFlags = bindFlags;
                d3d11_desc.CPUAccessFlags = CPUAccessFlags;
                d3d11_desc.MiscFlags = miscFlags;

                hr = d3dDevice->CreateTexture2D(
                    &d3d11_desc,
                    NULL,
                    (ID3D11Texture2D**)&texture.handle
                );
            }
            else if (desc.type == TextureType::Type3D) {
                D3D11_TEXTURE3D_DESC d3d11_desc = {};
                d3d11_desc.Width = desc.width;
                d3d11_desc.Height = desc.height;
                d3d11_desc.Depth = desc.depth;
                d3d11_desc.MipLevels = desc.mipLevels;
                d3d11_desc.Format = texture.dxgiFormat;
                d3d11_desc.Usage = usage;
                d3d11_desc.BindFlags = bindFlags;
                d3d11_desc.CPUAccessFlags = CPUAccessFlags;
                d3d11_desc.MiscFlags = miscFlags;

                hr = d3dDevice->CreateTexture3D(
                    &d3d11_desc,
                    NULL,
                    (ID3D11Texture3D**)&texture.handle
                );
            }
        }

        if (FAILED(hr)) {
            textures.dealloc(handle.id);
            return kInvalidTexture;
        }

        if ((desc.usage & TextureUsage::RenderTarget) != TextureUsage::None)
        {
            if (IsDepthStencilFormat(desc.format)) {
                ID3D11DepthStencilView* dsv;
                ThrowIfFailed(d3dDevice->CreateDepthStencilView(texture.handle, nullptr, &dsv));
                texture.DSVs.Push(dsv);
            }
            else
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtViewDesc = {};
                rtViewDesc.Format = texture.dxgiFormat;

                switch (desc.type)
                {
                case TextureType::Type2D:

                    if (desc.sampleCount <= TextureSampleCount::Count1)
                    {
                        if (desc.arraySize > 1)
                        {
                            rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                            rtViewDesc.Texture2DArray.MipSlice = 0;
                            //rtViewDesc.Texture2DArray.FirstArraySlice = slice;
                            rtViewDesc.Texture2DArray.ArraySize = desc.arraySize;
                        }
                        else
                        {
                            rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                            rtViewDesc.Texture2D.MipSlice = 0;
                        }
                    }
                    else
                    {
                        if (desc.arraySize > 1)
                        {
                            rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                            //rtViewDesc.Texture2DMSArray.FirstArraySlice = slice;
                            rtViewDesc.Texture2DMSArray.ArraySize = desc.arraySize;
                        }
                        else
                        {
                            rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                        }
                    }

                    break;
                case TextureType::Type3D:
                    rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                    rtViewDesc.Texture3D.MipSlice = 0;
                    rtViewDesc.Texture3D.FirstWSlice = 0;
                    rtViewDesc.Texture3D.WSize = (UINT)-1;
                    break;
                case TextureType::TypeCube:
                    rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtViewDesc.Texture2DArray.MipSlice = 0;
                    rtViewDesc.Texture2DArray.FirstArraySlice = 0;
                    rtViewDesc.Texture2DArray.ArraySize = desc.arraySize;
                    break;

                default:
                    break;
                }

                ID3D11RenderTargetView* rtv;
                ThrowIfFailed(d3dDevice->CreateRenderTargetView(texture.handle, &rtViewDesc, &rtv));
                texture.RTVs.Push(rtv);
            }
        }

        return handle;
    }

    void GraphicsDevice_D3D11::DestroyTexture(TextureHandle handle)
    {
        if (!handle.isValid())
            return;

        TextureD3D11& texture = textures[handle.id];
        for (uint32_t i = 0; i < texture.RTVs.Size(); i++) {
            texture.RTVs[i]->Release();
        }
        for (uint32_t i = 0; i < texture.DSVs.Size(); i++) {
            texture.DSVs[i]->Release();
        }

        texture.RTVs.Clear();
        texture.DSVs.Clear();
        SAFE_RELEASE(texture.handle);
        textures.dealloc(handle.id);
    }

    void GraphicsDevice_D3D11::ClearState(CommandList commandList)
    {
        deviceContexts[commandList]->ClearState();
    }

    void GraphicsDevice_D3D11::InsertDebugMarker(const char* name, CommandList commandList)
    {
        auto wideName = ToUtf16(name, strlen(name));
        userDefinedAnnotations[commandList]->SetMarker(wideName.c_str());
    }

    void GraphicsDevice_D3D11::PushDebugGroup(const char* name, CommandList commandList)
    {
        auto wideName = ToUtf16(name, strlen(name));
        userDefinedAnnotations[commandList]->BeginEvent(wideName.c_str());
    }

    void GraphicsDevice_D3D11::PopDebugGroup(CommandList commandList)
    {
        userDefinedAnnotations[commandList]->EndEvent();
    }

    void GraphicsDevice_D3D11::BeginRenderPass(const RenderPassDesc& desc, CommandList commandList)
    {
        uint32_t width = UINT32_MAX;
        uint32_t height = UINT32_MAX;

        uint32_t rtvsCount = 0;
        ID3D11RenderTargetView* rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        ID3D11DepthStencilView* depthStencilView = nullptr;

        for (uint32_t i = 0; i < kMaxColorAttachments; i++)
        {
            const RenderPassColorAttachment* attachment = &desc.colorAttachments[i];
            if (!attachment->texture.isValid())
                break;

            TextureD3D11& texture = textures[attachment->texture.id];

            width = min(width, Texture::CalculateMipSize(attachment->mipLevel, texture.width));
            height = min(height, Texture::CalculateMipSize(attachment->mipLevel, texture.height));

            uint32_t viewIndex = CalcSubresource(attachment->mipLevel, attachment->slice, texture.mipLevels);
            ID3D11RenderTargetView* rtv = texture.RTVs[viewIndex];

            if (attachment->loadAction == LoadAction::Clear)
            {
                deviceContexts[commandList]->ClearRenderTargetView(
                    rtv,
                    &desc.colorAttachments[i].clearColor.r
                );
            }
            rtvs[rtvsCount++] = rtv;
        }

        if (desc.depthStencilAttachment.texture.isValid())
        {
            const RenderPassDepthStencilAttachment* attachment = &desc.depthStencilAttachment;
            TextureD3D11& texture = textures[attachment->texture.id];
            width = min(width, Texture::CalculateMipSize(attachment->mipLevel, texture.width));
            height = min(height, Texture::CalculateMipSize(attachment->mipLevel, texture.height));

            UINT clearFlags = 0;
            if (attachment->depthLoadAction == LoadAction::Clear)
            {
                clearFlags |= D3D11_CLEAR_DEPTH;
            }

            if (attachment->stencilLoadOp == LoadAction::Clear)
            {
                clearFlags |= D3D11_CLEAR_STENCIL;
            }

            uint32_t viewIndex = CalcSubresource(attachment->mipLevel, attachment->slice, texture.mipLevels);
            depthStencilView = texture.DSVs[viewIndex];
            deviceContexts[commandList]->ClearDepthStencilView(
                depthStencilView,
                clearFlags,
                attachment->clearDepth,
                attachment->clearStencil
            );
        }

        deviceContexts[commandList]->OMSetRenderTargets(rtvsCount, rtvs, depthStencilView);

        // set viewport and scissor rect to cover render target size
        RectU rect = desc.renderArea;
        rect.x = min(width, rect.x);
        rect.y = min(height, rect.y);
        rect.width = min(width - rect.x, rect.width);
        rect.height = min(height - rect.y, rect.height);

        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = float(rect.x);
        viewport.TopLeftY = float(rect.y);
        viewport.Width = float(rect.width);
        viewport.Height = float(rect.height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D11_RECT scissorRect;
        scissorRect.left = long(rect.x);
        scissorRect.top = long(rect.y);
        scissorRect.right = long(rect.width);
        scissorRect.bottom = long(rect.height);

        deviceContexts[commandList]->RSSetViewports(1, &viewport);
        deviceContexts[commandList]->RSSetScissorRects(1, &scissorRect);

        blendColors[commandList] = { 1.0f, 1.0f, 1.0f, 1.0f };
        deviceContexts[commandList]->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    }

    void GraphicsDevice_D3D11::EndRenderPass(CommandList commandList)
    {
        ClearState(commandList);
    }

    void GraphicsDevice_D3D11::SetBlendColor(const Color& color, CommandList commandList)
    {
        blendColors[commandList] = color;
    }
    }

#endif // TODO
