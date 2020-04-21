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

#include "D3D11GPUDevice.h"
#include "D3D11GPUProvider.h"
#include "D3D11GPUAdapter.h"
#include "D3D11SwapChain.h"
#include "core/String.h"

namespace alimer
{
#if defined(_DEBUG)
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable()
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

    D3D11GPUDevice::D3D11GPUDevice(D3D11GPUProvider* provider, D3D11GPUAdapter* adapter)
        : GPUDevice(provider, adapter)
        , dxgiFactory(provider->GetDXGIFactory())
        , isTearingSupported(provider->IsTearingSupported())
        , _validation(provider->IsValidationEnabled())
    {
        CreateDeviceResources();
    }

    D3D11GPUDevice::~D3D11GPUDevice()
    {
        WaitForIdle();
        Shutdown();
    }

    void D3D11GPUDevice::Shutdown()
    {
        d3dContext.Reset();
        //d3dAnnotation.Reset();

        ReleaseTrackedResources();

#ifdef _DEBUG
        {
            ComPtr<ID3D11Debug> d3dDebug;
            if (SUCCEEDED(d3dDevice.As(&d3dDebug)))
            {
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY);
            }
        }
#endif

        d3dDevice.Reset();
    }

    void D3D11GPUDevice::CreateDeviceResources()
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (_validation)
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
#endif

        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        // Create the Direct3D 11 API device object and a corresponding context.
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;

        auto dxgiAdapter = static_cast<D3D11GPUAdapter*>(_adapter.get())->GetHandle();

        HRESULT hr = D3D11CreateDevice(dxgiAdapter,
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            creationFlags,
            s_featureLevels,
            _countof(s_featureLevels),
            D3D11_SDK_VERSION,
            device.GetAddressOf(),  // Returns the Direct3D device created.
            &d3dFeatureLevel,     // Returns feature level of device created.
            context.GetAddressOf()  // Returns the device immediate context.
        );


        ThrowIfFailed(hr);

#ifndef NDEBUG
        if (_validation)
        {
            ComPtr<ID3D11Debug> d3dDebug;
            if (SUCCEEDED(device.As(&d3dDebug)))
            {
                ComPtr<ID3D11InfoQueue> d3dInfoQueue;
                if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
                {
#ifdef _DEBUG
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, false);
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
        }
#endif

        ThrowIfFailed(device.As(&d3dDevice));
        ThrowIfFailed(context.As(&d3dContext));
        ///ThrowIfFailed(context.As(&d3dAnnotation));

        // Init caps and features.
        InitCapabilities();
    }

    void D3D11GPUDevice::InitCapabilities()
    {
        _features.independentBlend = true;
        _features.computeShader = true;
        _features.geometryShader = true;
        _features.tessellationShader = true;
        _features.multiViewport = true;
        _features.fullDrawIndexUint32 = true;
        _features.multiDrawIndirect = true;
        _features.fillModeNonSolid = true;
        _features.samplerAnisotropy = true;
        _features.textureCompressionETC2 = false;
        _features.textureCompressionASTC_LDR = false;
        _features.textureCompressionBC = true;
        _features.textureCubeArray = true;
        _features.raytracing = false;

        // Limits
        _limits.maxVertexAttributes = kMaxVertexAttributes;
        _limits.maxVertexBindings = kMaxVertexAttributes;
        _limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
        _limits.maxVertexBindingStride = kMaxVertexBufferStride;

        //caps.limits.maxTextureDimension1D = D3D11_REQ_TEXTURE1D_U_DIMENSION;
        _limits.maxTextureDimension2D = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        _limits.maxTextureDimension3D = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        _limits.maxTextureDimensionCube = D3D11_REQ_TEXTURECUBE_DIMENSION;
        _limits.maxTextureArrayLayers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        _limits.maxColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
        _limits.maxUniformBufferSize = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        _limits.minUniformBufferOffsetAlignment = 256u;
        _limits.maxStorageBufferSize = UINT32_MAX;
        _limits.minStorageBufferOffsetAlignment = 16;
        _limits.maxSamplerAnisotropy = D3D11_MAX_MAXANISOTROPY;
        _limits.maxViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        _limits.maxViewportWidth = D3D11_VIEWPORT_BOUNDS_MAX;
        _limits.maxViewportHeight = D3D11_VIEWPORT_BOUNDS_MAX;
        _limits.maxTessellationPatchSize = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        _limits.pointSizeRangeMin = 1.0f;
        _limits.pointSizeRangeMax = 1.0f;
        _limits.lineWidthRangeMin = 1.0f;
        _limits.lineWidthRangeMax = 1.0f;
        _limits.maxComputeSharedMemorySize = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        _limits.maxComputeWorkGroupCountX = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        _limits.maxComputeWorkGroupCountY = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        _limits.maxComputeWorkGroupCountZ = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        _limits.maxComputeWorkGroupInvocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        _limits.maxComputeWorkGroupSizeX = D3D11_CS_THREAD_GROUP_MAX_X;
        _limits.maxComputeWorkGroupSizeY = D3D11_CS_THREAD_GROUP_MAX_Y;
        _limits.maxComputeWorkGroupSizeZ = D3D11_CS_THREAD_GROUP_MAX_Z;

        /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
        /*UINT dxgi_fmt_caps = 0;
        for (int fmt = (VGPUTextureFormat_Undefined + 1); fmt < VGPUTextureFormat_Count; fmt++)
        {
            DXGI_FORMAT dxgi_fmt = _vgpu_d3d_get_format((VGPUTextureFormat)fmt);
            HRESULT hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, dxgi_fmt, &dxgi_fmt_caps);
            VGPU_ASSERT(SUCCEEDED(hr));
            sg_pixelformat_info* info = &_sg.formats[fmt];
            info->sample = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D);
            info->filter = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
            info->render = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
            info->blend = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE);
            info->msaa = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);
            info->depth = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
            if (info->depth) {
                info->render = true;
            }
        }*/
    }

    void D3D11GPUDevice::WaitForIdle()
    {
        d3dContext->Flush();
    }

    SwapChain* D3D11GPUDevice::CreateSwapChainCore(const SwapChainDescriptor* descriptor)
    {
        return new D3D11SwapChain(this, descriptor);
    }

    /*void D3D11GraphicsDevice::Present(const std::vector<Swapchain*>& swapchains)
    {
        // Present swap chains.
        HRESULT hr = S_OK;
        for (size_t i = 0, count = swapchains.size(); i < count; i++)
        {
            D3D11Swapchain* swapchain = static_cast<D3D11Swapchain*>(swapchains[i]);
            hr = swapchain->Present();

            if (hr == DXGI_ERROR_DEVICE_REMOVED
                || hr == DXGI_ERROR_DEVICE_HUNG
                || hr == DXGI_ERROR_DEVICE_RESET
                || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
                || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            {
                isLost = true;
                return;
            }

            ThrowIfFailed(hr);
        }
    }*/
}
