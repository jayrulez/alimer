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


#include "D3D11GraphicsDevice.h"
#include "D3D11SwapChain.h"
#include "D3D11Texture.h"
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
                SafeRelease(dxgiFactory6);
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

    D3D11GraphicsDevice::D3D11GraphicsDevice()
        : GraphicsDevice()
        , d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        dxgiLib = LoadLibraryA("dxgi.dll");
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");
#endif

        CreateDeviceResources();
    }

    D3D11GraphicsDevice::~D3D11GraphicsDevice()
    {
        BackendShutdown();
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        FreeLibrary(dxgiLib);
#endif
    }

    void D3D11GraphicsDevice::CreateDeviceResources()
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
            SafeRelease(context);
            SafeRelease(device);
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

    void D3D11GraphicsDevice::CreateWindowSizeDependentResources()
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

    void D3D11GraphicsDevice::BackendShutdown()
    {
        // Release leaked resources.
        ReleaseTrackedResources();

        for (uint32_t i = 0; i < kTotalCommandContexts; i++)
        {
            SafeRelease(userDefinedAnnotations[i]);
            SafeRelease(deviceContexts[i]);
        }

        ULONG refCount = d3dDevice->Release();

#ifdef _DEBUG
        if (refCount > 0)
        {
            LOG_WARN("There are {} unreleased references left on the ID3D11Device", refCount);

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
        SafeRelease(dxgiFactory);

#ifdef _DEBUG
        {
            IDXGIDebug1* dxgiDebug;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1 != nullptr && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiDebug))))
#endif
            {
                dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug->Release();
            }
        }
#endif
    }

    void D3D11GraphicsDevice::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
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

    void D3D11GraphicsDevice::WaitForGPU()
    {
        return deviceContexts[0]->Flush();
    }

    bool D3D11GraphicsDevice::BeginFrameImpl()
    {
        return !isLost;
    }

    void D3D11GraphicsDevice::EndFrameImpl()
    {
        
    }

    void D3D11GraphicsDevice::HandleDeviceLost()
    {
        if (events)
        {
            events->OnDeviceLost();
        }

        // Release leaked resources.
        ReleaseTrackedResources();

        for (uint32_t i = 0; i < kTotalCommandContexts; i++)
        {
            SafeRelease(userDefinedAnnotations[i]);
            SafeRelease(deviceContexts[i]);
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

        SafeRelease(dxgiFactory);

        CreateDeviceResources();
        CreateWindowSizeDependentResources();

        if (events)
        {
            events->OnDeviceRestored();
        }
    }

    SwapChain* D3D11GraphicsDevice::CreateSwapChain(const SwapChainDescription& desc)
    {
        return new D3D11SwapChain(this, desc);
    }

    Texture* D3D11GraphicsDevice::CreateTexture(const TextureDescription& desc, const void* initialData)
    {
        return new D3D11Texture(this, desc, initialData);
    }

    CommandList D3D11GraphicsDevice::BeginCommandList(const char* name)
    {
        CommandList cmd = commandlistCount.fetch_add(1);
        if (deviceContexts[cmd] == nullptr)
        {
            // need to create one more command list:
            ALIMER_ASSERT(cmd < kMaxCommandLists);

            ThrowIfFailed(d3dDevice->CreateDeferredContext1(0, &deviceContexts[cmd]));
            ThrowIfFailed(deviceContexts[cmd]->QueryInterface(&userDefinedAnnotations[cmd]));
        }

        return cmd;
    }

    void D3D11GraphicsDevice::InsertDebugMarker(CommandList commandList, const char* name)
    {
        auto wideName = ToUtf16(name, strlen(name));
        userDefinedAnnotations[commandList]->SetMarker(wideName.c_str());
    }

    void D3D11GraphicsDevice::PushDebugGroup(CommandList commandList, const char* name)
    {
        auto wideName = ToUtf16(name, strlen(name));
        userDefinedAnnotations[commandList]->BeginEvent(wideName.c_str());
    }

    void D3D11GraphicsDevice::PopDebugGroup(CommandList commandList)
    {
        userDefinedAnnotations[commandList]->EndEvent();
    }

    void D3D11GraphicsDevice::SetScissorRect(CommandList commandList, const Rect& scissorRect)
    {
        D3D11_RECT d3dScissorRect;
        d3dScissorRect.left = LONG(scissorRect.x);
        d3dScissorRect.top = LONG(scissorRect.y);
        d3dScissorRect.right = LONG(scissorRect.x + scissorRect.width);
        d3dScissorRect.bottom = LONG(scissorRect.y + scissorRect.height);
        deviceContexts[commandList]->RSSetScissorRects(1, &d3dScissorRect);
    }

    void D3D11GraphicsDevice::SetScissorRects(CommandList commandList, const Rect* scissorRects, uint32_t count)
    {
        D3D11_RECT d3dScissorRects[kMaxViewportAndScissorRects];
        for (uint32_t i = 0; i < count; ++i)
        {
            d3dScissorRects[i].left = LONG(scissorRects[i].x);
            d3dScissorRects[i].top = LONG(scissorRects[i].y);
            d3dScissorRects[i].right = LONG(scissorRects[i].x + scissorRects[i].width);
            d3dScissorRects[i].bottom = LONG(scissorRects[i].y + scissorRects[i].height);
        }
        deviceContexts[commandList]->RSSetScissorRects(count, d3dScissorRects);
    }

    void D3D11GraphicsDevice::SetViewport(CommandList commandList, const Viewport& viewport)
    {
        deviceContexts[commandList]->RSSetViewports(1, reinterpret_cast<const D3D11_VIEWPORT*>(&viewport));
    }

    void D3D11GraphicsDevice::SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count)
    {
        D3D11_VIEWPORT d3dViewports[kMaxViewportAndScissorRects];
        for (uint32_t i = 0; i < count; ++i)
        {
            d3dViewports[i].TopLeftX = viewports[i].x;
            d3dViewports[i].TopLeftY = viewports[i].y;
            d3dViewports[i].Width = viewports[i].width;
            d3dViewports[i].Height = viewports[i].height;
            d3dViewports[i].MinDepth = viewports[i].minDepth;
            d3dViewports[i].MaxDepth = viewports[i].maxDepth;
        }
        deviceContexts[commandList]->RSSetViewports(count, d3dViewports);
    }

    void D3D11GraphicsDevice::SetBlendColor(CommandList commandList, const Color& color)
    {
        //deviceContexts[commandList]->OMSetBlendFactor(color.Data());
    }

    void D3D11GraphicsDevice::BindBuffer(CommandList commandList, uint32_t slot, GraphicsBuffer* buffer)
    {
        //D3D11Buffer* d3dBuffer = static_cast<D3D11Buffer*>(buffer);
    }

    void D3D11GraphicsDevice::BindBufferData(CommandList commandList, uint32_t slot, const void* data, uint32_t size)
    {
    }
}
