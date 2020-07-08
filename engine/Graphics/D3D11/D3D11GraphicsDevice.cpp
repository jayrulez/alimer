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
#include "D3D11Buffer.h"
#include "Core/String.h"

namespace alimer
{
    namespace
    {
#if defined(GRAPHICS_DEBUG)
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
#endif /* defined(GRAPHICS_DEBUG) */
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
        if (CreateDXGIFactory1 == nullptr) {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        if (CreateDXGIFactory2 == nullptr) {
            return false;
        }
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");

        static HMODULE d3d12Lib = LoadLibraryA("d3d11.dll");
        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d12Lib, "D3D11CreateDevice");
#endif

        available = true;
        return true;
    }

    D3D11GraphicsDevice::D3D11GraphicsDevice()
    {
        ALIMER_VERIFY(IsAvailable());

        CreateFactory();

        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(GRAPHICS_DEBUG)
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

        // Get adapter and create device
        {
            ComPtr<IDXGIAdapter1> adapter;
            GetAdapter(adapter.GetAddressOf());

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
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;

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
                    device.GetAddressOf(),
                    &d3dFeatureLevel,
                    context.GetAddressOf()
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
                    device.GetAddressOf(),
                    &d3dFeatureLevel,
                    context.GetAddressOf()
                );

                if (SUCCEEDED(hr))
                {
                    OutputDebugStringA("Direct3D Adapter - WARP\n");
                }
            }
#endif

            ThrowIfFailed(hr);


#ifndef NDEBUG
            ComPtr<ID3D11Debug> d3dDebug;
            if (SUCCEEDED(device.As(&d3dDebug)))
            {
                ComPtr<ID3D11InfoQueue> d3dInfoQueue;
                if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
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
            }
#endif

            ThrowIfFailed(device.As(&d3dDevice));
            ThrowIfFailed(context.As(&d3dContext));
            ///ThrowIfFailed(context.As(&m_d3dAnnotation));
        }
    }

    D3D11GraphicsDevice::~D3D11GraphicsDevice()
    {
        BackendShutdown();
    }

    void D3D11GraphicsDevice::BackendShutdown()
    {
        d3dContext.Reset();
        ULONG refCount = d3dDevice.Reset();
#ifdef _DEBUG
        if (refCount > 0)
        {
            LOG_DEBUG("Direct3D11: There are {} unreleased references left on the device", refCount);

            /*ComPtr<ID3D11Debug> d3dDebug;
            if (SUCCEEDED(d3dDevice.As(&d3dDebug)))
            {
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY);
            }*/
        }
#else
        (void)refCount; // avoid warning
#endif
    }

    void D3D11GraphicsDevice::CreateFactory()
    {
#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
        bool debugDXGI = false;
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
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
        }
    }

    void D3D11GraphicsDevice::GetAdapter(IDXGIAdapter1** ppAdapter, bool lowPower)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> factory6;
        HRESULT hr = dxgiFactory.As(&factory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (lowPower) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (UINT adapterIndex = 0;
                SUCCEEDED(factory6->EnumAdapterByGpuPreference(
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
#endif

        if (!adapter)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf())); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

                break;
            }
        }

        if (!adapter)
        {
            LOG_ERROR("No Direct3D 12 device found");
        }

        *ppAdapter = adapter.Detach();
    }

    void D3D11GraphicsDevice::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        // Init capabilities
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

            caps.backendType = BackendType::Direct3D11;
            caps.vendorId = static_cast<GPUVendorId>(desc.VendorId);
            caps.deviceId = desc.DeviceId;

            std::wstring deviceName(desc.Description);
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
    }

    bool D3D11GraphicsDevice::BeginFrameImpl()
    {
        return true;
    }

    void D3D11GraphicsDevice::EndFrameImpl()
    {

    }

    RefPtr<SwapChain> D3D11GraphicsDevice::CreateSwapChain(const SwapChainDescription& desc)
    {
        return new D3D11SwapChain(this, desc);
    }

    RefPtr<Texture> D3D11GraphicsDevice::CreateTexture(const TextureDescription& desc, const void* initialData)
    {
        return new D3D11Texture(this, desc, initialData);
    }

    CommandList D3D11GraphicsDevice::BeginCommandList(const char* name)
    {
        CommandList cmd = 0;
        return cmd;
    }

    void D3D11GraphicsDevice::InsertDebugMarker(CommandList commandList, const char* name)
    {
        //auto wideName = ToUtf16(name, strlen(name));
        //PIXSetMarker(commandLists[commandList], PIX_COLOR_DEFAULT, wideName.c_str());
    }

    void D3D11GraphicsDevice::PushDebugGroup(CommandList commandList, const char* name)
    {
        //auto wideName = ToUtf16(name, strlen(name));
        //PIXBeginEvent(commandLists[commandList], PIX_COLOR_DEFAULT, wideName.c_str());
    }

    void D3D11GraphicsDevice::PopDebugGroup(CommandList commandList)
    {
        //PIXEndEvent(commandLists[commandList]);
    }

    void D3D11GraphicsDevice::SetScissorRect(CommandList commandList, const Rect& scissorRect)
    {
        D3D11_RECT d3dScissorRect;
        d3dScissorRect.left = LONG(scissorRect.x);
        d3dScissorRect.top = LONG(scissorRect.y);
        d3dScissorRect.right = LONG(scissorRect.x + scissorRect.width);
        d3dScissorRect.bottom = LONG(scissorRect.y + scissorRect.height);
        //commandLists[commandList]->RSSetScissorRects(1, &d3dScissorRect);
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
        //commandLists[commandList]->RSSetScissorRects(count, d3dScissorRects);
    }

    void D3D11GraphicsDevice::SetViewport(CommandList commandList, const Viewport& viewport)
    {
        //commandLists[commandList]->RSSetViewports(1, reinterpret_cast<const D3D11_VIEWPORT*>(&viewport));
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
        //commandLists[commandList]->RSSetViewports(count, d3dViewports);
    }

    void D3D11GraphicsDevice::SetBlendColor(CommandList commandList, const Color& color)
    {
        //commandLists[commandList]->OMSetBlendFactor(color.Data());
    }

    void D3D11GraphicsDevice::BindBuffer(CommandList commandList, uint32_t slot, GraphicsBuffer* buffer)
    {
        D3D11Buffer* d3dBuffer = static_cast<D3D11Buffer*>(buffer);
    }

    void D3D11GraphicsDevice::BindBufferData(CommandList commandList, uint32_t slot, const void* data, uint32_t size)
    {
    }

    void D3D11GraphicsDevice::HandleDeviceLost()
    {

    }
}
