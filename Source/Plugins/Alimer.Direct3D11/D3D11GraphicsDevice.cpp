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
#include "D3D11GraphicsProvider.h"
#include "D3D11GraphicsAdapter.h"
//#include "D3D11SwapChain.h"
#include "core/String.h"

namespace Alimer
{
    D3D11GraphicsDevice::D3D11GraphicsDevice(D3D11GraphicsProvider* provider, const std::shared_ptr<GraphicsAdapter>& adapter)
        : GraphicsDevice(adapter)
        , validation(provider->IsValidationEnabled())
        , dxgiFactory(provider->GetDXGIFactory())
        , isTearingSupported(provider->IsTearingSupported())
    {
        CreateDeviceResources();
    }

    D3D11GraphicsDevice::~D3D11GraphicsDevice()
    {
        Shutdown();
    }

    void D3D11GraphicsDevice::Shutdown()
    {
        SafeRelease(d3dContext);
        //d3dAnnotation.Reset();

        //ReleaseTrackedResources();

        ULONG refCount = d3dDevice->Release();
#ifdef _DEBUG
        if (refCount > 0)
        {
            LOG_WARN("Direct3D11: There are %d unreleased references left on the device", refCount);

            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(d3dDevice->QueryInterface(&d3dDebug)))
            {
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
                d3dDebug->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif
    }

    void D3D11GraphicsDevice::CreateDeviceResources()
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (validation)
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
        ID3D11Device* tempD3D11Device;
        ID3D11DeviceContext* tempD3D11Context;

        IDXGIAdapter1* dxgiAdapter = std::static_pointer_cast<D3D11GraphicsAdapter>(adapter)->GetDXGIAdapter();

        HRESULT hr = D3D11CreateDevice(dxgiAdapter,
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            creationFlags,
            s_featureLevels,
            _countof(s_featureLevels),
            D3D11_SDK_VERSION,
            &tempD3D11Device,
            &d3dFeatureLevel, 
            &tempD3D11Context
        );


        ThrowIfFailed(hr);

#ifndef NDEBUG
        if (validation)
        {
            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(tempD3D11Device->QueryInterface(&d3dDebug)))
            {
                ID3D11InfoQueue* d3dInfoQueue;
                if (SUCCEEDED(d3dDebug->QueryInterface(&d3dInfoQueue)))
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
                    d3dInfoQueue->Release();
                }

                d3dDebug->Release();
            }
        }
#endif

        ThrowIfFailed(tempD3D11Device->QueryInterface(&d3dDevice));
        ThrowIfFailed(tempD3D11Context->QueryInterface(&d3dContext));
        ///ThrowIfFailed(context.As(&d3dAnnotation));
        SafeRelease(tempD3D11Device);
        SafeRelease(tempD3D11Context);

        // Init caps and features.
        InitCapabilities();
    }

    void D3D11GraphicsDevice::InitCapabilities()
    {
        caps.features.independentBlend = true;
        caps.features.computeShader = true;
        caps.features.geometryShader = true;
        caps.features.tessellationShader = true;
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

    /*SwapChain* D3D11GPUDevice::CreateSwapChainCore(const SwapChainDescriptor* descriptor)
    {
        return new D3D11SwapChain(this, descriptor);
    }

    void D3D11GraphicsDevice::Present(const std::vector<Swapchain*>& swapchains)
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

