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

#if defined(VGPU_DRIVER_D3D11)

#include "vgpu_d3d_common.h"
#define D3D11_NO_HELPERS
#include <d3d11_1.h>
#include <d3dcompiler.h>

namespace vgpu
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    struct BufferD3D11 {
        enum { MAX_COUNT = 4096 };

        ID3D11Buffer* handle;
        uint32_t stride;
    };

    struct TextureD3D11 {
        enum { MAX_COUNT = 4096 };

        union {
            ID3D11Resource* handle;
            ID3D11Texture2D* tex2D;
            ID3D11Texture3D* tex3D;
        };
        ID3D11SamplerState* sampler;
        ID3D11ShaderResourceView* srv;
    };

    struct ShaderD3D11 {
        enum { MAX_COUNT = 1024 };

        ID3D11VertexShader* vertex;
        ID3D11PixelShader* fragment;
        ID3D11ComputeShader* compute;
        ID3D11InputLayout* inputLayout;
    };

    struct ViewportD3D11 {
        IDXGISwapChain1* swapchain;
        uint32_t width;
        uint32_t height;
        PixelFormat colorFormat;
        PixelFormat depthStencilFormat;
        ID3D11RenderTargetView* rtv;
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
        Caps caps;

        Pool<BufferD3D11, BufferD3D11::MAX_COUNT> buffers;
        Pool<TextureD3D11, TextureD3D11::MAX_COUNT> textures;
        Pool<ShaderD3D11, ShaderD3D11::MAX_COUNT> shaders;

        IDXGIFactory2* factory;
        DXGIFactoryCaps factoryCaps;

        ID3D11Device1* device;
        D3D_FEATURE_LEVEL featureLevel;
        ID3D11DeviceContext1* contexts[kMaxCommandLists] = {};
        ID3DUserDefinedAnnotation* annotations[kMaxCommandLists] = {};

        ID3D11BlendState* defaultBlendState;
        ID3D11RasterizerState* defaultRasterizerState;
        ID3D11DepthStencilState* defaultDepthStencilState;

        /* on-demand loaded d3dcompiler_47.dll handles */
        HINSTANCE d3dcompilerDLL;
        bool d3dcompiler_loadFailed;
        pD3DCompile D3DCompile_func;

        uint32_t backbufferCount = 2;
        ViewportD3D11* mainViewport;   // Guaranteed to be == Viewports[0]
        std::vector<ViewportD3D11*> viewports;      // Main viewports, followed by all secondary viewports.
    } d3d11;

    // Check for SDK Layer support.
    static bool d3d11_SdkLayersAvailable() noexcept
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

    static void d3d11_CreateFactory()
    {
        SAFE_RELEASE(d3d11.factory);

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

                VHR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&d3d11.factory)));

                dxgiInfoQueue->SetBreakOnSeverity(vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(vgpu_DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }

        if (!debugDXGI)
#endif
        {
            VHR(CreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory)));
        }

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            IDXGIFactory5* factory5 = nullptr;
            HRESULT hr = d3d11.factory->QueryInterface(&factory5);
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
                d3d11.factoryCaps |= DXGIFactoryCaps::Tearing;
            }

            SAFE_RELEASE(factory5);
        }

        // Disable HDR if we are on an OS that can't support FLIP swap effects
        {
            IDXGIFactory5* factory5 = nullptr;
            if (FAILED(d3d11.factory->QueryInterface(&factory5)))
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: HDR swap chains not supported");
#endif
            }
            else
            {
                d3d11.factoryCaps |= DXGIFactoryCaps::HDR;
            }

            SAFE_RELEASE(factory5);
        }

        // Disable FLIP if not on a supporting OS
        {
            IDXGIFactory4* factory4 = nullptr;
            if (FAILED(d3d11.factory->QueryInterface(&factory4)))
            {
#ifdef _DEBUG
                OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
            }
            else
            {
                d3d11.factoryCaps |= DXGIFactoryCaps::FlipPresent;
            }

            SAFE_RELEASE(factory4);
        }
    }

    static IDXGIAdapter1* d3d11_GetHardwareAdapter(bool lowPower)
    {
        IDXGIAdapter1* adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* factory6 = nullptr;
        HRESULT hr = d3d11.factory->QueryInterface(&factory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = lowPower ? DXGI_GPU_PREFERENCE_MINIMUM_POWER : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;

            for (UINT i = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(&adapter))); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", i, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif

                break;
            }
        }
        SAFE_RELEASE(factory6);
#endif

        if (!adapter)
        {
            for (UINT i = 0; SUCCEEDED(d3d11.factory->EnumAdapters1(i, &adapter)); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();
                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", i, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif

                break;
            }
        }

        return adapter;
    }

    static void d3d11_updateViewport(ViewportD3D11* viewport)
    {
        ID3D11Texture2D* resource;
        VHR(viewport->swapchain->GetBuffer(0, IID_PPV_ARGS(&resource)));
        //_backbufferTextures.Push(new D3D11Texture(_device, resource));

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = ToDXGIFormat(viewport->colorFormat);
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        VHR(d3d11.device->CreateRenderTargetView(
            resource,
            &rtvDesc,
            &viewport->rtv
        ));

        resource->Release();
    }

    static void d3d11_DestroyViewport(ViewportD3D11* viewport)
    {
        viewport->rtv->Release();
        viewport->swapchain->Release();
        delete viewport;
    }

    static bool d3d11_init(InitFlags flags, const PresentationParameters& presentationParameters)
    {
        if (any(flags & InitFlags::DebugRutime) || any(flags & InitFlags::GPUBasedValidation))
        {
            d3d11.debug = true;
        }

        d3d11.buffers.init();
        d3d11.textures.init();
        d3d11.shaders.init();

        // Create factory first.
        d3d11_CreateFactory();

        // Get adapter and create device.
        {
            IDXGIAdapter1* adapter = d3d11_GetHardwareAdapter(any(flags & InitFlags::GPUPreferenceLowPower));

            // Create the Direct3D 11 API device object and a corresponding context.
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

            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

            if (d3d11.debug)
            {
                if (d3d11_SdkLayersAvailable())
                {
                    // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
                }
                else
                {
                    OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
                }
            }

            ID3D11Device* device;
            ID3D11DeviceContext* context;

            HRESULT hr = E_FAIL;
            if (adapter)
            {
                hr = D3D11CreateDevice(
                    adapter,
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &device,
                    &d3d11.featureLevel,
                    &context
                );
            }
#if defined(NDEBUG)
            else
            {
                vgpu::logError("No Direct3D hardware device found");
                return false;
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
                    &d3d11.featureLevel,
                    &context
                );

                if (SUCCEEDED(hr))
                {
                    OutputDebugStringA("Direct3D Adapter - WARP\n");
                }
            }
#endif

            VHR(hr);

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

            VHR(device->QueryInterface(&d3d11.device));
            VHR(context->QueryInterface(&d3d11.contexts[0]));
            VHR(context->QueryInterface(&d3d11.annotations[0]));

            SAFE_RELEASE(context);
            SAFE_RELEASE(device);

            // Init capabilities
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                d3d11.caps.backendType = BackendType::Direct3D11;
                d3d11.caps.vendorId = static_cast<GPUVendorId>(desc.VendorId);
                d3d11.caps.deviceId = desc.DeviceId;
                d3d11.caps.adapterName = ToUtf8(desc.Description, lstrlenW(desc.Description));

                // Features
                d3d11.caps.features.independentBlend = true;
                d3d11.caps.features.computeShader = true;
                d3d11.caps.features.tessellationShader = true;
                d3d11.caps.features.logicOp = true;
                d3d11.caps.features.multiViewport = true;
                d3d11.caps.features.indexUInt32 = true;
                d3d11.caps.features.multiDrawIndirect = true;
                d3d11.caps.features.fillModeNonSolid = true;
                d3d11.caps.features.samplerAnisotropy = true;
                d3d11.caps.features.textureCompressionETC2 = false;
                d3d11.caps.features.textureCompressionASTC_LDR = false;
                d3d11.caps.features.textureCompressionBC = true;
                d3d11.caps.features.textureCubeArray = true;
                d3d11.caps.features.raytracing = false;

                // Limits
                d3d11.caps.limits.maxVertexAttributes = kMaxVertexAttributes;
                d3d11.caps.limits.maxVertexBindings = kMaxVertexAttributes;
                d3d11.caps.limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
                d3d11.caps.limits.maxVertexBindingStride = kMaxVertexBufferStride;

                d3d11.caps.limits.maxTextureDimension1D = D3D11_REQ_TEXTURE1D_U_DIMENSION;
                d3d11.caps.limits.maxTextureDimension2D = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
                d3d11.caps.limits.maxTextureDimension3D = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
                d3d11.caps.limits.maxTextureDimensionCube = D3D11_REQ_TEXTURECUBE_DIMENSION;
                d3d11.caps.limits.maxTextureArraySize = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
                d3d11.caps.limits.maxColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
                d3d11.caps.limits.maxUniformBufferSize = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
                d3d11.caps.limits.minUniformBufferOffsetAlignment = 256u;
                d3d11.caps.limits.maxStorageBufferSize = UINT32_MAX;
                d3d11.caps.limits.minStorageBufferOffsetAlignment = 16;
                d3d11.caps.limits.maxSamplerAnisotropy = D3D11_MAX_MAXANISOTROPY;
                d3d11.caps.limits.maxViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
                d3d11.caps.limits.maxViewportWidth = D3D11_VIEWPORT_BOUNDS_MAX;
                d3d11.caps.limits.maxViewportHeight = D3D11_VIEWPORT_BOUNDS_MAX;
                d3d11.caps.limits.maxTessellationPatchSize = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
                d3d11.caps.limits.pointSizeRangeMin = 1.0f;
                d3d11.caps.limits.pointSizeRangeMax = 1.0f;
                d3d11.caps.limits.lineWidthRangeMin = 1.0f;
                d3d11.caps.limits.lineWidthRangeMax = 1.0f;
                d3d11.caps.limits.maxComputeSharedMemorySize = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
                d3d11.caps.limits.maxComputeWorkGroupCountX = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
                d3d11.caps.limits.maxComputeWorkGroupCountY = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
                d3d11.caps.limits.maxComputeWorkGroupCountZ = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
                d3d11.caps.limits.maxComputeWorkGroupInvocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
                d3d11.caps.limits.maxComputeWorkGroupSizeX = D3D11_CS_THREAD_GROUP_MAX_X;
                d3d11.caps.limits.maxComputeWorkGroupSizeY = D3D11_CS_THREAD_GROUP_MAX_Y;
                d3d11.caps.limits.maxComputeWorkGroupSizeZ = D3D11_CS_THREAD_GROUP_MAX_Z;

                /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
                UINT dxgi_fmt_caps = 0;
                for (uint32_t format = (static_cast<uint32_t>(PixelFormat::Invalid) + 1); format < static_cast<uint32_t>(PixelFormat::Count); format++)
                {
                    const DXGI_FORMAT dxgiFormat = ToDXGIFormat((PixelFormat)format);

                    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
                        continue;

                    UINT formatSupport = 0;
                    if (FAILED(d3d11.device->CheckFormatSupport(dxgiFormat, &formatSupport))) {
                        continue;
                    }

                    D3D11_FORMAT_SUPPORT tf = (D3D11_FORMAT_SUPPORT)formatSupport;
                }
            }

            SAFE_RELEASE(adapter);
        }



        // Create swap chain if not running in headless
        if (presentationParameters.windowHandle != nullptr)
        {
            d3d11.mainViewport = new ViewportD3D11();
            d3d11.mainViewport->colorFormat = presentationParameters.backbufferFormat;
            d3d11.mainViewport->depthStencilFormat = presentationParameters.depthStencilFormat;

            d3d11.mainViewport->swapchain = d3dCreateSwapchain(
                d3d11.factory,
                d3d11.factoryCaps,
                d3d11.device,
                presentationParameters.windowHandle,
                presentationParameters.backbufferWidth,
                presentationParameters.backbufferHeight,
                presentationParameters.backbufferFormat,
                d3d11.backbufferCount);

            // Get dimension from SwapChain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
            d3d11.mainViewport->swapchain->GetDesc1(&swapChainDesc);
            d3d11.mainViewport->width = swapChainDesc.Width;
            d3d11.mainViewport->height = swapChainDesc.Height;
            d3d11_updateViewport(d3d11.mainViewport);
            d3d11.viewports.push_back(d3d11.mainViewport);
        }

        // Create imGui objects (for now here)
        {
            D3D11_BLEND_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.AlphaToCoverageEnable = false;
            desc.RenderTarget[0].BlendEnable = true;
            desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            d3d11.device->CreateBlendState(&desc, &d3d11.defaultBlendState);
        }

        // Create the rasterizer state
        {
            D3D11_RASTERIZER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            desc.ScissorEnable = true;
            desc.DepthClipEnable = true;
            d3d11.device->CreateRasterizerState(&desc, &d3d11.defaultRasterizerState);
        }

        // Create depth-stencil State
        {
            D3D11_DEPTH_STENCIL_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.DepthEnable = false;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
            desc.StencilEnable = false;
            desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            desc.BackFace = desc.FrontFace;
            d3d11.device->CreateDepthStencilState(&desc, &d3d11.defaultDepthStencilState);
        }

        return true;
    }

    static void d3d11_shutdown(void)
    {
        for (size_t i = 0; i < d3d11.viewports.size(); i++)
        {
            d3d11_DestroyViewport(d3d11.viewports[i]);
        }

        d3d11.viewports.clear();

        for (uint32_t i = 0; i < _countof(d3d11.contexts); i++)
        {
            SAFE_RELEASE(d3d11.annotations[i]);
            SAFE_RELEASE(d3d11.contexts[i]);
        }

        SAFE_RELEASE(d3d11.defaultBlendState);
        SAFE_RELEASE(d3d11.defaultRasterizerState);
        SAFE_RELEASE(d3d11.defaultDepthStencilState);

        ULONG refCount = d3d11.device->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            vgpu::logError("Direct3D11: There are %d unreleased references left on the device", refCount);

            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&d3dDebug))))
            {
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
                d3dDebug->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        SAFE_RELEASE(d3d11.factory);

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (DXGIGetDebugInterface1 && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#else
        if (SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#endif
        {
            dxgiDebug1->ReportLiveObjects(vgpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug1->Release();
        }
#endif

        memset(&d3d11, 0, sizeof(d3d11));
    }

    static void d3d11_beginFrame(void)
    {
        if (d3d11.mainViewport)
        {
            float clearColor[4] = { 0.392156899f, 0.584313750f, 0.929411829f, 1.0f };
            d3d11.contexts[0]->ClearRenderTargetView(d3d11.mainViewport->rtv, clearColor);
            d3d11.contexts[0]->OMSetRenderTargets(1, &d3d11.mainViewport->rtv, d3d11.mainViewport->dsv);
            cmdSetViewport(0, 0.0f, 0.0f, static_cast<float>(d3d11.mainViewport->width), static_cast<float>(d3d11.mainViewport->height));
            SetScissorRect(0, 0, 0, d3d11.mainViewport->width, d3d11.mainViewport->height);
        }
    }

    static void d3d11_endFrame(void)
    {
        HRESULT hr = S_OK;
        for (uint32_t i = 1; i < d3d11.viewports.size(); i++) {

        }

        if (d3d11.mainViewport)
        {
            hr = d3d11.mainViewport->swapchain->Present(1, 0);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // TODO: Handle device lost.
            }
        }

        VHR(hr);

        if (!d3d11.factory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            d3d11_CreateFactory();
        }
    }

    static const Caps* d3d11_queryCaps(void) {
        return &d3d11.caps;
    }

    /* Resource creation methods */
    static TextureHandle d3d11_createTexture(const TextureDescription& desc, const void* initialData)
    {
        if (d3d11.textures.isFull()) {
            logError("Direct3D11: Not enough free texture slots.");
            return kInvalidTexture;
        }

        const int id = d3d11.textures.alloc();
        TextureD3D11& texture = d3d11.textures[id];
        texture = {};

        // Setup initial data.
        std::vector<D3D11_SUBRESOURCE_DATA> subresourceData;
        if (initialData != nullptr)
        {
            subresourceData.resize(desc.arraySize * desc.mipLevels);

            const uint8_t* srcMem = reinterpret_cast<const uint8_t*>(initialData);
            const uint32_t srcTexelSize = getFormatBytesPerBlock(desc.format);

            for (uint32_t arrayIndex = 0; arrayIndex < desc.arraySize; arrayIndex++)
            {
                uint32_t mipWidth = desc.width;

                for (uint32_t mipIndex = 0; mipIndex < desc.mipLevels; mipIndex++)
                {
                    const uint32_t subresourceIndex = mipIndex + (arrayIndex * desc.mipLevels);
                    const uint32_t srcPitch = mipWidth * srcTexelSize;

                    subresourceData[subresourceIndex].pSysMem = srcMem;
                    subresourceData[subresourceIndex].SysMemPitch = srcPitch;
                    subresourceData[subresourceIndex].SysMemSlicePitch = 0;

                    srcMem += srcPitch;
                    mipWidth = _vgpu_max(mipWidth / 2u, 1u);
                }
            }
        }

        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT d3d11_bind_flags = 0;
        UINT CPUAccessFlags = 0;
        UINT d3d11_misc_flags = 0;
        uint32_t arraySize = desc.arraySize;

        if (desc.mipLevels == 0)
        {
            d3d11_bind_flags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            d3d11_misc_flags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        if (any(desc.usage & TextureUsage::Sampled))
        {
            d3d11_bind_flags |= D3D11_BIND_SHADER_RESOURCE;
        }

        if (any(desc.usage & TextureUsage::Storage))
        {
            d3d11_bind_flags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (desc.type == TextureType::TypeCube) {
            arraySize *= 6;
            d3d11_misc_flags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        const DXGI_FORMAT dxgiFormat = ToDXGIFormatWithUsage(desc.format, desc.usage);

        HRESULT hr = S_OK;
        if (desc.type == TextureType::Type2D || desc.type == TextureType::TypeCube)
        {
            D3D11_TEXTURE2D_DESC d3d11_desc = {};
            d3d11_desc.Width = _vgpu_align_to(getFormatWidthCompressionRatio(desc.format), desc.width);
            d3d11_desc.Height = _vgpu_align_to(getFormatWidthCompressionRatio(desc.format), desc.height);
            d3d11_desc.MipLevels = desc.mipLevels;
            d3d11_desc.ArraySize = arraySize;
            d3d11_desc.Format = dxgiFormat;
            d3d11_desc.SampleDesc.Count = desc.sampleCount;
            d3d11_desc.SampleDesc.Quality = 0;
            d3d11_desc.Usage = usage;
            d3d11_desc.BindFlags = d3d11_bind_flags;
            d3d11_desc.CPUAccessFlags = CPUAccessFlags;
            d3d11_desc.MiscFlags = d3d11_misc_flags;

            hr = d3d11.device->CreateTexture2D(
                &d3d11_desc,
                subresourceData.size() ? subresourceData.data() : (D3D11_SUBRESOURCE_DATA*)nullptr,
                &texture.tex2D);
        }

        // Create texture sampler
        {
            D3D11_SAMPLER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.MipLODBias = 0.f;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.MinLOD = 0.f;
            desc.MaxLOD = 0.f;
            d3d11.device->CreateSamplerState(&desc, &texture.sampler);
        }

        if (any(desc.usage & TextureUsage::Sampled))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.mipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            VHR(d3d11.device->CreateShaderResourceView(texture.handle, &srvDesc, &texture.srv));
        }

        if (FAILED(hr)) {
            d3d11.textures.dealloc((uint32_t)id);
            return kInvalidTexture;
        }

        return { (uint32_t)id };
    }

    static void d3d11_destroyTexture(TextureHandle handle)
    {
        TextureD3D11& texture = d3d11.textures[handle.id];
        SAFE_RELEASE(texture.handle);
        SAFE_RELEASE(texture.sampler);
        d3d11.textures.dealloc(handle.id);
    }

    static BufferHandle d3d11_createBuffer(uint32_t size, BufferUsage usage, uint32_t stride, const void* initialData)
    {
        if (d3d11.buffers.isFull()) {
            logError("Direct3D11: Not enough free buffer slots.");
            return kInvalidBuffer;
        }

        const int id = d3d11.buffers.alloc();
        BufferD3D11& buffer = d3d11.buffers[id];
        buffer = {};

        HRESULT hr = S_OK;
        D3D11_BUFFER_DESC d3d11Desc = {};
        d3d11Desc.ByteWidth = size;
        d3d11Desc.Usage = D3D11_USAGE_DYNAMIC;
        if (any(usage & BufferUsage::Uniform))
        {
            d3d11Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        }
        else
        {
            if (any(usage & BufferUsage::Vertex))
                d3d11Desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;

            if (any(usage & BufferUsage::Index))
                d3d11Desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
        }

        d3d11Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        d3d11Desc.MiscFlags = 0;

        hr = d3d11.device->CreateBuffer(&d3d11Desc, nullptr, &buffer.handle);

        if (FAILED(hr)) {
            d3d11.buffers.dealloc((uint32_t)id);
            return kInvalidBuffer;
        }

        buffer.stride = stride;

        return { (uint32_t)id };
    }

    static void d3d11_destroyBuffer(BufferHandle handle)
    {
        BufferD3D11& buffer = d3d11.buffers[handle.id];
        SAFE_RELEASE(buffer.handle);
        d3d11.buffers.dealloc(handle.id);
    }

    static void* d3d11_MapBuffer(BufferHandle handle)
    {
        BufferD3D11& buffer = d3d11.buffers[handle.id];
        D3D11_MAPPED_SUBRESOURCE mapSubresource;
        VHR(d3d11.contexts[0]->Map(buffer.handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSubresource));
        return (uint8_t*)mapSubresource.pData;
    }

    static void d3d11_UnmapBuffer(BufferHandle handle)
    {
        BufferD3D11& buffer = d3d11.buffers[handle.id];
        d3d11.contexts[0]->Unmap(buffer.handle, 0);
    }

    static bool d3d11_load_d3dcompiler_dll(void)
    {
#if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
        return true;
#else
        /* load DLL on demand */
        if (d3d11.d3dcompilerDLL == nullptr && !d3d11.d3dcompiler_loadFailed)
        {
            d3d11.d3dcompilerDLL = LoadLibraryA("d3dcompiler_47.dll");
            if (d3d11.d3dcompilerDLL == nullptr)
            {
                logError("Direct3D11: failed to load d3dcompiler_47.dll");
                d3d11.d3dcompiler_loadFailed = true;
                return false;
            }

            /* look up function pointers */
            d3d11.D3DCompile_func = (pD3DCompile)GetProcAddress(d3d11.d3dcompilerDLL, "D3DCompile");
            VGPU_ASSERT(d3d11.D3DCompile_func);
        }

        return d3d11.d3dcompilerDLL != nullptr;
#endif
    }

#if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#define _vgpu_d3d11_D3DCompile D3DCompile
#else
#define _vgpu_d3d11_D3DCompile d3d11.D3DCompile_func
#endif

    static ShaderBlob d3d11_compileShader(const char* source, const char* entryPoint, ShaderStage stage)
    {
        ShaderBlob result{};
        if (!d3d11_load_d3dcompiler_dll()) {
            return result;
        }

        const char* target = "vs_5_0";
        switch (stage)
        {
        case vgpu::ShaderStage::Vertex:
            target = "vs_5_0";
            break;
        case vgpu::ShaderStage::Fragment:
            target = "ps_5_0";
            break;
        case vgpu::ShaderStage::Compute:
            target = "cs_5_0";
            break;
        default:
            break;
        }

        UINT compileFlags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG;
#else
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ID3DBlob* output = nullptr;
        ID3DBlob* errorBlob = nullptr;
        HRESULT hr = _vgpu_d3d11_D3DCompile(
            source,
            strlen(source),
            nullptr,
            nullptr,
            nullptr,
            entryPoint,
            target,
            compileFlags,
            0,
            &output,
            &errorBlob
        );

        if (FAILED(hr))
        {
            logError((LPCSTR)errorBlob->GetBufferPointer());
            SAFE_RELEASE(errorBlob);
        }

        result.size = output->GetBufferSize();
        result.data = (uint8_t*)malloc(result.size);
        memcpy(result.data, output->GetBufferPointer(), result.size);
        output->Release();
        return result;
    }

    static ShaderHandle d3d11_createShader(const ShaderDescription& desc)
    {
        if (d3d11.shaders.isFull()) {
            logError("Direct3D11: Not enough free shader slots.");
            return kInvalidShader;
        }

        const int id = d3d11.shaders.alloc();
        ShaderD3D11& shader = d3d11.shaders[id];
        shader = {};

        for (uint32_t i = 0; i < (uint32_t)ShaderStage::Count; i++)
        {
            if (!desc.stages[i].size)
                continue;

            switch ((ShaderStage)i)
            {
            case vgpu::ShaderStage::Vertex:
                VHR(d3d11.device->CreateVertexShader(desc.stages[i].data, desc.stages[i].size, nullptr, &shader.vertex));
                break;
            case vgpu::ShaderStage::Fragment:
                VHR(d3d11.device->CreatePixelShader(desc.stages[i].data, desc.stages[i].size, nullptr, &shader.fragment));
                break;
            case vgpu::ShaderStage::Compute:
                VHR(d3d11.device->CreateComputeShader(desc.stages[i].data, desc.stages[i].size, nullptr, &shader.compute));
                break;
            default:
                break;
            }
        }

        // Create the input layout
        D3D11_INPUT_ELEMENT_DESC local_layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        if (d3d11.device->CreateInputLayout(local_layout, 3,
            desc.stages[0].data,
            desc.stages[0].size, &shader.inputLayout) != S_OK)
        {
            return kInvalidShader;
        }

        return { (uint32_t)id };
    }

    static void d3d11_destroyShader(ShaderHandle handle)
    {
        ShaderD3D11& shader = d3d11.shaders[handle.id];
        SAFE_RELEASE(shader.vertex);
        SAFE_RELEASE(shader.fragment);
        SAFE_RELEASE(shader.compute);
        SAFE_RELEASE(shader.inputLayout);
        d3d11.shaders.dealloc(handle.id);
    }

    /* Commands */
    static void d3d11_cmdSetViewport(CommandList commandList, float x, float y, float width, float height, float min_depth, float max_depth)
    {
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = x;
        viewport.TopLeftY = y;
        viewport.Width = width;
        viewport.Height = height;
        viewport.MinDepth = min_depth;
        viewport.MaxDepth = max_depth;

        d3d11.contexts[commandList]->RSSetViewports(1, &viewport);
    }

    static void d3d11_cmdSetScissor(CommandList commandList, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        D3D11_RECT scissorRect = {};
        scissorRect.left = (LONG)x;
        scissorRect.top = (LONG)y;
        scissorRect.right = LONG(x + width);
        scissorRect.bottom = LONG(y + height);
        d3d11.contexts[commandList]->RSSetScissorRects(1, &scissorRect);
    }

    static void d3d11_cmdSetVertexBuffer(CommandList commandList, BufferHandle handle)
    {
        BufferD3D11& buffer = d3d11.buffers[handle.id];
        uint32_t offset = 0;
        d3d11.contexts[commandList]->IASetVertexBuffers(0, 1, &buffer.handle, &buffer.stride, &offset);
    }

    static void d3d11_cmdSetIndexBuffer(CommandList commandList, BufferHandle handle)
    {
        BufferD3D11& buffer = d3d11.buffers[handle.id];
        d3d11.contexts[commandList]->IASetIndexBuffer(buffer.handle, buffer.stride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    }

    static void d3d11_cmdSetShader(CommandList commandList, ShaderHandle handle)
    {
        ShaderD3D11& shader = d3d11.shaders[handle.id];
        d3d11.contexts[commandList]->IASetInputLayout(shader.inputLayout);
        d3d11.contexts[commandList]->VSSetShader(shader.vertex, nullptr, 0);
        d3d11.contexts[commandList]->PSSetShader(shader.fragment, nullptr, 0);
        d3d11.contexts[commandList]->GSSetShader(nullptr, nullptr, 0);
        d3d11.contexts[commandList]->HSSetShader(nullptr, nullptr, 0);
        d3d11.contexts[commandList]->DSSetShader(nullptr, nullptr, 0);
        d3d11.contexts[commandList]->CSSetShader(nullptr, nullptr, 0);

        // Setup blend state
        const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
        d3d11.contexts[commandList]->OMSetBlendState(d3d11.defaultBlendState, blend_factor, 0xffffffff);
        d3d11.contexts[commandList]->OMSetDepthStencilState(d3d11.defaultDepthStencilState, 0);
        d3d11.contexts[commandList]->RSSetState(d3d11.defaultRasterizerState);
    }

    static void d3d11_cmdBindUniformBuffer(CommandList commandList, uint32_t slot, BufferHandle handle)
    {
        BufferD3D11& buffer = d3d11.buffers[handle.id];
        d3d11.contexts[commandList]->VSSetConstantBuffers(slot, 1, &buffer.handle);
    }

    static void d3d11_cmdBindTexture(CommandList commandList, uint32_t slot, TextureHandle handle)
    {
        TextureD3D11& texture = d3d11.textures[handle.id];
        d3d11.contexts[commandList]->PSSetShaderResources(slot, 1, &texture.srv);
        d3d11.contexts[commandList]->PSSetSamplers(slot, 1, &texture.sampler);
    }

    static void d3d11_cmdDrawIndexed(CommandList commandList, uint32_t indexCount, uint32_t startIndex, int32_t baseVertex)
    {
        d3d11.contexts[commandList]->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3d11.contexts[commandList]->DrawIndexed(indexCount, startIndex, baseVertex);
    }

    static bool d3d11_isSupported(void) {
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
        if (CreateDXGIFactory1 == nullptr)
            return false;

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgiDLL, "DXGIGetDebugInterface1");

        d3d11.d3d11DLL = LoadLibraryA("d3d11.dll");
        if (!d3d11.d3d11DLL) {
            return false;
        }

        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.d3d11DLL, "D3D11CreateDevice");
        if (D3D11CreateDevice == nullptr) {
            return false;
        }

#endif

        // Determine DirectX hardware feature levels this app will support.
        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
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

        if (FAILED(hr))
        {
            return false;
        }

        d3d11.available = true;
        return true;
    };

    static Renderer* d3d11_initRenderer(void) {
        static Renderer renderer = { nullptr };
        renderer.init = d3d11_init;
        renderer.shutdown = d3d11_shutdown;
        renderer.beginFrame = d3d11_beginFrame;
        renderer.endFrame = d3d11_endFrame;
        renderer.queryCaps = d3d11_queryCaps;

        /* Resource creation methods */
        renderer.createTexture = d3d11_createTexture;
        renderer.destroyTexture = d3d11_destroyTexture;

        renderer.createBuffer = d3d11_createBuffer;
        renderer.destroyBuffer = d3d11_destroyBuffer;
        renderer.MapBuffer = d3d11_MapBuffer;
        renderer.UnmapBuffer = d3d11_UnmapBuffer;

        renderer.compileShader = d3d11_compileShader;
        renderer.createShader = d3d11_createShader;
        renderer.destroyShader = d3d11_destroyShader;

        /* Commands */
        renderer.cmdSetViewport = d3d11_cmdSetViewport;
        renderer.cmdSetScissor = d3d11_cmdSetScissor;
        renderer.cmdSetVertexBuffer = d3d11_cmdSetVertexBuffer;
        renderer.cmdSetIndexBuffer = d3d11_cmdSetIndexBuffer;
        renderer.cmdSetShader = d3d11_cmdSetShader;
        renderer.cmdBindUniformBuffer = d3d11_cmdBindUniformBuffer;
        renderer.cmdBindTexture = d3d11_cmdBindTexture;
        renderer.cmdDrawIndexed = d3d11_cmdDrawIndexed;

        return &renderer;
    }

    Driver d3d11_driver = {
        BackendType::Direct3D11,
        d3d11_isSupported,
        d3d11_initRenderer
    };
}

#endif /* defined(VGPU_DRIVER_D3D12) */
