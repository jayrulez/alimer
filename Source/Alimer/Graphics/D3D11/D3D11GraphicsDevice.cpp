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
#include "D3D11CommandContext.h"
#include "D3D11SwapChain.h"
//#include "D3D11Texture.h"
//#include "D3D11Buffer.h"
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

    bool D3D11GraphicsDevice::IsAvailable()
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
        if (CreateDXGIFactory1 == nullptr)
        {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");

        static HMODULE d3d11Lib = LoadLibraryA("d3d11.dll");
        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11Lib, "D3D11CreateDevice");
        if (D3D11CreateDevice == nullptr)
        {
            return false;
        }
#endif

        available = true;
        return true;
    }

    D3D11GraphicsDevice::D3D11GraphicsDevice(Window* window, const Desc& desc)
    {
        ALIMER_VERIFY(IsAvailable());

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        dxgiLib = LoadLibraryA("dxgi.dll");
        CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(dxgiLib, "CreateDXGIFactory1");
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");
        d3d11Lib = LoadLibraryA("d3d11.dll");
        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11Lib, "D3D11CreateDevice");
#endif

        CreateFactory();

        // Get adapter and create device.
        {
            IDXGIAdapter1* dxgiAdapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
            IDXGIFactory6* factory6;
            HRESULT hr = dxgiFactory->QueryInterface(&factory6);
            if (SUCCEEDED(hr))
            {
                DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
                if (any(desc.flags & GPUDeviceFlags::LowPowerPreference))
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
                return;
            }

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

            if (any(desc.flags & GPUDeviceFlags::DebugRuntime)
                || any(desc.flags & GPUDeviceFlags::GPUBaseValidation))
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
            mainContext = new D3D11CommandContext(this, immediateContext);
            SafeRelease(tempD3DDevice);
        }
    }

    D3D11GraphicsDevice::~D3D11GraphicsDevice()
    {
        Shutdown();

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        FreeLibrary(dxgiLib);
#endif
    }

    void D3D11GraphicsDevice::Shutdown()
    {
        delete mainContext;

        ULONG refCount = d3dDevice->Release();
#ifdef _DEBUG
        if (refCount > 0)
        {
            LOGE("Direct3D11: There are {} unreleased references left on the device", refCount);

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

    void D3D11GraphicsDevice::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
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

    bool D3D11GraphicsDevice::BeginFrame()
    {
        return true;
    }

    void D3D11GraphicsDevice::EndFrame()
    {
        if (isLost) {
            return;
        }

        HRESULT hr = S_OK;
        for (auto& swapChain : createdSwapChains)
        {
            hr = swapChain.handle->Present(swapChain.syncInterval, swapChain.presentFlags);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // TODO: Handle device lost.
                isLost = true;
                return;
            }

        }

        if (!dxgiFactory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            CreateFactory();
        }
    }

    CommandContext* D3D11GraphicsDevice::GetMainContext() const
    {
        return mainContext;
    }

    /* Resource creation methods */
    BufferHandle D3D11GraphicsDevice::AllocBufferHandle()
    {
        std::lock_guard<std::mutex> LockGuard(handle_mutex);

        if (buffers.isFull()) {
            LOGE("Direct3D11: Not enough free buffer slots.");
            return kInvalidBuffer;
        }

        const int id = buffers.Alloc();
        if (id < 0) {
            return kInvalidBuffer;
        }

        Buffer& buffer = buffers[id];
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
            bufferSize = Math::AlignTo(size, caps.limits.minUniformBufferOffsetAlignment);
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

        D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        D3D11_SUBRESOURCE_DATA initialData;
        memset(&initialData, 0, sizeof(initialData));
        if (data != nullptr) {
            initialData.pSysMem = data;
            initialDataPtr = &initialData;
        }

        ID3D11Buffer* d3dBuffer;
        HRESULT hr = d3dDevice->CreateBuffer(&d3dDesc, initialDataPtr, &d3dBuffer);
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

        Buffer& buffer = buffers[handle.id];
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

    SwapChainHandle D3D11GraphicsDevice::AllocSwapChainHandle()
    {
        std::lock_guard<std::mutex> LockGuard(handle_mutex);

        if (swapChains.isFull()) {
            LOGE("Direct3D11: Not enough free SwapChain slots.");
            return kInvalidSwapChain;
        }

        const int id = swapChains.Alloc();
        if (id < 0) {
            return kInvalidSwapChain;
        }

        SwapChain& swapChain = swapChains[id];
        swapChain.handle = nullptr;
        return { (uint32_t)id };
    }

    SwapChainHandle D3D11GraphicsDevice::CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool isFullscreen, bool enableVSync, PixelFormat preferredColorFormat)
    {
        UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        uint32 syncInterval = 1;
        uint32_t presentFlags = 0;

        if (!enableVSync)
        {
            syncInterval = 0;
            if (any(dxgiFactoryCaps & DXGIFactoryCaps::Tearing))
            {
                flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                presentFlags = DXGI_PRESENT_ALLOW_TEARING;
            }
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        const DXGI_SCALING dxgiScaling = DXGI_SCALING_STRETCH;
#else
        const DXGI_SCALING dxgiScaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
#endif

        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = 0;
        swapchainDesc.Height = 0;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.Stereo = false;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.BufferCount = kInflightFrameCount;
        swapchainDesc.Scaling = dxgiScaling;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapchainDesc.Flags = flags;

        IDXGISwapChain1* swapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND hwnd = (HWND)windowHandle;
        if (!IsWindow(hwnd)) {
            LOGE("Direct3D11: Invalid window handle.");
            return kInvalidSwapChain;
        }

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapchainDesc = {};
        fsSwapchainDesc.Windowed = !isFullscreen;

        // Create a SwapChain from a Win32 window.
        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            d3dDevice,
            hwnd,
            &swapchainDesc,
            &fsSwapchainDesc,
            nullptr,
            &swapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
        IUnknown* window = (IUnknown*)windowHandle;
        ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
            d3dDevice,
            window,
            &swapchainDesc,
            nullptr,
            &swapChain
        ));
#endif

        SwapChainHandle handle = AllocSwapChainHandle();
        swapChains[handle.id].handle = swapChain;
        swapChains[handle.id].syncInterval = syncInterval;
        swapChains[handle.id].presentFlags = presentFlags;
        createdSwapChains.push_back(swapChains[handle.id]);
        return handle;
    }

    void D3D11GraphicsDevice::Destroy(SwapChainHandle handle)
    {
        if (!handle.isValid())
            return;

        SwapChain& swapChain = swapChains[handle.id];
        SafeRelease(swapChain.handle);

        std::lock_guard<std::mutex> LockGuard(handle_mutex);
        swapChains.Dealloc(handle.id);
    }

    void D3D11GraphicsDevice::Present(SwapChainHandle handle)
    {
        HRESULT hr = swapChains[handle.id].handle->Present(swapChains[handle.id].syncInterval, swapChains[handle.id].presentFlags);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // TODO: Handle device lost.
            isLost = true;
            return;
        }
    }

    uint32_t D3D11GraphicsDevice::GetImageCount(SwapChainHandle handle) const
    {
        return 1;
    }

    void D3D11GraphicsDevice::HandleDeviceLost(HRESULT hr)
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
