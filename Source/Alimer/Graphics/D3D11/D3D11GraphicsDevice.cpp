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
//#include "D3D11CommandContext.h"
//#include "D3D11SwapChain.h"
//#include "D3D11Texture.h"
//#include "D3D11Buffer.h"
#include "Core/String.h"

namespace alimer
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

    GraphicsImpl::GraphicsImpl()
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        dxgiLib = LoadLibraryA("dxgi.dll");
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");
#endif
        CreateFactory();
    }

    GraphicsImpl::~GraphicsImpl()
    {
        Shutdown();

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        FreeLibrary(dxgiLib);
#endif
    }

    void GraphicsImpl::Shutdown()
    {
        //defaultContext.Reset();
        ULONG refCount = d3dDevice->Release();
#ifdef _DEBUG
        if (refCount > 0)
        {
            //LOG_DEBUG("Direct3D11: There are {} unreleased references left on the device", refCount);

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

        SafeRelease(dxgiFactory);


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

    bool GraphicsImpl::Initialize(Window& window, GPUDeviceFlags flags)
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (any(flags & GPUDeviceFlags::DebugRuntime)
            || any(flags & GPUDeviceFlags::GPUBaseValidation))
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

        // Get adapter and create device
        {

            IDXGIAdapter1* dxgiAdapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
            IDXGIFactory6* factory6;
            HRESULT hr = dxgiFactory->QueryInterface(&factory6);
            if (SUCCEEDED(hr))
            {
                DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
                if (any(flags & GPUDeviceFlags::LowPowerPreference))
                {
                    gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
                }

                for (UINT adapterIndex = 0;
                    SUCCEEDED(factory6->EnumAdapterByGpuPreference(
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

            SafeRelease(factory6);
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
                LOGE("No Direct3D 11 device found");
                return false;
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
            ID3D11Device* tempD3DDevice;
            ID3D11DeviceContext* immediateContext;

            hr = E_FAIL;
            if (dxgiAdapter)
            {
                hr = D3D11CreateDevice(
                    dxgiAdapter,
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &tempD3DDevice,
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
                    &tempD3DDevice,
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

            if (any(flags & GPUDeviceFlags::DebugRuntime)
                || any(flags & GPUDeviceFlags::GPUBaseValidation))
            {
                ID3D11Debug* d3dDebug = nullptr;
                if (SUCCEEDED(tempD3DDevice->QueryInterface(_uuidof(ID3D11Debug), (void**)&d3dDebug)))
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

            ThrowIfFailed(tempD3DDevice->QueryInterface(&d3dDevice));
            //defaultContext.Reset(new D3D11CommandContext(this, immediateContext));
            SafeRelease(tempD3DDevice);
        }

        return true;
    }

    void GraphicsImpl::CreateFactory()
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
                presentFlagsNoVSync = DXGI_PRESENT_ALLOW_TEARING;
            }

            SafeRelease(factory5);
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
#else
        dxgiFactoryCaps |= DXGIFactoryCaps::FlipPresent;
#endif
    }

    void GraphicsImpl::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        // Init capabilities
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

            caps.backendType = BackendType::Direct3D11;
            caps.vendorId = desc.VendorId;
            caps.deviceId = desc.DeviceId;

            eastl::wstring deviceName(desc.Description);
            caps.adapterName = alimer::ToUtf8(deviceName);

            // Detect adapter type.
            /*if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                caps.adapterType = GPUAdapterType::CPU;
            }
            else {
                D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
                VHR(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

                caps.adapterType = arch.UMA ? GPUAdapterType::IntegratedGPU : GPUAdapterType::DiscreteGPU;
            }*/

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

    void GraphicsImpl::WaitForGPU()
    {
    }

    bool GraphicsImpl::BeginFrame()
    {
        return true;
    }

    uint64_t GraphicsImpl::EndFrame(uint64_t currentCPUFrame)
    {
        /*HRESULT hr = S_OK;
        for (uint32_t i = 1; i < viewports.Size(); i++)
        {
            hr = viewports[0]->GetHandle()->Present(0, presentFlagsNoVSync);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // TODO: Handle device lost.
                isLost = true;
                break;
            }
        }

        if (viewports.Size() > 0)
        {
            // Main viewport is presented with vertical sync on
            hr = viewports[0]->GetHandle()->Present(1, 0);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // TODO: Handle device lost.
                isLost = true;
            }
        }*/

        return currentCPUFrame;
    }

    /*RefPtr<SwapChain> D3D11GraphicsDevice::CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool isFullscreen, PixelFormat preferredColorFormat, PixelFormat depthStencilFormat)
    {
        return new D3D11SwapChain(this, windowHandle, width, height, isFullscreen, preferredColorFormat, depthStencilFormat);
    }

    RefPtr<Texture> D3D11GraphicsDevice::CreateTexture(const TextureDescription& desc, const void* initialData)
    {
        return new D3D11Texture(this, desc, initialData);
    }*/

    void GraphicsImpl::HandleDeviceLost(HRESULT hr)
    {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
        OutputDebugStringA(buff);
#endif
    }

    /*CommandContext* D3D11GraphicsDevice::GetDefaultContext() const
    {
        return defaultContext.Get();
    }*/
}
