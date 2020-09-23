//
// Copyright (c) 2019-2020 Amer Koleci.
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

#if defined(AGPU_DRIVER_D3D12)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#include <d3d12.h>
#include "D3D12MemAlloc.h"
#include "agpu_driver_d3d_common.h"
//#include "stb_ds.h"
#include <mutex>

namespace agpu
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    static PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
    static PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
    static PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
    static PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
#endif

    struct D3D12SwapChain {
        uint32_t width;
        uint32_t height;
        PixelFormat colorFormat;

        uint32_t syncInterval;
        uint32_t presentFlags;
        // HDR Support
        DXGI_COLOR_SPACE_TYPE colorSpace;

        IDXGISwapChain3* handle;
    };

    struct D3D12Buffer
    {
        enum { MAX_COUNT = 4096 };

        ID3D12Resource* handle;
    };

    struct D3D12Texture
    {
        enum { MAX_COUNT = 4096 };

        ID3D12Resource* handle;
    };

    struct D3D12RenderPass
    {
        enum { MAX_COUNT = 512 };

        uint32_t rtvsCount;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[kMaxColorAttachments];
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsv;
    };

    /* Global data */
    static struct {
        bool available_initialized;
        bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HMODULE dxgiDLL;
        HMODULE d3d12DLL;
#endif

        bool debug;
        bool GPUBasedValidation;

        DWORD dxgiFactoryFlags;
        IDXGIFactory4* factory;
        DXGIFactoryCaps factoryCaps;

        D3D_FEATURE_LEVEL minFeatureLevel;

        ID3D12Device* device;
        D3D_FEATURE_LEVEL feature_level;
        bool is_lost;

        Caps caps;

        D3D12SwapChain swapChain;

        std::mutex handle_mutex{};
        Pool<D3D12Buffer, D3D12Buffer::MAX_COUNT> buffers;
        Pool<D3D12Texture, D3D12Texture::MAX_COUNT> textures;
        Pool<D3D12RenderPass, D3D12RenderPass::MAX_COUNT> renderPasses;
    } d3d12;

    /* Device/Renderer */
    static bool d3d12_CreateFactory(void)
    {
        SAFE_RELEASE(d3d12.factory);

        HRESULT hr = S_OK;

#if defined(_DEBUG)
        if (d3d12.debug)
        {
            ID3D12Debug* debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                ID3D12Debug1* debugController1 = nullptr;
                if (SUCCEEDED(debugController->QueryInterface(&debugController1)))
                {
                    debugController1->SetEnableGPUBasedValidation(d3d12.GPUBasedValidation);
                }

                SAFE_RELEASE(debugController);
                SAFE_RELEASE(debugController1);
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                d3d12.dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
                VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
                VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
                };

                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(D3D_DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }

#endif
        hr = CreateDXGIFactory2(d3d12.dxgiFactoryFlags, IID_PPV_ARGS(&d3d12.factory));
        if (FAILED(hr)) {
            return false;
        }

        d3d12.factoryCaps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;

        // Check tearing support.
        {
            BOOL allowTearing = FALSE;
            IDXGIFactory5* factory5 = nullptr;
            HRESULT hr = d3d12.factory->QueryInterface(IID_PPV_ARGS(&factory5));
            if (SUCCEEDED(hr))
            {
                hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            SAFE_RELEASE(factory5);

            if (FAILED(hr) || !allowTearing)
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                d3d12.factoryCaps |= DXGIFactoryCaps::Tearing;
            }
        }

        return true;
    }

    static IDXGIAdapter1* d3d12_GetAdapter(bool lowPower)
    {
        /* Detect adapter now. */
        IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* factory6 = nullptr;
        HRESULT hr = d3d12.factory->QueryInterface(IID_PPV_ARGS(&factory6));
        if (SUCCEEDED(hr))
        {
            // By default prefer high performance
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (lowPower) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (uint32_t i = 0;
                DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(&adapter)); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter, d3d12.minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", i, desc.VendorId, desc.DeviceId, desc.Description);
                    OutputDebugStringW(buff);
#endif
                    break;
                }
            }
        }

        SAFE_RELEASE(factory6);
#endif

        if (!adapter)
        {
            for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != d3d12.factory->EnumAdapters1(i, &adapter); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter, d3d12.minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", i, desc.VendorId, desc.DeviceId, desc.Description);
                    OutputDebugStringW(buff);
#endif
                    break;
                }
            }
        }


#if !defined(NDEBUG)
        if (!adapter)
        {
            // Try WARP12 instead
            if (FAILED(d3d12.factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
            {
                logError("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            logError("No Direct3D 12 device found");
        }

        return adapter;
    }

    /* SwapChain functions. */
    static bool d3d12_UpdateSwapChain(D3D12SwapChain* swapChain, const PresentationParameters* presentationParameters);
    static void d3d12_DestroySwapChain(D3D12SwapChain* swapChain);

    static bool d3d12_init(InitFlags flags, const PresentationParameters* presentationParameters)
    {
        d3d12.debug = any(flags & InitFlags::DebugRuntime) || any(flags & InitFlags::GPUBasedValidation);
        d3d12.GPUBasedValidation = any(flags & InitFlags::GPUBasedValidation);
        d3d12.dxgiFactoryFlags = 0;
        d3d12.minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        if (!d3d12_CreateFactory()) {
            return false;
        }

        const bool lowPower = any(flags & InitFlags::LowPowerGPUPreference);
        IDXGIAdapter1* dxgi_adapter = d3d12_GetAdapter(lowPower);

        /* Init caps first. */
        {
            DXGI_ADAPTER_DESC1 adapter_desc;
            VHR(dxgi_adapter->GetDesc1(&adapter_desc));

            /* Log some info */
            logInfo("GPU driver: D3D12");
            logInfo("Direct3D Adapter: VID:%04X, PID:%04X - %ls", adapter_desc.VendorId, adapter_desc.DeviceId, adapter_desc.Description);

            d3d12.caps.backend = BackendType::Direct3D12;
            d3d12.caps.vendorID = adapter_desc.VendorId;
            d3d12.caps.deviceID = adapter_desc.DeviceId;

            d3d12.caps.features.independent_blend = true;
            d3d12.caps.features.compute_shader = true;
            d3d12.caps.features.tessellation_shader = true;
            d3d12.caps.features.multi_viewport = true;
            d3d12.caps.features.index_uint32 = true;
            d3d12.caps.features.multi_draw_indirect = true;
            d3d12.caps.features.fill_mode_non_solid = true;
            d3d12.caps.features.sampler_anisotropy = true;
            d3d12.caps.features.texture_compression_ETC2 = false;
            d3d12.caps.features.texture_compression_ASTC_LDR = false;
            d3d12.caps.features.texture_compression_BC = true;
            d3d12.caps.features.texture_cube_array = true;
            d3d12.caps.features.raytracing = false;

            // Limits
            d3d12.caps.limits.max_vertex_attributes = kMaxVertexAttributes;
            d3d12.caps.limits.max_vertex_bindings = kMaxVertexAttributes;
            d3d12.caps.limits.max_vertex_attribute_offset = kMaxVertexAttributeOffset;
            d3d12.caps.limits.max_vertex_binding_stride = kMaxVertexBufferStride;

            d3d12.caps.limits.maxTextureDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
            d3d12.caps.limits.maxTextureDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            d3d12.caps.limits.maxTextureDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            d3d12.caps.limits.maxTextureDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
            d3d12.caps.limits.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            d3d12.caps.limits.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
            d3d12.caps.limits.max_uniform_buffer_size = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
            d3d12.caps.limits.min_uniform_buffer_offset_alignment = 256u;
            d3d12.caps.limits.max_storage_buffer_size = UINT32_MAX;
            d3d12.caps.limits.min_storage_buffer_offset_alignment = 16;
            d3d12.caps.limits.max_sampler_anisotropy = D3D12_MAX_MAXANISOTROPY;
            d3d12.caps.limits.max_viewports = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            d3d12.caps.limits.max_viewport_width = D3D12_VIEWPORT_BOUNDS_MAX;
            d3d12.caps.limits.max_viewport_height = D3D12_VIEWPORT_BOUNDS_MAX;
            d3d12.caps.limits.max_tessellation_patch_size = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
            d3d12.caps.limits.point_size_range_min = 1.0f;
            d3d12.caps.limits.point_size_range_max = 1.0f;
            d3d12.caps.limits.line_width_range_min = 1.0f;
            d3d12.caps.limits.line_width_range_max = 1.0f;
            d3d12.caps.limits.max_compute_shared_memory_size = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
            d3d12.caps.limits.max_compute_work_group_count_x = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            d3d12.caps.limits.max_compute_work_group_count_y = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            d3d12.caps.limits.max_compute_work_group_count_z = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            d3d12.caps.limits.max_compute_work_group_invocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
            d3d12.caps.limits.max_compute_work_group_size_x = D3D12_CS_THREAD_GROUP_MAX_X;
            d3d12.caps.limits.max_compute_work_group_size_y = D3D12_CS_THREAD_GROUP_MAX_Y;
            d3d12.caps.limits.max_compute_work_group_size_z = D3D12_CS_THREAD_GROUP_MAX_Z;

#if TODO
            /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
            UINT dxgi_fmt_caps = 0;
            for (int fmt = (VGPUTextureFormat_Undefined + 1); fmt < VGPUTextureFormat_Count; fmt++)
            {
                DXGI_FORMAT dxgi_fmt = _vgpu_d3d_get_format((VGPUTextureFormat)fmt);
                HRESULT hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, dxgi_fmt, &dxgi_fmt_caps);
                VGPU_ASSERT(SUCCEEDED(hr));
                /*sg_pixelformat_info* info = &_sg.formats[fmt];
                info->sample = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D);
                info->filter = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
                info->render = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
                info->blend = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE);
                info->msaa = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);
                info->depth = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
                if (info->depth) {
                    info->render = true;
                }*/
            }
#endif // TODO

        }

        /* Release adapter */
        dxgi_adapter->Release();

        /* Create swap chain if required. */
        if (presentationParameters != nullptr)
        {
            if (!d3d12_UpdateSwapChain(&d3d12.swapChain, presentationParameters))
            {
                return false;
            }
        }

        /* Init pools*/
        d3d12.buffers.init();
        d3d12.textures.init();
        d3d12.renderPasses.init();

        return true;
        }

    static void d3d12_shutdown(void)
    {
        if (d3d12.swapChain.handle)
        {
            d3d12_DestroySwapChain(&d3d12.swapChain);
        }

        ULONG refCount = d3d12.device->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            logError("Direct3D11: There are %d unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice = nullptr;
            if (SUCCEEDED(d3d12.device->QueryInterface(IID_PPV_ARGS(&debugDevice))))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        SAFE_RELEASE(d3d12.factory);

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug1;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
        {
            dxgiDebug1->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug1->Release();
        }
#endif
    }

    static void d3d12_update_color_space(D3D12SwapChain* context)
    {
        context->colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

        bool isDisplayHDR10 = false;

#if defined(NTDDI_WIN10_RS2)
        IDXGIOutput* output;
        if (SUCCEEDED(context->handle->GetContainingOutput(&output)))
        {
            IDXGIOutput6* output6;
            if (SUCCEEDED(output->QueryInterface(IID_PPV_ARGS(&output6))))
            {
                DXGI_OUTPUT_DESC1 desc;
                VHR(output6->GetDesc1(&desc));

                if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                {
                    // Display output is HDR10.
                    isDisplayHDR10 = true;
                }

                output6->Release();
            }

            output->Release();
        }
#endif

        if (isDisplayHDR10)
        {
            switch (context->colorFormat)
            {
            case PixelFormat::RGBA16Unorm:
                // The application creates the HDR10 signal.
                context->colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                break;

            case PixelFormat::RGBA32Float:
                // The system creates the HDR10 signal; application uses linear values.
                context->colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
                break;

            default:
                break;
            }
        }

        IDXGISwapChain3* swapChain3;
        if (SUCCEEDED(context->handle->QueryInterface(IID_PPV_ARGS(&swapChain3))))
        {
            UINT colorSpaceSupport = 0;
            if (SUCCEEDED(swapChain3->CheckColorSpaceSupport(context->colorSpace, &colorSpaceSupport))
                && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
            {
                VHR(swapChain3->SetColorSpace1(context->colorSpace));
            }

            swapChain3->Release();
        }
    }

    static void d3d11_AfterReset(D3D12SwapChain* swapChain)
    {
        d3d12_update_color_space(swapChain);

        DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
        VHR(swapChain->handle->GetDesc1(&swapchain_desc));
        swapChain->width = swapchain_desc.Width;
        swapChain->height = swapchain_desc.Height;

        /* TODO: Until we support textures. */
        // ID3D11Texture2D* backbuffer;
        //VHR(swapChain->handle->GetBuffer(0, IID_PPV_ARGS(&backbuffer)));

        //ID3D11Device1_CreateRenderTargetView(d3d11.device, (ID3D11Resource*)backbuffer, NULL, &context->rtv);
        //ID3D11Texture2D_Release(backbuffer);
    }

    static bool d3d12_UpdateSwapChain(D3D12SwapChain* swapChain, const PresentationParameters* presentationParameters)
    {
        const uint32_t backbufferCount = 2u;
        swapChain->colorFormat = presentationParameters->colorFormat;
        swapChain->colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

        /* TODO: Multisample */

        // Create swapchain if required.
        if (swapChain->handle == nullptr)
        {
            /* Setup sync interval and present flags. */
            swapChain->syncInterval = 1;
            swapChain->presentFlags = 0;
            if (!presentationParameters->enableVSync)
            {
                swapChain->syncInterval = 0;
                if (any(d3d12.factoryCaps & DXGIFactoryCaps::Tearing))
                {
                    swapChain->presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
                }
            }

            /*swapChain->handle = d3dCreateSwapChain(
                d3d12.factory, d3d12.factoryCaps, d3d12.device,
                presentationParameters->windowHandle,
                ToDXGISwapChainFormat(presentationParameters->colorFormat),
                presentationParameters->backBufferWidth, presentationParameters->backBufferHeight,
                backbufferCount,
                presentationParameters->isFullscreen);*/
        }
        else
        {
            // TODO: Resize.
        }

        d3d11_AfterReset(swapChain);

        return true;
    }

    static void d3d12_DestroySwapChain(D3D12SwapChain* swapChain)
    {
        SAFE_RELEASE(swapChain->handle);
    }

    static void d3d12_resize(uint32_t width, uint32_t height)
    {

    }

    static bool d3d12_beginFrame(void) {
        float clear_color[4] = { 0.392156899f, 0.584313750f, 0.929411829f, 1.0f };

        //ID3D11DeviceContext1_ClearRenderTargetView(d3d11.context, d3d11_current_context->rtv, clear_color);
        //ID3D11DeviceContext1_ClearDepthStencilView(d3d11.context, d3d11_current_context->dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        //ID3D11DeviceContext1_OMSetRenderTargets(d3d11.context, 1, &d3d11_current_context->rtv, d3d11_current_context->dsv);

        // Set the viewport.
        //auto viewport = m_deviceResources->GetScreenViewport();
        //context->RSSetViewports(1, &viewport);

        return true;
    }

    static void d3d12_endFrame(void)
    {
        if (d3d12.swapChain.handle != nullptr)
        {
            HRESULT hr = d3d12.swapChain.handle->Present(d3d12.swapChain.syncInterval, d3d12.swapChain.presentFlags);
            if (hr == DXGI_ERROR_DEVICE_REMOVED
                || hr == DXGI_ERROR_DEVICE_HUNG
                || hr == DXGI_ERROR_DEVICE_RESET
                || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
                || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            {
                d3d12.is_lost = true;
            }
        }

        if (!d3d12.is_lost)
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            if (!d3d12.factory->IsCurrent())
            {
                d3d12_CreateFactory();
            }
        }
    }

    static const Caps* d3d12_QueryCaps(void)
    {
        return &d3d12.caps;
    }

    static RenderPassHandle d3d12_CreateRenderPass(const PassDescription& desc)
    {
        std::lock_guard<std::mutex> lock(d3d12.handle_mutex);

        if (d3d12.renderPasses.isFull())
        {
            logError("D3D12: Not enough free render pass slots.");
            return kInvalidRenderPass;
        }

        const int id = d3d12.renderPasses.alloc();
        if (id < 0) {
            return kInvalidRenderPass;
        }

        D3D12RenderPass& renderPass = d3d12.renderPasses[id];
        renderPass = {};

        HRESULT hr = S_OK;
        for (uint32_t i = 0; i < kMaxColorAttachments; i++)
        {
            if (!desc.colorAttachments[i].texture.isValid())
                continue;

            D3D12Texture& texture = d3d12.textures[desc.colorAttachments[i].texture.value];

            /*hr = d3d12.device->CreateRenderTargetView(texture.handle, nullptr, &renderPass.rtvs[renderPass.rtvsCount]);
            if (FAILED(hr))
            {
                logError("Direct3D11: Failed to create RenderTargetView");
                d3d11.renderPasses.dealloc((uint32_t)id);
                return kInvalidRenderPass;
            }

            renderPass.rtvsCount++;*/
        }

        return { (uint32_t)id };
    }

    static void d3d12_DestroyRenderPass(RenderPassHandle handle)
    {

    }

    static BufferHandle d3d12_CreateBuffer(uint32_t count, uint32_t stride, const void* initialData)
    {
        std::lock_guard<std::mutex> lock(d3d12.handle_mutex);

        if (d3d12.buffers.isFull())
        {
            logError("D3D12: Not enough free buffer slots.");
            return kInvalidBuffer;
        }

        const int id = d3d12.buffers.alloc();
        if (id < 0) {
            return kInvalidBuffer;
        }

        D3D12Buffer& buffer = d3d12.buffers[id];
        buffer.handle = nullptr;
        /*HRESULT hr = d3d11.device->CreateBuffer(&bufferDesc, initialDataPtr, &buffer.handle);
        if (FAILED(hr))
        {
            logError("Direct3D11: Failed to create buffer");
            return kInvalidBuffer;
        }*/

        return { (uint32_t)id };
    }

    static void d3d12_DestroyBuffer(BufferHandle handle)
    {

    }

    static void d3d12_PushDebugGroup(const char* name)
    {
        wchar_t wName[128];
        if (StringConvert(name, wName) > 0)
        {
            //d3d11.annotation->BeginEvent(wName);
        }
    }

    static void d3d12_PopDebugGroup(void)
    {
        //d3d11.annotation->EndEvent();
    }

    static void d3d12_InsertDebugMarker(const char* name)
    {
        wchar_t wName[128];
        if (StringConvert(name, wName) > 0)
        {
            //d3d11.annotation->SetMarker(wName);
        }
    }

    /* Driver */
    static bool d3d12_IsSupported(void)
    {
        if (d3d12.available_initialized) {
            return d3d12.available;
        }

        d3d12.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        d3d12.dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!d3d12.dxgiDLL) {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d12.dxgiDLL, "CreateDXGIFactory2");
        if (!CreateDXGIFactory2)
        {
            return false;
        }

        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d12.dxgiDLL, "DXGIGetDebugInterface1");

        d3d12.d3d12DLL = LoadLibraryA("d3d12.dll");
        if (!d3d12.d3d12DLL) {
            return false;
        }

        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12.d3d12DLL, "D3D12GetDebugInterface");
        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12.d3d12DLL, "D3D12CreateDevice");
        if (!D3D12CreateDevice) {
            return false;
        }
#endif

        d3d12.available = true;
        return true;
    };

    static agpu_renderer* d3d12_CreateRenderer(void)
    {
        static agpu_renderer renderer = { 0 };
        ASSIGN_DRIVER(d3d12);
        return &renderer;
    }

    Driver D3D12_Driver = {
        BackendType::Direct3D12,
        d3d12_IsSupported,
        d3d12_CreateRenderer
    };
    }

#endif /* defined(AGPU_DRIVER_D3D11)  */
