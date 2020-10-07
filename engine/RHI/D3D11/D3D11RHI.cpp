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

//#include "D3D11CommandBuffer.h"
#include "D3D11RHI.h"
#include "D3D11Texture.h"
#include "Platform/Window.h"

namespace Alimer
{
    namespace
    {
#if defined(_DEBUG)
        inline bool SdkLayersAvailable() noexcept
        {
            // Check for SDK Layer support.
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
        D3D11_USAGE D3D11GetUsage(MemoryUsage usage)
        {
            switch (usage)
            {
            case MemoryUsage::GpuOnly:  return D3D11_USAGE_DEFAULT;
            case MemoryUsage::CpuOnly:  return D3D11_USAGE_STAGING;
            case MemoryUsage::CpuToGpu: return D3D11_USAGE_DYNAMIC;
            case MemoryUsage::GpuToCpu: return D3D11_USAGE_STAGING;
            default:
                ALIMER_ASSERT_FAIL("Invalid Memory Usage");
                return D3D11_USAGE_DEFAULT;
            }
        }

        D3D11_CPU_ACCESS_FLAG D3D11GetCPUAccessFlags(MemoryUsage usage)
        {
            switch (usage)
            {
            case MemoryUsage::GpuOnly:  return (D3D11_CPU_ACCESS_FLAG)0;
            case MemoryUsage::CpuOnly:  return (D3D11_CPU_ACCESS_FLAG)(D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);
            case MemoryUsage::CpuToGpu: return (D3D11_CPU_ACCESS_FLAG)(D3D11_CPU_ACCESS_WRITE);
            case MemoryUsage::GpuToCpu: return (D3D11_CPU_ACCESS_FLAG)(D3D11_CPU_ACCESS_READ);

            default:
                ALIMER_ASSERT_FAIL("Invalid Memory Usage");
                return (D3D11_CPU_ACCESS_FLAG)0;
            }
        }

        UINT D3D11GetBindFlags(RHIBuffer::Usage usage)
        {
            if (any(usage & RHIBuffer::Usage::Uniform))
            {
                // This cannot be combined with nothing else.
                return D3D11_BIND_CONSTANT_BUFFER;
            }

            UINT flags = {};
            if (any(usage & RHIBuffer::Usage::Vertex))
                flags |= D3D11_BIND_VERTEX_BUFFER;
            if (any(usage & RHIBuffer::Usage::Index))
                flags |= D3D11_BIND_INDEX_BUFFER;

            if (any(usage & RHIBuffer::Usage::Storage))
            {
                flags |= D3D11_BIND_SHADER_RESOURCE;
                flags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            return flags;
        }
    }

    /* --- D3D11RHIBuffer --- */
    D3D11RHIBuffer::D3D11RHIBuffer(D3D11RHIDevice* device_, RHIBuffer::Usage usage, uint64_t size, MemoryUsage memoryUsage, const void* initialData)
        : RHIBuffer(usage, size, memoryUsage)
        , device(device_)
    {
        static constexpr uint64_t c_maxBytes = D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u;
        static_assert(c_maxBytes <= UINT32_MAX, "Exceeded integer limits");

        if (size > c_maxBytes)
        {
            LOGE("Direct3D11: Resource size too large for DirectX 11 (size {})", size);
            return;
        }

        uint64_t bufferSize = size;
        if (any(usage & Usage::Uniform))
        {
            bufferSize = AlignTo(size, device->GetCaps().limits.minUniformBufferOffsetAlignment);
        }
        else
        {
            bufferSize = AlignTo(size, static_cast<uint64_t>(4u));
        }

        D3D11_BUFFER_DESC d3dDesc;
        d3dDesc.ByteWidth = bufferSize;
        d3dDesc.Usage = D3D11GetUsage(memoryUsage);
        d3dDesc.BindFlags = D3D11GetBindFlags(usage);
        d3dDesc.CPUAccessFlags = D3D11GetCPUAccessFlags(memoryUsage);
        d3dDesc.MiscFlags = 0;
        d3dDesc.StructureByteStride = 0;

        if (any(usage & Usage::Storage))
        {
            d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        }

        if (any(usage & Usage::Indirect))
        {
            d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        }

        D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        D3D11_SUBRESOURCE_DATA initialResourceData = {};
        if (initialData != nullptr)
        {
            initialResourceData.pSysMem = initialData;
            initialDataPtr = &initialResourceData;
        }

        HRESULT hr = device->d3dDevice->CreateBuffer(&d3dDesc, initialDataPtr, &handle);
        if (FAILED(hr))
        {
            LOGE("Direct3D11: Failed to create buffer");
        }
    }

    D3D11RHIBuffer::~D3D11RHIBuffer()
    {
        Destroy();
    }

    void D3D11RHIBuffer::Destroy()
    {
        SafeRelease(handle);
    }

    void D3D11RHIBuffer::SetName(const std::string& newName)
    {
        RHIBuffer::SetName(newName);
        D3D11SetObjectName(handle, name);
    }

    /* --- D3D11RHIBuffer --- */
    D3D11RHISwapChain::D3D11RHISwapChain(D3D11RHIDevice* device_)
        : device(device_)
        , commandBuffer(new D3D11RHICommandBuffer(device_))
    {

    }

    D3D11RHISwapChain::~D3D11RHISwapChain()
    {
        Destroy();
    }

    void D3D11RHISwapChain::Destroy()
    {
        if (!handle)
            return;

        SafeDelete(commandBuffer);
        SafeRelease(handle);
    }

    bool D3D11RHISwapChain::CreateOrResize()
    {
        drawableSize = window->GetSize();

        UINT swapChainFlags = 0;

        if (device->IsTearingSupported())
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        bool needRecreate = handle == nullptr || window->GetNativeHandle() != windowHandle;
        if (needRecreate)
        {
            Destroy();

            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = UINT(drawableSize.width);
            swapChainDesc.Height = UINT(drawableSize.height);
            swapChainDesc.Format = ToDXGIFormat(SRGBToLinearFormat(colorFormat));
            swapChainDesc.Stereo = FALSE;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = kBufferCount;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#else
            swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#endif
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapChainDesc.Flags = swapChainFlags;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            windowHandle = (HWND)window->GetNativeHandle();
            ALIMER_ASSERT(IsWindow(windowHandle));

            //DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            //fsSwapChainDesc.Windowed = TRUE; // !window->IsFullscreen();

            // Create a swap chain for the window.
            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForHwnd(
                device->d3dDevice,
                windowHandle,
                &swapChainDesc,
                nullptr, // &fsSwapChainDesc,
                nullptr,
                &handle
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(device->GetDXGIFactory()->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
#else
            windowHandle = (IUnknown*)window->GetNativeHandle();
            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
                device->d3dDevice,
                windowHandle,
                &swapChainDesc,
                nullptr,
                &handle
            ));
#endif
        }
        else
        {

        }

        // Update present data
        if (!verticalSync)
        {
            syncInterval = 0u;
            // Recommended to always use tearing if supported when using a sync interval of 0.
            presentFlags = device->IsTearingSupported() ? DXGI_PRESENT_ALLOW_TEARING : 0u;
        }
        else
        {
            syncInterval = 1u;
            presentFlags = 0u;
        }

        AfterReset();

        return true;
    }

    void D3D11RHISwapChain::AfterReset()
    {
        colorTexture.Reset();

        ComPtr<ID3D11Texture2D> backbufferTexture;
        ThrowIfFailed(handle->GetBuffer(0, IID_PPV_ARGS(&backbufferTexture)));
        colorTexture = MakeRefPtr<D3D11Texture>(device, backbufferTexture.Get(), colorFormat);

        if (depthStencilFormat != PixelFormat::Invalid)
        {
            //GPUTextureDescription depthTextureDesc = GPUTextureDescription::New2D(depthStencilFormat, extent.width, extent.height, false, TextureUsage::RenderTarget);
            //depthStencilTexture.Reset(new D3D11Texture(device, depthTextureDesc, nullptr));
        }
    }

    RHITexture* D3D11RHISwapChain::GetCurrentTexture() const
    {
        return colorTexture.Get();
    }

    RHICommandBuffer* D3D11RHISwapChain::CurrentFrameCommandBuffer()
    {
        return commandBuffer;
    }

    /* --- D3D11RHICommandBuffer --- */
    D3D11RHICommandBuffer::D3D11RHICommandBuffer(D3D11RHIDevice* device)
        : annotation(nullptr)
    {
        // TODO: Add deferred contexts?
        //ThrowIfFailed(device->d3dDevice->CreateDeferredContext1(0, &context));
        context = device->context;
        ThrowIfFailed(context->QueryInterface(&annotation));
    }

    D3D11RHICommandBuffer::~D3D11RHICommandBuffer()
    {
        annotation->Release(); annotation = nullptr;
        //context->Release(); context = nullptr;
    }

    void D3D11RHICommandBuffer::PushDebugGroup(const std::string& name)
    {
        std::wstring wideLabel = ToUtf16(name);
        annotation->BeginEvent(wideLabel.c_str());
    }

    void D3D11RHICommandBuffer::PopDebugGroup()
    {
        annotation->EndEvent();
    }

    void D3D11RHICommandBuffer::InsertDebugMarker(const std::string& name)
    {
        std::wstring wideLabel = ToUtf16(name);
        annotation->SetMarker(wideLabel.c_str());
    }

    void D3D11RHICommandBuffer::SetViewport(const RHIViewport& viewport)
    {
        D3D11_VIEWPORT d3dViewport;
        d3dViewport.TopLeftX = viewport.x;
        d3dViewport.TopLeftY = viewport.y;
        d3dViewport.Width = viewport.width;
        d3dViewport.Height = viewport.height;
        d3dViewport.MinDepth = viewport.minDepth;
        d3dViewport.MaxDepth = viewport.maxDepth;
        context->RSSetViewports(1, &d3dViewport);
    }

    void D3D11RHICommandBuffer::SetScissorRect(const RectI& scissorRect)
    {
        D3D11_RECT d3dScissorRect;
        d3dScissorRect.left = static_cast<LONG>(scissorRect.x);
        d3dScissorRect.top = static_cast<LONG>(scissorRect.y);
        d3dScissorRect.right = static_cast<LONG>(scissorRect.x + scissorRect.width);
        d3dScissorRect.bottom = static_cast<LONG>(scissorRect.y + scissorRect.height);
        context->RSSetScissorRects(1, &d3dScissorRect);
    }

    void D3D11RHICommandBuffer::SetBlendColor(const Color& color)
    {

    }

    void D3D11RHICommandBuffer::BeginRenderPass(const RenderPassDesc& renderPass)
    {
        ID3D11RenderTargetView* renderTargetViews[kMaxColorAttachments];

        uint32_t numColorAttachments = 0;

        for (uint32_t i = 0; i < kMaxColorAttachments; i++)
        {
            auto& attachment = renderPass.colorAttachments[i];
            if (attachment.texture == nullptr)
                continue;

            D3D11Texture* texture = static_cast<D3D11Texture*>(attachment.texture);

            ID3D11RenderTargetView* rtv = texture->GetRTV(DXGI_FORMAT_UNKNOWN, attachment.mipLevel, attachment.slice);

            switch (attachment.loadAction)
            {
            case LoadAction::DontCare:
                context->DiscardView(rtv);
                break;

            case LoadAction::Clear:
                context->ClearRenderTargetView(rtv, &attachment.clearColor.r);
                break;

            default:
                break;
            }

            renderTargetViews[numColorAttachments++] = rtv;
        }

        context->OMSetRenderTargets(numColorAttachments, renderTargetViews, nullptr);
    }

    void D3D11RHICommandBuffer::EndRenderPass()
    {
        // TODO: Resolve 
        context->OMSetRenderTargets(kMaxColorAttachments, zeroRTVS, nullptr);
    }


    /* --- D3D11GraphicsDevice --- */
    bool D3D11RHIDevice::IsAvailable()
    {
        static bool available = false;
        static bool available_initialized = false;

        if (available_initialized) {
            return available;
        }

        available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        static HMODULE dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!dxgiDLL) {
            return false;
        }
        CreateDXGIFactory1 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(dxgiDLL, "CreateDXGIFactory1"));

        if (!CreateDXGIFactory1)
        {
            return false;
        }

        CreateDXGIFactory2 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(dxgiDLL, "CreateDXGIFactory2"));
        DXGIGetDebugInterface1 = reinterpret_cast<PFN_DXGI_GET_DEBUG_INTERFACE1>(GetProcAddress(dxgiDLL, "DXGIGetDebugInterface1"));

        static HMODULE d3d11DLL = LoadLibraryA("d3d11.dll");
        if (!d3d11DLL) {
            return false;
        }

        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11DLL, "D3D11CreateDevice");
        if (!D3D11CreateDevice) {
            return false;
        }
#endif

        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        HRESULT hr = D3D11CreateDevice(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            s_featureLevels,
            _countof(s_featureLevels),
            D3D11_SDK_VERSION,
            NULL,
            NULL,
            NULL
        );

        if (FAILED(hr))
        {
            return false;
        }

        available = true;
        return true;
    }

    D3D11RHIDevice::D3D11RHIDevice(GraphicsDeviceFlags flags)
        : debugRuntime(any(flags& GraphicsDeviceFlags::DebugRuntime) || any(flags & GraphicsDeviceFlags::GPUBasedValidation))
    {
        ALIMER_VERIFY(IsAvailable());

        CreateFactory();

        // Get adapter and create device.
        {
            ComPtr<IDXGIAdapter1> dxgiAdapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
            ComPtr<IDXGIFactory6> dxgiFactory6;
            HRESULT hr = dxgiFactory.As(&dxgiFactory6);
            if (SUCCEEDED(hr))
            {
                DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
                if (any(flags & GraphicsDeviceFlags::LowPowerPreference))
                {
                    gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
                }

                for (UINT adapterIndex = 0;
                    SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
                        adapterIndex,
                        gpuPreference,
                        IID_PPV_ARGS(dxgiAdapter.ReleaseAndGetAddressOf())));
                    adapterIndex++)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    {
                        // Don't select the Basic Render Driver adapter.
                        continue;
                    }

                    break;
                }
            }
#endif

            if (!dxgiAdapter)
            {
                for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.ReleaseAndGetAddressOf())); ++adapterIndex)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    {
                        // Don't select the Basic Render Driver adapter.
                        continue;
                    }

                    break;
                }
            }

            if (!dxgiAdapter)
            {
                LOGE("No Direct3D 11 device found");
                return;
            }

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

            // Determine DirectX hardware feature levels this app will support.
            static const D3D_FEATURE_LEVEL s_featureLevels[] =
            {
                D3D_FEATURE_LEVEL_12_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0
            };

            // Create the Direct3D 11 API device object and a corresponding context.
            ID3D11Device* tempDevice;
            ID3D11DeviceContext* tempContext;

            hr = E_FAIL;
            if (dxgiAdapter)
            {
                hr = D3D11CreateDevice(
                    dxgiAdapter.Get(),
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &tempDevice,
                    &featureLevel,
                    &tempContext
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
                    &tempDevice,
                    &featureLevel,
                    &tempContext
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
            if (SUCCEEDED(tempDevice->QueryInterface(&d3dDebug)))
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

            ThrowIfFailed(tempDevice->QueryInterface(IID_PPV_ARGS(&d3dDevice)));

            // Create main context.
            ThrowIfFailed(tempContext->QueryInterface(&context));

            // Init caps
            InitCapabilities(dxgiAdapter.Get());
        }
    }

    D3D11RHIDevice::~D3D11RHIDevice()
    {
        Shutdown();
    }

    void D3D11RHIDevice::Shutdown()
    {
        context->Release();
        context = nullptr;

#if !defined(NDEBUG)
        ULONG refCount = d3dDevice->Release();
        if (refCount > 0)
        {
            LOGD("Direct3D11: There are {} unreleased references left on the device", refCount);

            ID3D11Debug* d3d11Debug;
            if (SUCCEEDED(d3dDevice->QueryInterface(&d3d11Debug)))
            {
                d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
                d3d11Debug->Release();
            }
        }
#else
        d3dDevice->Release();
#endif
        d3dDevice = nullptr;

        dxgiFactory.Reset();

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (DXGIGetDebugInterface1 &&
            SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#endif
        {
            dxgiDebug1->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug1->Release();
        }
#endif
    }

    void D3D11RHIDevice::CreateFactory()
    {
#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
        bool debugDXGI = false;

        if (debugRuntime)
        {
            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1 != nullptr && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
#endif
            {
                debugDXGI = true;

                ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

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
            }
        }

        if (!debugDXGI)
#endif
        {
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
        }

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            ComPtr<IDXGIFactory5> factory5;
            HRESULT hr = dxgiFactory.As(&factory5);
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
        }

        // Disable HDR if we are on an OS that can't support FLIP swap effects
        {
            ComPtr<IDXGIFactory5> factory5;
            if (FAILED(dxgiFactory.As(&factory5)))
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: HDR swap chains not supported");
#endif
            }
            else
            {
                dxgiFactoryCaps |= DXGIFactoryCaps::HDR;
            }
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        // Disable FLIP if not on a supporting OS
        {
            ComPtr<IDXGIFactory4> factory4;
            if (FAILED(dxgiFactory.As(&factory4)))
            {
#ifdef _DEBUG
                OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
            }
            else
            {
                dxgiFactoryCaps |= DXGIFactoryCaps::FlipPresent;
            }
        }
#else
        dxgiFactoryCaps |= DXGIFactoryCaps::FlipPresent;
#endif
    }

    void D3D11RHIDevice::InitCapabilities(IDXGIAdapter1* adapter)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));

        caps.backendType = GraphicsBackendType::Direct3D11;
        caps.deviceId = desc.DeviceId;
        caps.vendorId = desc.VendorId;

        std::wstring deviceName(desc.Description);
        caps.adapterName = Alimer::ToUtf8(deviceName);

        // Detect adapter type.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            caps.adapterType = GPUAdapterType::CPU;
        }
        else
        {
            caps.adapterType = GPUAdapterType::IntegratedGPU;
        }

        D3D11_FEATURE_DATA_THREADING threadingSupport = { 0 };
        ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingSupport, sizeof(threadingSupport)));

        // Features
        caps.features.baseVertex = true;
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

        //caps.limits.maxTextureDimension1D = D3D11_REQ_TEXTURE1D_U_DIMENSION;
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
        if (caps.limits.maxViewports > kMaxViewportAndScissorRects) {
            caps.limits.maxViewports = kMaxViewportAndScissorRects;
        }

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
        for (uint32_t format = (static_cast<uint32_t>(PixelFormat::Invalid) + 1); format < static_cast<uint32_t>(PixelFormat::Count); format++)
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

    bool D3D11RHIDevice::IsDeviceLost() const
    {
        return deviceLost;
    }

    void D3D11RHIDevice::WaitForGPU()
    {
        context->Flush();
    }

    RHIDevice::FrameOpResult D3D11RHIDevice::BeginFrame(RHISwapChain* swapChain, BeginFrameFlags flags)
    {
        return FrameOpResult::Success;
    }

    RHIDevice::FrameOpResult D3D11RHIDevice::EndFrame(RHISwapChain* swapChain, EndFrameFlags flags)
    {
        D3D11RHISwapChain* d3dSwapChain = static_cast<D3D11RHISwapChain*>(swapChain);

        if (!any(flags & EndFrameFlags::SkipPresent))
        {
            HRESULT hr = d3dSwapChain->handle->Present(d3dSwapChain->syncInterval, d3dSwapChain->presentFlags);

            // Handle device lost logic.
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                LOGW("Device loss detected on SwapChain Present");
                HandleDeviceLost();
                return FrameOpResult::DeviceLost;
            }
            else if (FAILED(hr))
            {
                LOGW("Failed to present: %s", GetDXErrorStringAnsi(hr).c_str());
                return FrameOpResult::Error;
            }
        }
        else
        {
            context->Flush();
        }

        // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
        if (!dxgiFactory->IsCurrent())
        {
            CreateFactory();
        }

        return FrameOpResult::Success;
    }

    RHISwapChain* D3D11RHIDevice::CreateSwapChain()
    {
        return new D3D11RHISwapChain(this);
    }

    RHIBuffer* D3D11RHIDevice::CreateBuffer(RHIBuffer::Usage usage, uint64_t size, MemoryUsage memoryUsage)
    {
        return new D3D11RHIBuffer(this, usage, size, memoryUsage, nullptr);
    }

    RHIBuffer* D3D11RHIDevice::CreateStaticBuffer(RHIResourceUploadBatch* batch, const void* initialData, RHIBuffer::Usage usage, uint64_t size)
    {
        ALIMER_UNUSED(batch);

        return new D3D11RHIBuffer(this, usage, size, MemoryUsage::GpuOnly, initialData);
    }

    void D3D11RHIDevice::HandleDeviceLost()
    {
        deviceLost = true;
        HRESULT result = d3dDevice->GetDeviceRemovedReason();

        const char* reason = "?";
        switch (result)
        {
        case DXGI_ERROR_DEVICE_HUNG:			reason = "HUNG"; break;
        case DXGI_ERROR_DEVICE_REMOVED:			reason = "REMOVED"; break;
        case DXGI_ERROR_DEVICE_RESET:			reason = "RESET"; break;
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:	reason = "INTERNAL_ERROR"; break;
        case DXGI_ERROR_INVALID_CALL:			reason = "INVALID_CALL"; break;
        }

        LOGE("The Direct3D 12 device has been removed (Error: {} '{}').  Please restart the application.", result, reason);
    }
}
