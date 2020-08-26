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
#include "D3D11CommandBuffer.h"
#include "D3D11GraphicsDevice.h"
#include "D3D11SwapChain.h"
#include "D3D11GPUBuffer.h"
#include "D3D11Texture.h"
#include "Core/String.h"

using Microsoft::WRL::ComPtr;

namespace Alimer
{
    D3D11GraphicsDevice::D3D11GraphicsDevice(Window* window, const GraphicsDeviceSettings& settings)
        : GraphicsDevice(window, GPUBackendType::D3D11)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        dxgiDLL = LoadLibraryW(L"dxgi.dll");
        d3d11DLL = LoadLibraryW(L"d3d11.dll");

        DXGIGetDebugInterface1 = reinterpret_cast<PFN_DXGI_GET_DEBUG_INTERFACE1>(GetProcAddress(dxgiDLL, "DXGIGetDebugInterface1"));
        CreateDXGIFactory1 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(dxgiDLL, "CreateDXGIFactory1"));
        CreateDXGIFactory2 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(dxgiDLL, "CreateDXGIFactory2"));
        D3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(GetProcAddress(d3d11DLL, "D3D11CreateDevice"));
#endif

        CreateFactory();

        // Get adapter and create device.
        {
            ComPtr<IDXGIAdapter1> dxgiAdapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
            ComPtr<IDXGIFactory6> dxgiFactory6;
            HRESULT hr = dxgiFactory.As(&dxgiFactory6);
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
            if (IsSdkLayersAvailable())
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
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;

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

            ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&d3dDevice)));

            // Create main context.
            ThrowIfFailed(context.As(&immediateContext));
            ThrowIfFailed(context.As(&d3dAnnotation));

            // Init adapter.
            adapter = new D3D11GPUAdapter(dxgiAdapter);

            // Init caps
            InitCapabilities();
        }

        // Create SwapChain.
        swapChain = new D3D11SwapChain(this, window, settings.colorFormatSrgb, settings.verticalSync);
    }

    D3D11GraphicsDevice::~D3D11GraphicsDevice()
    {
        Shutdown();

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        FreeLibrary(dxgiDLL);
        FreeLibrary(d3d11DLL);
#endif
    }

    void D3D11GraphicsDevice::Shutdown()
    {
        SafeDelete(swapChain);
        d3dAnnotation.Reset();
        immediateContext.Reset();
        cmdBuffersPool.clear();

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
    }

    GPUAdapter* D3D11GraphicsDevice::GetAdapter() const
    {
        return adapter;
    }

    void D3D11GraphicsDevice::CreateFactory()
    {
#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
        bool debugDXGI = false;
        {
            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
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

#if defined(_DEBUG)
    bool D3D11GraphicsDevice::IsSdkLayersAvailable() noexcept
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

    void D3D11GraphicsDevice::InitCapabilities()
    {
        D3D11_FEATURE_DATA_THREADING threadingSupport = { 0 };
        ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingSupport, sizeof(threadingSupport)));

        // Features
        features.independentBlend = true;
        features.computeShader = true;
        features.geometryShader = true;
        features.tessellationShader = true;
        features.logicOp = true;
        features.multiViewport = true;
        features.fullDrawIndexUint32 = true;
        features.multiDrawIndirect = true;
        features.fillModeNonSolid = true;
        features.samplerAnisotropy = true;
        features.textureCompressionETC2 = false;
        features.textureCompressionASTC_LDR = false;
        features.textureCompressionBC = true;
        features.textureCubeArray = true;
        features.raytracing = false;

        // Limits
        limits.maxVertexAttributes = kMaxVertexAttributes;
        limits.maxVertexBindings = kMaxVertexAttributes;
        limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
        limits.maxVertexBindingStride = kMaxVertexBufferStride;

        //caps.limits.maxTextureDimension1D = D3D11_REQ_TEXTURE1D_U_DIMENSION;
        limits.maxTextureDimension2D = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        limits.maxTextureDimension3D = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        limits.maxTextureDimensionCube = D3D11_REQ_TEXTURECUBE_DIMENSION;
        limits.maxTextureArrayLayers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        limits.maxColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
        limits.maxUniformBufferSize = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        limits.minUniformBufferOffsetAlignment = 256u;
        limits.maxStorageBufferSize = UINT32_MAX;
        limits.minStorageBufferOffsetAlignment = 16;
        limits.maxSamplerAnisotropy = D3D11_MAX_MAXANISOTROPY;
        limits.maxViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        if (limits.maxViewports > kMaxViewportAndScissorRects) {
            limits.maxViewports = kMaxViewportAndScissorRects;
        }

        limits.maxViewportWidth = D3D11_VIEWPORT_BOUNDS_MAX;
        limits.maxViewportHeight = D3D11_VIEWPORT_BOUNDS_MAX;
        limits.maxTessellationPatchSize = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        limits.pointSizeRangeMin = 1.0f;
        limits.pointSizeRangeMax = 1.0f;
        limits.lineWidthRangeMin = 1.0f;
        limits.lineWidthRangeMax = 1.0f;
        limits.maxComputeSharedMemorySize = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        limits.maxComputeWorkGroupCountX = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        limits.maxComputeWorkGroupCountY = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        limits.maxComputeWorkGroupCountZ = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        limits.maxComputeWorkGroupInvocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        limits.maxComputeWorkGroupSizeX = D3D11_CS_THREAD_GROUP_MAX_X;
        limits.maxComputeWorkGroupSizeY = D3D11_CS_THREAD_GROUP_MAX_Y;
        limits.maxComputeWorkGroupSizeZ = D3D11_CS_THREAD_GROUP_MAX_Z;

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
        SubmitCommandBuffers();

        HRESULT  hr = swapChain->Present();

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
            HandleDeviceLost();
        }
        else
        {
            ThrowIfFailed(hr);

            if (!dxgiFactory->IsCurrent())
            {
                // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
                CreateFactory();
            }
        }
    }

    void D3D11GraphicsDevice::HandleDeviceLost()
    {
        isLost = true;
    }

    Texture* D3D11GraphicsDevice::GetBackbufferTexture() const
    {
        return swapChain->GetColorTexture();
    }

    CommandBuffer* D3D11GraphicsDevice::RequestCommandBufferCore(const char* name, bool profile)
    {
        std::lock_guard<std::mutex> LockGuard(cmdBuffersAllocationMutex);

        D3D11CommandBuffer* commandBuffer = nullptr;
        if (availableCommandBuffers.empty())
        {
            //commandBuffer = new D3D11CommandBuffer(this, MEGABYTES(8));
            commandBuffer = new D3D11ContextCommandBuffer(this);
            cmdBuffersPool.emplace_back(commandBuffer);
        }
        else
        {
            commandBuffer = availableCommandBuffers.front();
            availableCommandBuffers.pop();
            commandBuffer->Reset();
        }
        ALIMER_ASSERT(commandBuffer != nullptr);

        return commandBuffer;
    }

    void D3D11GraphicsDevice::CommitCommandBuffer(D3D11CommandBuffer* commandBuffer)
    {
        commitCommandBuffers.push(commandBuffer);
    }

    void D3D11GraphicsDevice::SubmitCommandBuffer(D3D11CommandBuffer* commandBuffer)
    {
        ALIMER_ASSERT(commandBuffer != nullptr);
        std::lock_guard<std::mutex> LockGuard(cmdBuffersAllocationMutex);
        commandBuffer->Execute(immediateContext.Get(), d3dAnnotation.Get());
        availableCommandBuffers.push(commandBuffer);
    }

    void D3D11GraphicsDevice::SubmitCommandBuffers()
    {
        D3D11CommandBuffer* commandBuffer = nullptr;

        while (!commitCommandBuffers.empty())
        {
            commandBuffer = commitCommandBuffers.front();
            SubmitCommandBuffer(commandBuffer);
            commitCommandBuffers.pop();
        }

        immediateContext->ClearState();
    }
}
