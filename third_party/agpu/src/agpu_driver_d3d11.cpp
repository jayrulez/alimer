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

#if defined(AGPU_DRIVER_D3D11)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#include <d3d11_3.h>
#include "agpu_driver_d3d_common.h"
//#include "stb_ds.h"
#include <mutex>

namespace agpu
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    static PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1 = nullptr;
    static PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
    static PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
    static PFN_D3D11_CREATE_DEVICE D3D11CreateDevice = nullptr;
#endif

#ifndef NDEBUG
    static const IID D3D11_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
#endif

    struct D3D11SwapChain {
        uint32_t width;
        uint32_t height;
        PixelFormat colorFormat;

        uint32_t syncInterval;
        uint32_t presentFlags;
        // HDR Support
        DXGI_COLOR_SPACE_TYPE colorSpace;

        IDXGISwapChain1* handle;
        ID3D11RenderTargetView* rtv;
        ID3D11DepthStencilView* dsv;
    };

    struct D3D11Buffer
    {
        enum { MAX_COUNT = 4096 };

        ID3D11Buffer* handle;
    };

    struct D3D11Texture
    {
        enum { MAX_COUNT = 4096 };

        union {
            ID3D11Resource* handle;
            ID3D11Texture2D* tex2D;
            ID3D11Texture3D* tex3D;
        };
    };

    struct D3D11RenderPass
    {
        enum { MAX_COUNT = 512 };

        uint32_t rtvsCount;
        ID3D11RenderTargetView* rtvs[kMaxColorAttachments];
        ID3D11DepthStencilView* dsv;
    };

    /* Global data */
    static struct {
        bool available_initialized;
        bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HMODULE dxgiDLL;
        HMODULE d3d11DLL;
#endif

        bool debug;

        IDXGIFactory2* factory;
        DXGIFactoryCaps factoryCaps;

        ID3D11Device1* device;
        ID3D11DeviceContext1* context;
        ID3DUserDefinedAnnotation* annotation;
        D3D_FEATURE_LEVEL feature_level;
        bool is_lost;

        Caps caps;

        D3D11SwapChain swapChain;

        std::mutex handle_mutex{};
        Pool<D3D11Buffer, D3D11Buffer::MAX_COUNT> buffers;
        Pool<D3D11Texture, D3D11Texture::MAX_COUNT> textures;
        Pool<D3D11RenderPass, D3D11RenderPass::MAX_COUNT> renderPasses;
    } d3d11;

    /* Device/Renderer */
    static bool _agpu_d3d11_sdk_layers_available() {
        HRESULT hr = D3D11CreateDevice(
            NULL,
            D3D_DRIVER_TYPE_NULL,
            NULL,
            D3D11_CREATE_DEVICE_DEBUG,
            NULL,
            0,
            D3D11_SDK_VERSION,
            NULL,
            NULL,
            NULL
        );

        return SUCCEEDED(hr);
    }

    static bool d3d11_CreateFactory(void)
    {
        SAFE_RELEASE(d3d11.factory);

        HRESULT hr = S_OK;

#if defined(_DEBUG)
        bool debugDXGI = false;

        if (d3d11.debug)
        {
            IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1 &&
                SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#endif
            {
                debugDXGI = true;
                hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&d3d11.factory));
                if (FAILED(hr)) {
                    return false;
                }

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

        if (!debugDXGI)
#endif
        {
            hr = CreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory));
            if (FAILED(hr)) {
                return false;
            }
        }

        d3d11.factoryCaps = DXGIFactoryCaps::None;

        // Disable FLIP if not on a supporting OS
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        {
            IDXGIFactory4* factory4;
            HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory4));
            if (SUCCEEDED(hr))
            {
                d3d11.factoryCaps |= DXGIFactoryCaps::FlipPresent;
            }
            else
            {
            }

            factory4->Release();
        }
#else
        d3d11.factoryCaps |= DXGIFactoryCaps::FlipPresent;
#endif

        // Check tearing support.
        {
            BOOL allowTearing = FALSE;
            IDXGIFactory5* factory5 = nullptr;
            HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory5));
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
                d3d11.factoryCaps |= DXGIFactoryCaps::Tearing;
            }
        }

        return true;
    }
    static IDXGIAdapter1* d3d11_get_adapter(bool lowPower)
    {
        /* Detect adapter now. */
        IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* factory6 = nullptr;
        HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory6));
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

                break;
            }
        }

        SAFE_RELEASE(factory6);
#endif

        if (!adapter)
        {
            for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != d3d11.factory->EnumAdapters1(i, &adapter); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

                break;
            }
        }

        return adapter;
    }

    /* SwapChain functions. */
    static bool d3d11_UpdateSwapChain(D3D11SwapChain* swapChain, const PresentationParameters* presentationParameters);
    static void d3d11_DestroySwapChain(D3D11SwapChain* swapChain);

    static bool d3d11_init(InitFlags flags, const PresentationParameters* presentationParameters)
    {
        d3d11.debug = any(flags & InitFlags::DebugRuntime) || any(flags & InitFlags::GPUBasedValidation);

        if (!d3d11_CreateFactory()) {
            return false;
        }

        const bool lowPower = any(flags & InitFlags::LowPowerGPUPreference);
        IDXGIAdapter1* dxgi_adapter = d3d11_get_adapter(lowPower);

        /* Create d3d11 device */
        {
            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

            if (d3d11.debug && _agpu_d3d11_sdk_layers_available())
            {
                // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
            }
#if defined(_DEBUG)
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }
#endif

            static const D3D_FEATURE_LEVEL s_featureLevels[] =
            {
                D3D_FEATURE_LEVEL_12_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0
            };

            // Create the Direct3D 11 API device object and a corresponding context.
            ID3D11Device* temp_d3d_device;
            ID3D11DeviceContext* temp_d3d_context;

            HRESULT hr = E_FAIL;
            if (dxgi_adapter)
            {
                hr = D3D11CreateDevice(
                    (IDXGIAdapter*)dxgi_adapter,
                    D3D_DRIVER_TYPE_UNKNOWN,
                    NULL,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &temp_d3d_device,
                    &d3d11.feature_level,
                    &temp_d3d_context
                );
            }
#if defined(NDEBUG)
            else
            {
                logError("No Direct3D hardware device found");
                AGPU_UNREACHABLE();
            }
#else
            if (FAILED(hr))
            {
                // If the initialization fails, fall back to the WARP device.
                // For more information on WARP, see:
                // http://go.microsoft.com/fwlink/?LinkId=286690
                hr = D3D11CreateDevice(
                    NULL,
                    D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                    NULL,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &temp_d3d_device,
                    &d3d11.feature_level,
                    &temp_d3d_context
                );

                if (SUCCEEDED(hr))
                {
                    OutputDebugStringA("Direct3D Adapter - WARP\n");
                }
            }
#endif

            if (FAILED(hr)) {
                return false;
            }

#ifndef NDEBUG
            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3dDebug))))
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

            VHR(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3d11.device)));
            VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.context)));
            VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.annotation)));
            temp_d3d_context->Release();
            temp_d3d_device->Release();
        }

        /* Init caps first. */
        {
            DXGI_ADAPTER_DESC1 adapter_desc;
            VHR(dxgi_adapter->GetDesc1(&adapter_desc));

            /* Log some info */
            logInfo("GPU driver: D3D11");
            logInfo("Direct3D Adapter: VID:%04X, PID:%04X - %ls", adapter_desc.VendorId, adapter_desc.DeviceId, adapter_desc.Description);

            d3d11.caps.backend = BackendType::Direct3D11;
            d3d11.caps.vendorID = adapter_desc.VendorId;
            d3d11.caps.deviceID = adapter_desc.DeviceId;

            d3d11.caps.features.independent_blend = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_0);
            d3d11.caps.features.compute_shader = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_0);
            d3d11.caps.features.tessellation_shader = (d3d11.feature_level >= D3D_FEATURE_LEVEL_11_0);
            d3d11.caps.features.multi_viewport = true;
            d3d11.caps.features.index_uint32 = true;
            d3d11.caps.features.multi_draw_indirect = (d3d11.feature_level >= D3D_FEATURE_LEVEL_11_0);
            d3d11.caps.features.fill_mode_non_solid = true;
            d3d11.caps.features.sampler_anisotropy = true;
            d3d11.caps.features.texture_compression_ETC2 = false;
            d3d11.caps.features.texture_compression_ASTC_LDR = false;
            d3d11.caps.features.texture_compression_BC = true;
            d3d11.caps.features.texture_cube_array = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_1);
            d3d11.caps.features.raytracing = false;

            // Limits
            d3d11.caps.limits.max_vertex_attributes = kMaxVertexAttributes;
            d3d11.caps.limits.max_vertex_bindings = kMaxVertexAttributes;
            d3d11.caps.limits.max_vertex_attribute_offset = kMaxVertexAttributeOffset;
            d3d11.caps.limits.max_vertex_binding_stride = kMaxVertexBufferStride;

            d3d11.caps.limits.maxTextureDimension1D = D3D11_REQ_TEXTURE1D_U_DIMENSION;
            d3d11.caps.limits.maxTextureDimension2D = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            d3d11.caps.limits.maxTextureDimension3D = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            d3d11.caps.limits.maxTextureDimensionCube = D3D11_REQ_TEXTURECUBE_DIMENSION;
            d3d11.caps.limits.maxTextureArrayLayers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            d3d11.caps.limits.maxColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
            d3d11.caps.limits.max_uniform_buffer_size = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
            d3d11.caps.limits.min_uniform_buffer_offset_alignment = 256u;
            d3d11.caps.limits.max_storage_buffer_size = UINT32_MAX;
            d3d11.caps.limits.min_storage_buffer_offset_alignment = 16;
            d3d11.caps.limits.max_sampler_anisotropy = D3D11_MAX_MAXANISOTROPY;
            d3d11.caps.limits.max_viewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            d3d11.caps.limits.max_viewport_width = D3D11_VIEWPORT_BOUNDS_MAX;
            d3d11.caps.limits.max_viewport_height = D3D11_VIEWPORT_BOUNDS_MAX;
            d3d11.caps.limits.max_tessellation_patch_size = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
            d3d11.caps.limits.point_size_range_min = 1.0f;
            d3d11.caps.limits.point_size_range_max = 1.0f;
            d3d11.caps.limits.line_width_range_min = 1.0f;
            d3d11.caps.limits.line_width_range_max = 1.0f;
            d3d11.caps.limits.max_compute_shared_memory_size = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
            d3d11.caps.limits.max_compute_work_group_count_x = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            d3d11.caps.limits.max_compute_work_group_count_y = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            d3d11.caps.limits.max_compute_work_group_count_z = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
            d3d11.caps.limits.max_compute_work_group_invocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
            d3d11.caps.limits.max_compute_work_group_size_x = D3D11_CS_THREAD_GROUP_MAX_X;
            d3d11.caps.limits.max_compute_work_group_size_y = D3D11_CS_THREAD_GROUP_MAX_Y;
            d3d11.caps.limits.max_compute_work_group_size_z = D3D11_CS_THREAD_GROUP_MAX_Z;

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
            if (!d3d11_UpdateSwapChain(&d3d11.swapChain, presentationParameters))
            {
                return false;
            }
        }

        /* Init pools*/
        d3d11.buffers.init();
        d3d11.textures.init();
        d3d11.renderPasses.init();

        return true;
    }

    static void d3d11_shutdown(void)
    {
        if (d3d11.swapChain.handle)
        {
            d3d11_DestroySwapChain(&d3d11.swapChain);
        }

        SAFE_RELEASE(d3d11.annotation);
        SAFE_RELEASE(d3d11.context);

        ULONG refCount = d3d11.device->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            logError("Direct3D11: There are %d unreleased references left on the device", refCount);

            ID3D11Debug* d3d11_debug = NULL;
            if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&d3d11_debug))))
            {
                d3d11_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
                d3d11_debug->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        SAFE_RELEASE(d3d11.factory);

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (DXGIGetDebugInterface1 &&
            SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#endif
        {
            dxgiDebug1->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug1->Release();
        }
#endif
    }

    static void d3d11_update_color_space(D3D11SwapChain* context)
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

    static void d3d11_AfterReset(D3D11SwapChain* swapChain)
    {
        d3d11_update_color_space(swapChain);

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

    static bool d3d11_UpdateSwapChain(D3D11SwapChain* swapChain, const PresentationParameters* presentationParameters)
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
                if (any(d3d11.factoryCaps & DXGIFactoryCaps::Tearing))
                {
                    swapChain->presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
                }
            }

            swapChain->handle = d3dCreateSwapChain(
                d3d11.factory, d3d11.factoryCaps, d3d11.device,
                presentationParameters->windowHandle,
                ToDXGISwapChainFormat(presentationParameters->colorFormat),
                presentationParameters->backBufferWidth, presentationParameters->backBufferHeight,
                backbufferCount,
                presentationParameters->isFullscreen);
        }
        else
        {
            // TODO: Resize.
        }

        d3d11_AfterReset(swapChain);

        return true;
    }

    static void d3d11_DestroySwapChain(D3D11SwapChain* swapChain)
    {
        SAFE_RELEASE(swapChain->handle);
        SAFE_RELEASE(swapChain->rtv);
    }

    static void d3d11_resize(uint32_t width, uint32_t height)
    {

    }

    static bool d3d11_beginFrame(void) {
        float clear_color[4] = { 0.392156899f, 0.584313750f, 0.929411829f, 1.0f };

        //ID3D11DeviceContext1_ClearRenderTargetView(d3d11.context, d3d11_current_context->rtv, clear_color);
        //ID3D11DeviceContext1_ClearDepthStencilView(d3d11.context, d3d11_current_context->dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        //ID3D11DeviceContext1_OMSetRenderTargets(d3d11.context, 1, &d3d11_current_context->rtv, d3d11_current_context->dsv);

        // Set the viewport.
        //auto viewport = m_deviceResources->GetScreenViewport();
        //context->RSSetViewports(1, &viewport);

        return true;
    }

    static void d3d11_endFrame(void)
    {
        if (d3d11.swapChain.handle != nullptr)
        {
            HRESULT hr = d3d11.swapChain.handle->Present(d3d11.swapChain.syncInterval, d3d11.swapChain.presentFlags);
            if (hr == DXGI_ERROR_DEVICE_REMOVED
                || hr == DXGI_ERROR_DEVICE_HUNG
                || hr == DXGI_ERROR_DEVICE_RESET
                || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
                || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            {
                d3d11.is_lost = true;
            }
        }

        if (!d3d11.is_lost)
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            if (!d3d11.factory->IsCurrent())
            {
                d3d11_CreateFactory();
            }
        }
    }

    static const Caps* d3d11_QueryCaps(void)
    {
        return &d3d11.caps;
    }

    static RenderPassHandle d3d11_CreateRenderPass(const PassDescription& desc)
    {
        std::lock_guard<std::mutex> lock(d3d11.handle_mutex);

        if (d3d11.renderPasses.isFull())
        {
            logError("D3D11: Not enough free render pass slots.");
            return kInvalidRenderPass;
        }

        const int id = d3d11.renderPasses.alloc();
        if (id < 0) {
            return kInvalidRenderPass;
        }

        D3D11RenderPass& renderPass = d3d11.renderPasses[id];
        renderPass = {};

        HRESULT hr = S_OK;
        for (uint32_t i = 0; i < kMaxColorAttachments; i++)
        {
            if (!desc.colorAttachments[i].texture.isValid())
                continue;

            D3D11Texture& texture = d3d11.textures[desc.colorAttachments[i].texture.value];

            hr = d3d11.device->CreateRenderTargetView(texture.handle, nullptr, &renderPass.rtvs[renderPass.rtvsCount]);
            if (FAILED(hr))
            {
                logError("Direct3D11: Failed to create RenderTargetView");
                d3d11.renderPasses.dealloc((uint32_t)id);
                return kInvalidRenderPass;
            }

            renderPass.rtvsCount++;
        }
        return { (uint32_t)id };
    }

    static void d3d11_DestroyRenderPass(RenderPassHandle handle)
    {

    }

    static BufferHandle d3d11_CreateBuffer(uint32_t count, uint32_t stride, const void* initialData)
    {
        /* Verify resource limits first. */
        static constexpr uint64_t c_maxBytes = D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u;
        static_assert(c_maxBytes <= UINT32_MAX, "Exceeded integer limits");

        uint64_t sizeInBytes = uint64_t(count) * uint64_t(stride);

        if (sizeInBytes > c_maxBytes)
        {
            logError("Direct3D11: Resource size too large for DirectX 11 (size {})", sizeInBytes);
            return kInvalidBuffer;
        }

        std::lock_guard<std::mutex> lock(d3d11.handle_mutex);

        if (d3d11.buffers.isFull())
        {
            logError("D3D11: Not enough free buffer slots.");
            return kInvalidBuffer;
        }

        const int id = d3d11.buffers.alloc();
        if (id < 0) {
            return kInvalidBuffer;
        }

        /*if (any(desc.usage & BufferUsage::Uniform))
        {
            sizeInBytes = AlignTo(sizeInBytes, device->GetCaps().limits.minUniformBufferOffsetAlignment);
        }*/

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = static_cast<UINT>(sizeInBytes);
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        //bufferDesc.BindFlags = D3D11GetBindFlags(desc.usage);
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        /*if (any(desc.usage & BufferUsage::Dynamic))
        {
            d3dDesc.Usage = D3D11_USAGE_DYNAMIC;
            d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else if (any(desc.usage & BufferUsage::Staging)) {
            d3dDesc.Usage = D3D11_USAGE_STAGING;
            d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
        }

        if (any(usage & BufferUsage::StorageReadOnly)
            || any(usage & BufferUsage::StorageReadWrite))
        {
            if (desc.stride != 0)
            {
                d3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            }
            else
            {
                d3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            }
        }

        if (any(desc.usage & BufferUsage::Indirect))
        {
            d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        }*/

        D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        D3D11_SUBRESOURCE_DATA initialResourceData = {};
        if (initialData != nullptr)
        {
            initialResourceData.pSysMem = initialData;
            initialDataPtr = &initialResourceData;
        }

        D3D11Buffer& buffer = d3d11.buffers[id];
        buffer.handle = nullptr;
        HRESULT hr = d3d11.device->CreateBuffer(&bufferDesc, initialDataPtr, &buffer.handle);
        if (FAILED(hr))
        {
            logError("Direct3D11: Failed to create buffer");
            return kInvalidBuffer;
        }

        return { (uint32_t)id };
    }

    static void d3d11_DestroyBuffer(BufferHandle handle)
    {

    }

    static void d3d11_PushDebugGroup(const char* name)
    {
        wchar_t wName[128];
        if (StringConvert(name, wName) > 0)
        {
            d3d11.annotation->BeginEvent(wName);
        }
    }

    static void d3d11_PopDebugGroup(void)
    {
        d3d11.annotation->EndEvent();
    }

    static void d3d11_InsertDebugMarker(const char* name)
    {
        wchar_t wName[128];
        if (StringConvert(name, wName) > 0)
        {
            d3d11.annotation->SetMarker(wName);
        }
    }

    /* Driver */
    static bool d3d11_is_supported(void)
    {
        if (d3d11.available_initialized) {
            return d3d11.available;
        }

        d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        d3d11.dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!d3d11.dxgiDLL) {
            return false;
        }

        CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory1");
        if (!CreateDXGIFactory1)
        {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgiDLL, "DXGIGetDebugInterface1");

        d3d11.d3d11DLL = LoadLibraryA("d3d11.dll");
        if (!d3d11.d3d11DLL) {
            return false;
        }

        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.d3d11DLL, "D3D11CreateDevice");
        if (!D3D11CreateDevice) {
            return false;
        }
#endif

        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        HRESULT hr = D3D11CreateDevice(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            s_featureLevels,
            AGPU_COUNT_OF(s_featureLevels),
            D3D11_SDK_VERSION,
            NULL,
            NULL,
            NULL
        );

        if (FAILED(hr))
        {
            return false;
        }

        d3d11.available = true;
        return true;
    };

    static agpu_renderer* d3d11_create_renderer(void)
    {
        static agpu_renderer renderer = { 0 };
        ASSIGN_DRIVER(d3d11);
        return &renderer;
    }

    Driver D3D11_Driver = {
        BackendType::Direct3D11,
        d3d11_is_supported,
        d3d11_create_renderer
    };
}

#endif /* defined(AGPU_DRIVER_D3D11)  */
