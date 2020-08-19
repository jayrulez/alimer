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

#include "D3D11GPUAdapter.h"
#include "D3D11GPUSwapChain.h"
#include "D3D11GPUContext.h"
#include "D3D11GraphicsDevice.h"
#include "Core/String.h"

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

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    struct D3D11Initializer
    {
        HMODULE dxgiLib;
        HMODULE d3d11Lib;

        D3D11Initializer()
        {
            dxgiLib = nullptr;
            d3d11Lib = nullptr;
        }

        ~D3D11Initializer()
        {
            if (dxgiLib)
                FreeLibrary(dxgiLib);

            if (d3d11Lib)
                FreeLibrary(d3d11Lib);
        }

        bool Initialize()
        {
            dxgiLib = LoadLibraryA("dxgi.dll");
            if (!dxgiLib)
                return false;

            CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(dxgiLib, "CreateDXGIFactory1");
            if (!CreateDXGIFactory1)
                return false;

            CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
            DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");

            d3d11Lib = LoadLibraryA("d3d11.dll");
            if (!d3d11Lib)
                return false;

            D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11Lib, "D3D11CreateDevice");
            if (!D3D11CreateDevice) {
                return false;
            }

            return true;
        }
    };

    D3D11Initializer s_d3d11_Initializer;
#endif

    bool D3D11GraphicsDevice::IsAvailable()
    {
        static bool available_initialized = false;
        static bool available = false;
        if (available_initialized) {
            return available;
        }

        available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (!s_d3d11_Initializer.Initialize())
            return false;
#endif

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

    D3D11GraphicsDevice::D3D11GraphicsDevice(const GPUDeviceDescriptor& descriptor)
        : GraphicsDevice(GPUBackendType::D3D11)
    {
        ALIMER_VERIFY(IsAvailable());

        CreateFactory();

        // Get adapter and create device.
        {
            IDXGIAdapter1* dxgiAdapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
            IDXGIFactory6* dxgiFactory6 = nullptr;
            HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
            if (SUCCEEDED(hr))
            {
                const bool lowPower = false;
                DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
                if (lowPower)
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
                    dxgiAdapter,
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

            // Create main context.
            mainContext = new D3D11GPUContext(this, context);
            context->Release();
            device->Release();

            // Init adapter.
            adapter = new D3D11GPUAdapter(dxgiAdapter);

            // Init caps
            InitCapabilities();
        }

        mainSwapChain = CreateSwapChainCore(descriptor.swapChain);
    }

    D3D11GraphicsDevice::~D3D11GraphicsDevice()
    {
        Shutdown();
    }

    void D3D11GraphicsDevice::Shutdown()
    {
        SafeDelete(mainContext);
        mainSwapChain.Reset();

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

        SafeDelete(adapter);
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

    GPUAdapter* D3D11GraphicsDevice::GetAdapter() const
    {
        return adapter;
    }

    CommandContext* D3D11GraphicsDevice::GetMainContext() const
    {
        return mainContext;
    }

    GPUSwapChain* D3D11GraphicsDevice::GetMainSwapChain() const
    {
        return mainSwapChain.Get();
    }

    void D3D11GraphicsDevice::CreateFactory()
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
                dxgiInfoQueue->Release();
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
                isTearingSupported = false;
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                isTearingSupported = true;
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

    void D3D11GraphicsDevice::InitCapabilities()
    {
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

    bool D3D11GraphicsDevice::BeginFrameImpl()
    {
        return !isLost;
    }

    void D3D11GraphicsDevice::EndFrameImpl()
    {
        if (isLost) {
            return;
        }

        // TODO: Measure timestamp using query

        if (!dxgiFactory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            CreateFactory();
        }
    }

    void D3D11GraphicsDevice::Present(GPUSwapChain* swapChain, bool verticalSync)
    {
        HRESULT hr = S_OK;
        if (verticalSync)
        {
            hr = static_cast<D3D11GPUSwapChain*>(swapChain)->Present(1, 0);
        }
        else
        {

            hr = static_cast<D3D11GPUSwapChain*>(swapChain)->Present(0, isTearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0);
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
    }

    void D3D11GraphicsDevice::HandleDeviceLost()
    {
    }

    /* Resource creation methods */
    GPUSwapChain* D3D11GraphicsDevice::CreateSwapChainCore(const GPUSwapChainDescriptor& descriptor)
    {
        return new D3D11GPUSwapChain(this, descriptor);
    }

    

    BufferHandle D3D11GraphicsDevice::AllocBufferHandle()
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

    BufferHandle D3D11GraphicsDevice::CreateBuffer(BufferUsage usage, uint32_t size, uint32_t stride, const void* data)
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

    void D3D11GraphicsDevice::Destroy(BufferHandle handle)
    {
        if (!handle.isValid())
            return;

        D3D11Buffer& buffer = buffers[handle.id];
        SafeRelease(buffer.handle);

        std::lock_guard<std::mutex> LockGuard(handle_mutex);
        buffers.Dealloc(handle.id);
    }

    void D3D11GraphicsDevice::SetName(BufferHandle handle, const char* name)
    {
        if (!handle.isValid())
            return;

        D3D11SetObjectName(buffers[handle.id].handle, name);
    }
}
