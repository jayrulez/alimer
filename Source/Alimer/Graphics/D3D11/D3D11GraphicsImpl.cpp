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

#include "D3D11GraphicsImpl.h"
#include "Graphics/Texture.h"
#include "Graphics/Buffer.h"
#include "Core/String.h"

namespace alimer
{
    namespace
    {
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

        UINT D3D11GetBindFlags(BufferUsage usage)
        {
            if (any(usage & BufferUsage::Uniform))
            {
                // This cannot be combined with nothing else.
                return D3D11_BIND_CONSTANT_BUFFER;
            }

            UINT flags = {};
            if (any(usage & BufferUsage::Index))
                flags |= D3D11_BIND_INDEX_BUFFER;
            if (any(usage & BufferUsage::Vertex))
                flags |= D3D11_BIND_VERTEX_BUFFER;

            if (any(usage & BufferUsage::Storage))
            {
                flags |= D3D11_BIND_SHADER_RESOURCE;
                flags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            return flags;
        }
    }

    bool D3D11GraphicsImpl::IsAvailable()
    {
        static bool available_initialized = false;
        static bool available = false;
        if (available_initialized) {
            return available;
        }

        available_initialized = true;

        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
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

        if (SUCCEEDED(hr))
        {
            available = true;
            return true;
        }

        return false;
    }

    D3D11GraphicsImpl::D3D11GraphicsImpl()
    {
        ALIMER_VERIFY(IsAvailable());

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        dxgiLib = LoadLibraryA("dxgi.dll");
        ALIMER_ASSERT(dxgiLib != nullptr);
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");
#endif
        CreateFactory();
    }

    D3D11GraphicsImpl::~D3D11GraphicsImpl()
    {
        Shutdown();

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        FreeLibrary(dxgiLib);
#endif
    }

    void D3D11GraphicsImpl::Shutdown()
    {
        //delete mainContext;

        backbufferTexture.Reset();
        SafeRelease(swapChain);

        for (uint32_t i = 0; i < _countof(d3dContexts); i++)
        {
            SafeRelease(d3dAnnotations[i]);
            SafeRelease(d3dContexts[i]);
        }

        ULONG refCount = d3dDevice->Release();
#if !defined(NDEBUG)
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
        (void)refCount; // avoid warning
#endif

        dxgiFactory.Reset();

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (DXGIGetDebugInterface1 && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#endif
        {
            dxgiDebug1->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug1->Release();
        }
#endif
    }

    void D3D11GraphicsImpl::CreateFactory()
    {
#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
        bool debugDXGI = false;
        {
            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1 != nullptr && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#endif
            {
                debugDXGI = true;

                ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

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

            ComPtr<IDXGIFactory5> factory5;
            HRESULT hr = dxgiFactory.As(&factory5);
            if (SUCCEEDED(hr))
            {
                hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
                isTearingSupported = false;
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                isTearingSupported = true;
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

    void D3D11GraphicsImpl::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        // Init capabilities
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

            caps.rendererType = RendererType::Direct3D11;
            caps.vendorId = desc.VendorId;
            caps.deviceId = desc.DeviceId;

            std::wstring deviceName(desc.Description);
            caps.adapterName = alimer::ToUtf8(deviceName);

            // Detect adapter type.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                caps.adapterType = GPUAdapterType::CPU;
            }
            else
            {
                caps.adapterType = GPUAdapterType::IntegratedGPU;
            }

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
    }

    bool D3D11GraphicsImpl::Initialize(WindowHandle windowHandle, uint32_t width, uint32_t height, bool isFullscreen_)
    {
        isFullscreen = isFullscreen_;

        // Get adapter and create device.
        {
            ComPtr<IDXGIAdapter1> dxgiAdapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
            ComPtr<IDXGIFactory6> factory6;
            HRESULT hr = dxgiFactory.As(&factory6);
            if (SUCCEEDED(hr))
            {
                const bool lowPower = false;
                DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
                if (lowPower)
                {
                    gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
                }

                for (UINT adapterIndex = 0;
                    SUCCEEDED(factory6->EnumAdapterByGpuPreference(
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
                return false;
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
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0
            };

            // Create the Direct3D 11 API device object and a corresponding context.
            ID3D11Device* device;
            ID3D11DeviceContext* context;

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
                    &device,
                    &d3dFeatureLevel,
                    &context
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
            if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&d3dDebug))))
            {
                ID3D11InfoQueue* d3dInfoQueue;
                if (SUCCEEDED(d3dDebug->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
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

            ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&d3dDevice)));
            ThrowIfFailed(context->QueryInterface(IID_PPV_ARGS(&d3dContexts[0])));
            ThrowIfFailed(context->QueryInterface(IID_PPV_ARGS(&d3dAnnotations[0])));
            context->Release();
            device->Release();

            // Init caps
            InitCapabilities(dxgiAdapter.Get());
        }


        window = windowHandle;
        backbufferSize.x = width;
        backbufferSize.y = height;
        UpdateSwapChain();

        initialized = true;
        return true;
    }

    void D3D11GraphicsImpl::UpdateSwapChain()
    {
        d3dContexts[0]->OMSetRenderTargets(kMaxColorAttachments, zeroRTVS, nullptr);
        backbufferTexture.Reset();
        d3dContexts[0]->Flush();

        if (swapChain)
        {

        }
        else
        {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            const DXGI_SCALING dxgiScaling = DXGI_SCALING_STRETCH;
#else
            const DXGI_SCALING dxgiScaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
#endif

            DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
            swapchainDesc.Width = backbufferSize.x;
            swapchainDesc.Height = backbufferSize.y;
            swapchainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            swapchainDesc.Stereo = false;
            swapchainDesc.SampleDesc.Count = 1;
            swapchainDesc.SampleDesc.Quality = 0;
            swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchainDesc.BufferCount = kInflightFrameCount;
            swapchainDesc.Scaling = dxgiScaling;
            swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            if (isTearingSupported)
            {
                swapchainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapchainDesc = {};
            fsSwapchainDesc.Windowed = !isFullscreen;

            // Create a SwapChain from a Win32 window.
            ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
                d3dDevice,
                window,
                &swapchainDesc,
                &fsSwapchainDesc,
                nullptr,
                &swapChain
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else

            IDXGISwapChain1* tempSwapChain;
            ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
                d3dDevice,
                window,
                &swapchainDesc,
                nullptr,
                &tempSwapChain
            ));
            ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));
            SafeRelease(tempSwapChain);
#endif
        }

        // Create a render target view of the swap chain back buffer.
        {
            RefPtr<ID3D11Texture2D> backbufferTextureHandle;
            ThrowIfFailed(swapChain->GetBuffer(0, IID_PPV_ARGS(backbufferTextureHandle.GetAddressOf())));
            backbufferTexture = Texture::CreateExternalTexture(backbufferTextureHandle, backbufferSize.x, backbufferSize.y, PixelFormat::BGRA8Unorm, false);
        }

        if (depthStencilFormat != PixelFormat::Invalid)
        {
            // Create a depth stencil view for use with 3D rendering if needed.
            D3D11_TEXTURE2D_DESC depthStencilDesc = {};
            depthStencilDesc.Width = backbufferSize.x;
            depthStencilDesc.Height = backbufferSize.y;
            depthStencilDesc.MipLevels = 1;
            depthStencilDesc.ArraySize = 1;
            depthStencilDesc.Format = ToDXGIFormat(depthStencilFormat);
            depthStencilDesc.SampleDesc.Count = 1;
            depthStencilDesc.SampleDesc.Quality = 0;
            depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
            depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

            RefPtr<ID3D11Texture2D> depthStencil;
            ThrowIfFailed(d3dDevice->CreateTexture2D(
                &depthStencilDesc,
                nullptr,
                depthStencil.GetAddressOf()
            ));
        }
    }

    bool D3D11GraphicsImpl::BeginFrame()
    {
        return true;
    }

    void D3D11GraphicsImpl::EndFrame(uint64_t frameIndex)
    {
        if (isLost) {
            return;
        }

        HRESULT hr = S_OK;
        if (verticalSync)
        {
            hr = swapChain->Present(1, 0);
        }
        else
        {
            hr = swapChain->Present(0, isTearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0);
        }

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif

            // TODO: Handle device lost.
            HandleDeviceLost();
            isLost = true;
            return;
        }

        if (!dxgiFactory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            CreateFactory();
        }
    }

    void D3D11GraphicsImpl::HandleDeviceLost()
    {
    }

    /* Resource creation methods */
    TextureHandle D3D11GraphicsImpl::AllocTextureHandle()
    {
        std::lock_guard<std::mutex> LockGuard(handle_mutex);

        if (textures.isFull()) {
            LOGE("Not enough free texture slots.");
            return kInvalidTexture;
        }
        const int id = textures.Alloc();
        ALIMER_ASSERT(id >= 0);

        D3D11Texture& texture = textures[id];
        texture.handle = nullptr;
        return { (uint32_t)id };
    }

    TextureHandle D3D11GraphicsImpl::CreateTexture(const TextureDescription* desc, const void* data)
    {
        TextureHandle handle = kInvalidTexture;

        if (desc->externalHandle != nullptr)
        {
            handle = AllocTextureHandle();
            textures[handle.id].handle = (ID3D11Resource*)desc->externalHandle;
            textures[handle.id].handle->AddRef();
        }
        else
        {
            switch (desc->dimension)
            {
            case TextureDimension::Texture2D:
                handle = CreateTexture2D(desc->width, desc->height, data);
                break;

            default:
                break;
            }
        }

        return handle;
    }

    TextureHandle D3D11GraphicsImpl::CreateTexture2D(uint32_t width, uint32_t height, const void* data)
    {
        D3D11_TEXTURE2D_DESC d3dDesc;
        d3dDesc.Width = width;
        d3dDesc.Height = height;
        d3dDesc.MipLevels = 1u;
        d3dDesc.ArraySize = 1u;
        d3dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        d3dDesc.SampleDesc.Count = 1;
        d3dDesc.SampleDesc.Quality = 0;
        d3dDesc.Usage = D3D11_USAGE_DEFAULT;
        d3dDesc.BindFlags = 0;
        d3dDesc.CPUAccessFlags = 0;
        d3dDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        D3D11_SUBRESOURCE_DATA initialData;
        memset(&initialData, 0, sizeof(initialData));
        if (data != nullptr) {
            initialData.pSysMem = data;
            initialDataPtr = &initialData;
        }

        ID3D11Texture2D* resource;
        HRESULT hr = d3dDevice->CreateTexture2D(&d3dDesc, initialDataPtr, &resource);
        if (FAILED(hr)) {
            LOGE("Direct3D11: Failed to create 2D texture");
            return kInvalidTexture;
        }

        TextureHandle handle = AllocTextureHandle();
        textures[handle.id].handle = resource;
        return handle;
    }

    void D3D11GraphicsImpl::Destroy(TextureHandle handle)
    {
        if (!handle.isValid())
            return;

        D3D11Texture& texture = textures[handle.id];
        SafeRelease(texture.handle);

        texture.srvs.clear();
        texture.uavs.clear();
        texture.rtvs.clear();
        texture.dsvs.clear();

        std::lock_guard<std::mutex> LockGuard(handle_mutex);
        textures.Dealloc(handle.id);
    }

    void D3D11GraphicsImpl::SetName(TextureHandle handle, const char* name)
    {
        if (!handle.isValid())
            return;

        D3D11SetObjectName(textures[handle.id].handle, name);
    }

    ID3D11ShaderResourceView* D3D11GraphicsImpl::GetSRV(Texture* texture, DXGI_FORMAT format, uint32_t level, uint32_t slice)
    {
        ALIMER_ASSERT(texture);

        // For non-array textures force slice to 0.
        if (texture->GetArrayLayers() <= 1)
            slice = 0;

        const uint32_t subresource = texture->GetSubresourceIndex(level, slice);

        D3D11Texture& d3dTexture = textures[texture->GetHandle().id];

        // Already created?
        if (d3dTexture.srvs.size() > subresource) {
            return d3dTexture.srvs[subresource].Get();
        }

        D3D11_RESOURCE_DIMENSION type;
        d3dTexture.handle->GetType(&type);

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.Format = format;

        switch (type)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            ((ID3D11Texture1D*)d3dTexture.handle)->GetDesc(&desc);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray.MostDetailedMip = level;
                viewDesc.Texture1DArray.MipLevels = desc.MipLevels;

                if (slice > 0)
                {
                    viewDesc.Texture1DArray.FirstArraySlice = slice;
                    viewDesc.Texture1DArray.ArraySize = 1;
                }
                else
                {
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D.MostDetailedMip = level;
                viewDesc.Texture1D.MipLevels = desc.MipLevels;
            }
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            ((ID3D11Texture2D*)d3dTexture.handle)->GetDesc(&desc);

            if (desc.SampleDesc.Count > 1)
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = slice;
                        viewDesc.Texture2DMSArray.ArraySize = 1;
                    }
                    else
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = 0;
                        viewDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.MostDetailedMip = level;
                    viewDesc.Texture2DArray.MipLevels = desc.MipLevels;

                    if (slice > 0)
                    {
                        viewDesc.Texture2DArray.ArraySize = 1;
                        viewDesc.Texture2DArray.FirstArraySlice = slice;
                    }
                    else
                    {
                        viewDesc.Texture2DArray.FirstArraySlice = 0;
                        viewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MostDetailedMip = level;
                    viewDesc.Texture2D.MipLevels = desc.MipLevels;
                }
            }

            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC desc = {};
            ((ID3D11Texture3D*)d3dTexture.handle)->GetDesc(&desc);

            viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            viewDesc.Texture3D.MostDetailedMip = level;
            viewDesc.Texture3D.MipLevels = desc.MipLevels;
            break;
        }

        default:
            break;
        }

        RefPtr<ID3D11ShaderResourceView> srv;
        ThrowIfFailed(d3dDevice->CreateShaderResourceView(d3dTexture.handle, &viewDesc, srv.GetAddressOf()));
        d3dTexture.srvs.push_back(srv);
        return srv.Get();
    }

    ID3D11UnorderedAccessView* D3D11GraphicsImpl::GetUAV(Texture* texture, DXGI_FORMAT format, uint32_t level, uint32_t slice)
    {
        ALIMER_ASSERT(texture);

        // For non-array textures force slice to 0.
        if (texture->GetArrayLayers() <= 1)
            slice = 0;

        const uint32_t subresource = texture->GetSubresourceIndex(level, slice);

        D3D11Texture& d3dTexture = textures[texture->GetHandle().id];

        // Already created?
        if (d3dTexture.uavs.size() > subresource) {
            return d3dTexture.uavs[subresource].Get();
        }

        D3D11_RESOURCE_DIMENSION type;
        d3dTexture.handle->GetType(&type);

        D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
        viewDesc.Format = format;

        switch (type)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            ((ID3D11Texture1D*)d3dTexture.handle)->GetDesc(&desc);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray.MipSlice = level;

                if (slice > 0)
                {
                    viewDesc.Texture1DArray.FirstArraySlice = slice;
                    viewDesc.Texture1DArray.ArraySize = 1;
                }
                else
                {
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D.MipSlice = level;
            }
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            ((ID3D11Texture2D*)d3dTexture.handle)->GetDesc(&desc);

            // UAV cannot be created from multisample texture.
            ALIMER_ASSERT(desc.SampleDesc.Count == 1);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                viewDesc.Texture2DArray.MipSlice = level;

                if (slice > 0)
                {
                    viewDesc.Texture2DArray.ArraySize = 1;
                    viewDesc.Texture2DArray.FirstArraySlice = slice;
                }
                else
                {
                    viewDesc.Texture2DArray.FirstArraySlice = 0;
                    viewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                viewDesc.Texture2D.MipSlice = level;
            }

            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC desc = {};
            ((ID3D11Texture3D*)d3dTexture.handle)->GetDesc(&desc);

            viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            viewDesc.Texture3D.MipSlice = level;

            if (slice > 0)
            {
                viewDesc.Texture3D.FirstWSlice = slice;
                viewDesc.Texture3D.WSize = 1;
            }
            else
            {
                viewDesc.Texture3D.FirstWSlice = 0;
                viewDesc.Texture3D.WSize = desc.Depth; /* A value of -1 indicates all of the slices along the w axis, starting from FirstWSlice. */
            }
            break;
        }

        default:
            break;
        }

        RefPtr<ID3D11UnorderedAccessView> uav;
        ThrowIfFailed(d3dDevice->CreateUnorderedAccessView(d3dTexture.handle, &viewDesc, uav.GetAddressOf()));
        d3dTexture.uavs.push_back(uav);
        return uav.Get();
    }

    ID3D11RenderTargetView* D3D11GraphicsImpl::GetRTV(Texture* texture, DXGI_FORMAT format, uint32_t level, uint32_t slice)
    {
        ALIMER_ASSERT(texture);

        // For non-array textures force slice to 0.
        if (texture->GetArrayLayers() <= 1)
            slice = 0;

        const uint32_t subresource = texture->GetSubresourceIndex(level, slice);

        D3D11Texture& d3dTexture = textures[texture->GetHandle().id];

        // Already created?
        if (d3dTexture.rtvs.size() > subresource) {
            return d3dTexture.rtvs[subresource].Get();
        }

        D3D11_RESOURCE_DIMENSION type;
        d3dTexture.handle->GetType(&type);

        D3D11_RENDER_TARGET_VIEW_DESC viewDesc = {};
        viewDesc.Format = format;

        switch (type)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            ((ID3D11Texture1D*)d3dTexture.handle)->GetDesc(&desc);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray.MipSlice = level;
                if (slice > 0)
                {
                    viewDesc.Texture1DArray.FirstArraySlice = slice;
                    viewDesc.Texture1DArray.ArraySize = 1;
                }
                else
                {
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D.MipSlice = level;
            }
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            ((ID3D11Texture2D*)d3dTexture.handle)->GetDesc(&desc);

            if (desc.SampleDesc.Count > 1)
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = slice;
                        viewDesc.Texture2DMSArray.ArraySize = 1;
                    }
                    else
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = 0;
                        viewDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.MipSlice = level;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DArray.ArraySize = 1;
                        viewDesc.Texture2DArray.FirstArraySlice = slice;
                    }
                    else
                    {
                        viewDesc.Texture2DArray.FirstArraySlice = 0;
                        viewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MipSlice = level;
                }
            }

            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC desc = {};
            ((ID3D11Texture3D*)d3dTexture.handle)->GetDesc(&desc);

            viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            viewDesc.Texture3D.MipSlice = level;

            if (slice > 0)
            {
                viewDesc.Texture3D.FirstWSlice = slice;
                viewDesc.Texture3D.WSize = 1;
            }
            else
            {
                viewDesc.Texture3D.FirstWSlice = 0;
                viewDesc.Texture3D.WSize = desc.Depth; /* A value of -1 indicates all of the slices along the w axis, starting from FirstWSlice. */
            }
            break;
        }

        default:
            break;
        }

        RefPtr<ID3D11RenderTargetView> rtv;
        ThrowIfFailed(d3dDevice->CreateRenderTargetView(d3dTexture.handle, &viewDesc, rtv.GetAddressOf()));
        d3dTexture.rtvs.push_back(rtv);
        return rtv.Get();
    }

    ID3D11DepthStencilView* D3D11GraphicsImpl::GetDSV(Texture* texture, DXGI_FORMAT format, uint32_t level, uint32_t slice)
    {
        ALIMER_ASSERT(texture);

        // For non-array textures force slice to 0.
        if (texture->GetArrayLayers() <= 1)
            slice = 0;

        const uint32_t subresource = texture->GetSubresourceIndex(level, slice);

        D3D11Texture& d3dTexture = textures[texture->GetHandle().id];

        // Already created?
        if (d3dTexture.dsvs.size() > subresource) {
            return d3dTexture.dsvs[subresource].Get();
        }

        D3D11_RESOURCE_DIMENSION type;
        d3dTexture.handle->GetType(&type);

        D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
        viewDesc.Format = format;
        viewDesc.Flags = 0; // TODO: Handle ReadOnlyDepth and ReadOnlyStencil D3D11_DSV_READ_ONLY_DEPTH

        switch (type)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            ((ID3D11Texture1D*)d3dTexture.handle)->GetDesc(&desc);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray.MipSlice = level;
                if (slice > 0)
                {
                    viewDesc.Texture1DArray.FirstArraySlice = slice;
                    viewDesc.Texture1DArray.ArraySize = 1;
                }
                else
                {
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D.MipSlice = level;
            }
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            ((ID3D11Texture2D*)d3dTexture.handle)->GetDesc(&desc);

            if (desc.SampleDesc.Count > 1)
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = slice;
                        viewDesc.Texture2DMSArray.ArraySize = 1;
                    }
                    else
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = 0;
                        viewDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.MipSlice = level;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DArray.ArraySize = 1;
                        viewDesc.Texture2DArray.FirstArraySlice = slice;
                    }
                    else
                    {
                        viewDesc.Texture2DArray.FirstArraySlice = 0;
                        viewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MipSlice = level;
                }
            }

            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            ALIMER_VERIFY_MSG(false, "Cannot create 3D Depth Stencil");
            ALIMER_DEBUG_BREAK();
            break;

        default:
            break;
        }

        RefPtr<ID3D11DepthStencilView> view;
        ThrowIfFailed(d3dDevice->CreateDepthStencilView(d3dTexture.handle, &viewDesc, view.GetAddressOf()));
        d3dTexture.dsvs.push_back(view);
        return view.Get();
    }

    BufferHandle D3D11GraphicsImpl::AllocBufferHandle()
    {
        std::lock_guard<std::mutex> LockGuard(handle_mutex);

        if (buffers.isFull()) {
            LOGE("Not enough free buffer slots.");
            return kInvalidBuffer;
        }
        const int id = buffers.Alloc();
        ALIMER_ASSERT(id >= 0);

        D3D11Buffer& buffer = buffers[id];
        buffer.handle = nullptr;
        return { (uint32_t)id };
    }

    BufferHandle D3D11GraphicsImpl::CreateBuffer(BufferUsage usage, uint32_t size, uint32_t stride, const void* data)
    {
        static constexpr uint64_t c_maxBytes = D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u;
        static_assert(c_maxBytes <= UINT32_MAX, "Exceeded integer limits");

        if (size > c_maxBytes)
        {
            LOGE("Direct3D11: Resource size too large for DirectX 11 (size {})", size);
            return kInvalidBuffer;
        }

        uint32_t bufferSize = size;
        if ((usage & BufferUsage::Uniform) != BufferUsage::None)
        {
            bufferSize = AlignTo(size, caps.limits.minUniformBufferOffsetAlignment);
        }

        const bool needUav = any(usage & BufferUsage::Storage) || any(usage & BufferUsage::Indirect);

        D3D11_BUFFER_DESC d3dDesc = {};
        d3dDesc.ByteWidth = bufferSize;
        d3dDesc.BindFlags = D3D11GetBindFlags(usage);
        d3dDesc.Usage = D3D11_USAGE_DEFAULT;
        d3dDesc.CPUAccessFlags = 0;

        if (any(usage & BufferUsage::Dynamic))
        {
            d3dDesc.Usage = D3D11_USAGE_DYNAMIC;
            d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else if (any(usage & BufferUsage::Staging)) {
            d3dDesc.Usage = D3D11_USAGE_STAGING;
            d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
        }

        if (needUav)
        {
            const bool rawBuffer = false;
            if (rawBuffer)
            {
                d3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            }
            else
            {
                d3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            }
        }

        if (any(usage & BufferUsage::Indirect))
        {
            d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        }

        d3dDesc.StructureByteStride = stride;

        D3D11_SUBRESOURCE_DATA initialData;
        initialData.pSysMem = data;
        initialData.SysMemPitch = 0;
        initialData.SysMemSlicePitch = 0;

        ID3D11Buffer* d3dBuffer;
        HRESULT hr = d3dDevice->CreateBuffer(&d3dDesc, data ? &initialData : nullptr, &d3dBuffer);
        if (FAILED(hr)) {
            LOGE("Direct3D11: Failed to create buffer");
            return kInvalidBuffer;
        }

        BufferHandle handle = AllocBufferHandle();
        buffers[handle.id].handle = d3dBuffer;
        return handle;
    }

    void D3D11GraphicsImpl::Destroy(BufferHandle handle)
    {
        if (!handle.isValid())
            return;

        D3D11Buffer& buffer = buffers[handle.id];
        SafeRelease(buffer.handle);

        std::lock_guard<std::mutex> LockGuard(handle_mutex);
        buffers.Dealloc(handle.id);
    }

    void D3D11GraphicsImpl::SetName(BufferHandle handle, const char* name)
    {
        if (!handle.isValid())
            return;

        D3D11SetObjectName(buffers[handle.id].handle, name);
    }

    /* Commands */
    void D3D11GraphicsImpl::PushDebugGroup(const String& name, CommandList commandList)
    {
        auto wideName = ToUtf16(name);
        d3dAnnotations[commandList]->BeginEvent(wideName.c_str());
    }

    void D3D11GraphicsImpl::PopDebugGroup(CommandList commandList)
    {
        d3dAnnotations[commandList]->EndEvent();
    }

    void D3D11GraphicsImpl::InsertDebugMarker(const String& name, CommandList commandList)
    {
        auto wideName = ToUtf16(name);
        d3dAnnotations[commandList]->SetMarker(wideName.c_str());
    }

    void D3D11GraphicsImpl::BeginRenderPass(CommandList commandList, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        ID3D11RenderTargetView* renderTargetViews[kMaxColorAttachments];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            Texture* texture = colorAttachments[i].texture == nullptr ? backbufferTexture.Get() : colorAttachments[i].texture;

            D3D11Texture& d3d11Texture = textures[texture->GetHandle().id];
            ID3D11RenderTargetView* rtv = GetRTV(texture, DXGI_FORMAT_UNKNOWN, colorAttachments[i].mipLevel, colorAttachments[i].slice);

            switch (colorAttachments[i].loadAction)
            {
            case LoadAction::DontCare:
                d3dContexts[commandList]->DiscardView(rtv);
                break;

            case LoadAction::Clear:
                d3dContexts[commandList]->ClearRenderTargetView(rtv, &colorAttachments[i].clearColor.r);
                break;

            default:
                break;
            }

            renderTargetViews[i] = rtv;
        }

        d3dContexts[commandList]->OMSetRenderTargets(numColorAttachments, renderTargetViews, nullptr);
    }

    void D3D11GraphicsImpl::EndRenderPass(CommandList commandList)
    {
        /* TODO: Resolve */
    }
}
