//
// Copyright (c) 2019 Amer Koleci.
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

#include "vgpu_backend.h"

#ifndef CINTERFACE
#   define CINTERFACE
#endif
#ifndef COBJMACROS
#   define COBJMACROS
#endif
#ifndef NOMINMAX
#   define NOMINMAX
#endif
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#ifndef D3D11_NO_HELPERS
#   define D3D11_NO_HELPERS
#endif
#include <windows.h>
#include <d3d11_1.h>
#include "vgpu_d3d.h"
#include <stdio.h>

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#include <d3dcompiler.h>

/* DXGI guids */
static const GUID vgpu_IID_IDXGIAdapter1 = { 0x29038f61, 0x3839, 0x4626, {0x91,0xfd,0x08,0x68,0x79,0x01,0x1a,0x05} };
static const GUID vgpu_IID_IDXGIFactory2 = { 0x50c83a1c, 0xe072, 0x4c48, {0x87,0xb0,0x36,0x30,0xfa,0x36,0xa6,0xd0} };
static const GUID vgpu_IID_IDXGIFactory4 = { 0x1bc6ea02, 0xef36, 0x464f, {0xbf,0x0c,0x21,0xca,0x39,0xe5,0x16,0x8a} };
static const GUID vgpu_IID_IDXGIFactory5 = { 0x7632e1f5, 0xee65, 0x4dca, {0x87,0xfd,0x84,0xcd,0x75,0xf8,0x83,0x8d} };
static const GUID vgpu_IID_IDXGIFactory6 = { 0xc1b6694f, 0xff09, 0x44a9, {0xb0,0x3c,0x77,0x90,0x0a,0x0a,0x1d,0x17} };

#ifdef _DEBUG
#include <dxgidebug.h>

static const GUID vgpu_IID_IDXGIInfoQueue = { 0xD67441C7, 0x672A, 0x476f, {0x9E, 0x82, 0xCD, 0x55, 0xB4, 0x49, 0x49, 0xCE} };
static const GUID vgpu_IID_IDXGIDebug = { 0x119E7452, 0xDE9E, 0x40fe, {0x88, 0x06, 0x88, 0xF9, 0x0C, 0x12, 0xB4, 0x41 } };
static const GUID vgpu_IID_IDXGIDebug1 = { 0xc5a05f0c,0x16f2,0x4adf, {0x9f,0x4d,0xa8,0xc4,0xd5,0x8a,0xc5,0x50 } };

// Declare debug guids to avoid linking with "dxguid.lib"
static const GUID vgpu_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
static const GUID vgpu_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

/* D3D11 guids */
static const GUID vgpu_IID_ID3D11BlendState1 = { 0xcc86fabe, 0xda55, 0x401d, {0x85, 0xe7, 0xe3, 0xc9, 0xde, 0x28, 0x77, 0xe9} };
static const GUID vgpu_IID_ID3D11RasterizerState1 = { 0x1217d7a6, 0x5039, 0x418c, {0xb0, 0x42, 0x9c, 0xbe, 0x25, 0x6a, 0xfd, 0x6e} };
static const GUID vgpu_IID_ID3DDeviceContextState = { 0x5c1e0d8a, 0x7c23, 0x48f9, {0x8c, 0x59, 0xa9, 0x29, 0x58, 0xce, 0xff, 0x11} };
static const GUID vgpu_IID_ID3D11DeviceContext1 = { 0xbb2c6faa, 0xb5fb, 0x4082, {0x8e, 0x6b, 0x38, 0x8b, 0x8c, 0xfa, 0x90, 0xe1} };
static const GUID vgpu_IID_ID3D11Device1 = { 0xa04bfb29, 0x08ef, 0x43d6, {0xa4, 0x9c, 0xa9, 0xbd, 0xbd, 0xcb, 0xe6, 0x86} };
static const GUID vgpu_IID_ID3DUserDefinedAnnotation = { 0xb2daad8b, 0x03d4, 0x4dbf, {0x95, 0xeb, 0x32, 0xab, 0x4b, 0x63, 0xd0, 0xab} };
static const GUID vgpu_IID_ID3D11Texture2D = { 0x6f15aaf2,0xd208,0x4e89, {0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c } };

#ifdef _DEBUG
static const GUID vgpu_IID_ID3D11Debug = { 0x79cf2233, 0x7536, 0x4948, {0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60} };
static const GUID vgpu_IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, {0xab,0x82,0xe9,0x7e,0xc7,0x43,0x26,0xf6} };
#endif

#define VHR(hr) if (FAILED(hr)) { VGPU_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->lpVtbl->Release(obj); (obj) = nullptr; }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

#define _VGPU_MAX_SWAPCHAINS (16u)
typedef struct {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window;
#else
    IUnknown* window;
#endif

    uint32_t width;
    uint32_t height;

    vgpu_pixel_format colorFormat;
    vgpu_color clear_color;
    IDXGISwapChain1* handle;
    UINT sync_interval;
    UINT present_flags;

    vgpu_texture backbufferTexture;
    vgpu_pixel_format depthStencilFormat;
    vgpu_texture depthStencilTexture;
    VGPURenderPass renderPass;

} VGPUSwapchainD3D11;

typedef struct {
    ID3D11Buffer* handle;
} VGPUBufferD3D11;

typedef struct {
    union
    {
        ID3D11Resource* resource;
        ID3D11Texture1D* tex1d;
        ID3D11Texture2D* tex2d;
        ID3D11Texture3D* tex3d;
    } handle;
    DXGI_FORMAT dxgi_format;
    VGPUTextureLayout layout;
    vgpu_texture_desc desc;
} VGPUTextureD3D11;

typedef struct VGPUSamplerD3D11 {
    ID3D11SamplerState* handle;
} VGPUSamplerD3D11;

typedef struct {
    uint32_t                width;
    uint32_t                height;
    uint32_t                color_attachment_count;
    ID3D11RenderTargetView* color_rtvs[VGPU_MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView* dsv;
    vgpu_color              clear_colors[VGPU_MAX_COLOR_ATTACHMENTS];
} VGPURenderPassD3D11;

typedef struct {
    ID3D11VertexShader* vertex;
    ID3D11PixelShader* fragment;
    void*   vs_blob;
    SIZE_T  vs_blob_length;
} vgpu_shader_d3d11;

typedef struct {
    vgpu_shader_d3d11* shader;
    ID3D11InputLayout* input_layout;
} vgpu_pipeline_d3d11;

typedef struct VGPURendererD3D11 {
    /* Associated vgpu_device */
    VGPUDevice gpu_device;

    bool headless;
    bool validation;
    IDXGIFactory2* dxgi_factory;
    bool flip_present_supported;
    bool tearing_supported;

    ID3D11Device1* d3d_device;
    ID3D11DeviceContext1* d3d_context;
    ID3DUserDefinedAnnotation* d3d_annotation;
    D3D_FEATURE_LEVEL feature_level;

    vgpu_caps caps;

    VGPUSwapchainD3D11 swapchains[_VGPU_MAX_SWAPCHAINS];
} VGPURendererD3D11;

static struct {
    bool                    available_initialized;
    bool                    available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    bool can_use_new_features;
    HMODULE dxgi_handle;
    HMODULE d3d11_handle;

    // DXGI functions
    PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    // D3D11 functions.
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;

    HINSTANCE d3dcompiler_dll;
    bool d3dcompiler_dll_load_failed;
    pD3DCompile D3DCompile;
#endif
} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define vgpuCreateDXGIFactory1 d3d11.CreateDXGIFactory1
#   define vgpuCreateDXGIFactory2 d3d11.CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 d3d11.DXGIGetDebugInterface1
#   define vgpuD3D11CreateDevice d3d11.D3D11CreateDevice
#   define _vgpu_d3d11_D3DCompile d3d11.D3DCompile
#else
#   define vgpuCreateDXGIFactory2 CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define vgpuD3D11CreateDevice D3D11CreateDevice
#   define _vgpu_d3d11_D3DCompile D3DCompile
#endif

#if defined(_DEBUG)
static const GUID g_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };

static bool vgpu_sdk_layers_available() {
    HRESULT hr = vgpuD3D11CreateDevice(
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


/* Helper functions */
static void _vgpu_d3d11_set_name(VGPURendererD3D11* renderer, ID3D11DeviceChild* handle, const char* name)
{
#if defined(_DEBUG)
    if (!renderer->validation)
        return;

    if (name)
    {
        const size_t length = strlen(name);
        ID3D11DeviceChild_SetPrivateData(handle, &g_WKPDID_D3DDebugObjectName, (UINT)length, name);
    }
    else
    {
        ID3D11DeviceChild_SetPrivateData(handle, &g_WKPDID_D3DDebugObjectName, 0, NULL);
    }
#endif /* defined(_DEBUG) */
}

/* Conversion functions */
static vgpu_texture_usage _vgpu_d3d11_get_texture_usage(UINT bind_flags) {
    vgpu_texture_usage usage = 0;
    if (bind_flags & D3D11_BIND_SHADER_RESOURCE) {
        usage |= VGPU_TEXTURE_USAGE_SAMPLED;
    }
    if (bind_flags & D3D11_BIND_UNORDERED_ACCESS) {
        usage |= VGPU_TEXTURE_USAGE_STORAGE;
    }
    if (bind_flags & D3D11_BIND_RENDER_TARGET ||
        bind_flags & D3D11_BIND_DEPTH_STENCIL) {
        usage |= VGPU_TEXTURE_USAGE_RENDERTARGET;
    }

    return usage;
}

static UINT _vgpu_d3d11_get_bind_flags(vgpu_buffer_usage usage)
{
    /* Excluse constant buffer */
    if (usage & VGPU_BUFFER_USAGE_UNIFORM)
    {
        return D3D11_BIND_CONSTANT_BUFFER;
    }
    
    UINT bind_flags = 0;
    if (usage & VGPU_BUFFER_USAGE_VERTEX)
    {
        bind_flags |= D3D11_BIND_VERTEX_BUFFER;
    }
    if (usage & VGPU_BUFFER_USAGE_INDEX)
    {
        bind_flags |= D3D11_BIND_INDEX_BUFFER;
    }

    if (usage & VGPU_BUFFER_USAGE_STORAGE)
    {
        bind_flags |= D3D11_BIND_SHADER_RESOURCE;
        bind_flags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    return bind_flags;
}

static D3D11_COMPARISON_FUNC get_d3d11_comparison_func(vgpu_compare_function function)
{
    switch (function)
    {
    case VGPU_COMPARE_FUNCTION_NEVER:
        return D3D11_COMPARISON_NEVER;
    case VGPU_COMPARE_FUNCTION_LESS:
        return D3D11_COMPARISON_LESS;
    case VGPU_COMPARE_FUNCTION_LESS_EQUAL:
        return D3D11_COMPARISON_LESS_EQUAL;
    case VGPU_COMPARE_FUNCTION_GREATER:
        return D3D11_COMPARISON_GREATER;
    case VGPU_COMPARE_FUNCTION_GREATER_EQUAL:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case VGPU_COMPARE_FUNCTION_EQUAL:
        return D3D11_COMPARISON_EQUAL;
    case VGPU_COMPARE_FUNCTION_NOT_EQUAL:
        return D3D11_COMPARISON_NOT_EQUAL;
    case VGPU_COMPARE_FUNCTION_ALWAYS:
        return D3D11_COMPARISON_ALWAYS;

    default:
        _VGPU_UNREACHABLE();
    }
}

static void vgpu_d3d11_init_or_resize_swapchain(
    VGPURendererD3D11* renderer,
    VGPUSwapchainD3D11* swapchain,
    uint32_t width, uint32_t height, bool fullscreen)
{
    const uint32_t sample_count = 1u;
    const DXGI_FORMAT back_buffer_dxgi_format = _vgpu_d3d_swapchain_pixel_format(swapchain->colorFormat);

    if (swapchain->handle != nullptr)
    {
    }
    else
    {
        DXGI_SWAP_CHAIN_DESC1 dxgi_swap_chain_desc = {
            .Width = width,
            .Height = height,
            .Format = back_buffer_dxgi_format,
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2u,
            .SampleDesc = {
                .Count = sample_count,
                .Quality = sample_count > 1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0
            },
            .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
        };

        if (!swapchain->sync_interval && renderer->tearing_supported)
        {
            dxgi_swap_chain_desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }


#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        dxgi_swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
        if (!renderer->flip_present_supported)
        {
            dxgi_swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        }

        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgi_swap_chain_fullscreen_desc = {
            .Windowed = !fullscreen
        };

        // Create a SwapChain from a Win32 window.
        VHR(IDXGIFactory2_CreateSwapChainForHwnd(
            renderer->dxgi_factory,
            (IUnknown*)renderer->d3d_device,
            swapchain->window,
            &dxgi_swap_chain_desc,
            &dxgi_swap_chain_fullscreen_desc,
            nullptr,
            &swapchain->handle
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(IDXGIFactory2_MakeWindowAssociation(renderer->dxgi_factory, swapchain->window, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));
#else
        swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

        // Create a swap chain for the window.
        IDXGISwapChain1* tempSwapChain;
        VHR(d3d11.dxgiFactory->CreateSwapChainForCoreWindow(
            d3d11.device,
            swapChain->window,
            &swapChainDesc,
            nullptr,
            &tempSwapChain
        ));

        IDXGISwapChain3* handle;
        VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain->handle)));
        VHR(swapChain.handle->SetRotation(DXGI_MODE_ROTATION_IDENTITY));
        SAFE_RELEASE(tempSwapChain);
#endif
    }

    ID3D11Texture2D* render_target;
    VHR(IDXGISwapChain1_GetBuffer(swapchain->handle, 0, &vgpu_IID_ID3D11Texture2D, (void**)&render_target));

    D3D11_TEXTURE2D_DESC d3d_texture_desc;
    ID3D11Texture2D_GetDesc(render_target, &d3d_texture_desc);

    const vgpu_texture_desc texture_desc = {
        .type = VGPU_TEXTURE_TYPE_2D,
        .usage = _vgpu_d3d11_get_texture_usage(d3d_texture_desc.BindFlags),
        .width = d3d_texture_desc.Width,
        .height = d3d_texture_desc.Height,
        .layers = d3d_texture_desc.ArraySize,
        .format = VGPUTextureFormat_BGRA8Unorm,
        .mip_levels = d3d_texture_desc.MipLevels,
        .sample_count = d3d_texture_desc.SampleDesc.Count,
        .external_handle = (void*)render_target
    };
    swapchain->backbufferTexture = vgpu_create_texture(&texture_desc);

    if (swapchain->depthStencilFormat != VGPU_PIXELFORMAT_UNDEFINED)
    {
        const vgpu_texture_desc depth_texture_desc = {
            .type = VGPU_TEXTURE_TYPE_2D,
            .usage = VGPU_TEXTURE_USAGE_RENDERTARGET,
            .width = d3d_texture_desc.Width,
            .height = d3d_texture_desc.Height,
            .layers = 1u,
            .format = swapchain->depthStencilFormat,
            .mip_levels = 1u,
            .sample_count = 1u
        };

        swapchain->depthStencilTexture = vgpu_create_texture(&depth_texture_desc);
    }

    VGPURenderPassDescriptor pass_desc = {
        .color_attachments[0].texture = swapchain->backbufferTexture,
        .color_attachments[0].clear_color = swapchain->clear_color
    };

    if (swapchain->depthStencilFormat != VGPU_PIXELFORMAT_UNDEFINED)
    {
        pass_desc.depth_stencil_attachment.texture = swapchain->depthStencilTexture;
    }

    swapchain->renderPass = vgpuCreateRenderPass(&pass_desc);
    ID3D11Texture2D_Release(render_target);
}

static bool d3d11_create_factory(VGPURendererD3D11* renderer)
{
    HRESULT hr = S_OK;

    SAFE_RELEASE(renderer->dxgi_factory);

#if defined(_DEBUG)
    bool debug_dxgi = false;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    if (d3d11.can_use_new_features)
#endif
    {
        if (renderer->validation)
        {
            IDXGIInfoQueue* dxgi_info_queue;
            if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, &vgpu_IID_IDXGIInfoQueue, (void**)&dxgi_info_queue)))
            {
                debug_dxgi = true;

                hr = vgpuCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, &vgpu_IID_IDXGIFactory2, (void**)&renderer->dxgi_factory);
                if (FAILED(hr)) {
                    return false;
                }

                IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info_queue, vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info_queue, vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
                };
                DXGI_INFO_QUEUE_FILTER filter;
                memset(&filter, 0, sizeof(DXGI_INFO_QUEUE_FILTER));
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                IDXGIInfoQueue_AddStorageFilterEntries(dxgi_info_queue, vgpu_DXGI_DEBUG_DXGI, &filter);
                IDXGIInfoQueue_Release(dxgi_info_queue);
            }
        }
    }

    if (!debug_dxgi)
#endif
    {
        hr = vgpuCreateDXGIFactory1(&vgpu_IID_IDXGIFactory2, (void**)&renderer->dxgi_factory);
        if (FAILED(hr)) {
            return false;
        }
    }

    return true;
}

static void vgpu_d3d11_destroy_swapchain(VGPURendererD3D11* renderer, VGPUSwapchainD3D11* swapchain)
{
    if (swapchain->depthStencilTexture) {
        vgpu_destroy_texture(swapchain->depthStencilTexture);
    }

    vgpu_destroy_texture(swapchain->backbufferTexture);
    vgpuDestroyRenderPass(swapchain->renderPass);

    SAFE_RELEASE(swapchain->handle);
}

static bool d3d11_init(VGPUDevice device, const vgpu_config* config)
{
    if (!vgpu_d3d11_supported()) {
        return false;
    }

    VGPURendererD3D11* renderer = (VGPURendererD3D11*)device->renderer;
    renderer->headless = config->swapchain == NULL;
    renderer->validation = config->debug || config->profile;
    if (!d3d11_create_factory(renderer)) {
        vgpu_shutdown();
        return false;
    }

    renderer->flip_present_supported = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    // Disable FLIP if not on a supporting OS
    {
        IDXGIFactory4* dxgi_factory4;
        HRESULT hr = IDXGIFactory2_QueryInterface(renderer->dxgi_factory, &vgpu_IID_IDXGIFactory4, (void**)&dxgi_factory4);
        if (FAILED(hr))
        {
            renderer->flip_present_supported = false;
#ifdef _DEBUG
            OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
        }
        SAFE_RELEASE(dxgi_factory4);
    }
#endif

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* dxgi_factory5;
        HRESULT hr = IDXGIFactory2_QueryInterface(renderer->dxgi_factory, &vgpu_IID_IDXGIFactory5, (void**)&dxgi_factory5);
        if (SUCCEEDED(hr))
        {
            hr = IDXGIFactory5_CheckFeatureSupport(dxgi_factory5, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            renderer->tearing_supported = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            renderer->tearing_supported = true;
        }
        SAFE_RELEASE(dxgi_factory5);
    }

    IDXGIAdapter1* dxgi_adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* dxgi_factory6;
    HRESULT hr = IDXGIFactory2_QueryInterface(renderer->dxgi_factory, &vgpu_IID_IDXGIFactory6, (void**)&dxgi_factory6);
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        const bool lowPower = false;
        if (lowPower) {
            gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
        }

        for (uint32_t i = 0;
            DXGI_ERROR_NOT_FOUND != IDXGIFactory6_EnumAdapterByGpuPreference(dxgi_factory6, i, gpuPreference, &vgpu_IID_IDXGIAdapter1, (void**)&dxgi_adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(IDXGIAdapter1_GetDesc1(dxgi_adapter, &desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                IDXGIAdapter1_Release(dxgi_adapter);
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            break;
        }

        IDXGIFactory6_Release(dxgi_factory6);
    }

#endif
    if (!dxgi_adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != IDXGIFactory2_EnumAdapters1(renderer->dxgi_factory, i, &dxgi_adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(IDXGIAdapter1_GetDesc1(dxgi_adapter, &desc));

            // Don't select the Basic Render Driver adapter.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                IDXGIAdapter1_Release(dxgi_adapter);

                continue;
            }

            break;
        }
    }

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (renderer->validation && vgpu_sdk_layers_available())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
        }
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
            D3D_FEATURE_LEVEL_11_0
        };

        // Create the Direct3D 11 API device object and a corresponding context.
        ID3D11Device* temp_d3d_device;
        ID3D11DeviceContext* temp_d3d_context;

        HRESULT hr = E_FAIL;
        if (dxgi_adapter)
        {
            hr = vgpuD3D11CreateDevice(
                (IDXGIAdapter*)dxgi_adapter,
                D3D_DRIVER_TYPE_UNKNOWN,
                nullptr,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &renderer->feature_level,
                &temp_d3d_context
            );
        }
#if defined(NDEBUG)
        else
        {
            ALIMER_LOGERROR("No Direct3D hardware device found");
            ALIMER_UNREACHABLE();
        }
#else
        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            hr = vgpuD3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                nullptr,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &renderer->feature_level,
                &temp_d3d_context
            );

            if (SUCCEEDED(hr))
            {
                OutputDebugStringA("Direct3D Adapter - WARP\n");
            }
        }
#endif

        if (FAILED(hr)) {
            vgpu_shutdown();
            return false;
        }

#ifndef NDEBUG
        ID3D11Debug* d3d_debug;
        if (SUCCEEDED(ID3D11Device_QueryInterface(temp_d3d_device, &vgpu_IID_ID3D11Debug, (void**)&d3d_debug)))
        {
            ID3D11InfoQueue* d3d_info_queue;
            if (SUCCEEDED(ID3D11Debug_QueryInterface(d3d_debug, &vgpu_IID_ID3D11InfoQueue, (void**)&d3d_info_queue)))
            {
#ifdef _DEBUG
                ID3D11InfoQueue_SetBreakOnSeverity(d3d_info_queue, D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                ID3D11InfoQueue_SetBreakOnSeverity(d3d_info_queue, D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
                D3D11_MESSAGE_ID hide[] =
                {
                    D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                };
                D3D11_INFO_QUEUE_FILTER filter;
                memset(&filter, 0, sizeof(D3D11_INFO_QUEUE_FILTER));
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                ID3D11InfoQueue_AddStorageFilterEntries(d3d_info_queue, &filter);
            }
            SAFE_RELEASE(d3d_info_queue);
            SAFE_RELEASE(d3d_debug);
        }
#endif

        VHR(ID3D11Device_QueryInterface(temp_d3d_device, &vgpu_IID_ID3D11Device1, (void**)&renderer->d3d_device));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &vgpu_IID_ID3D11DeviceContext1, (void**)&renderer->d3d_context));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &vgpu_IID_ID3DUserDefinedAnnotation, (void**)&renderer->d3d_annotation));
        SAFE_RELEASE(temp_d3d_context);
        SAFE_RELEASE(temp_d3d_device);
    }

    // Init features and limits.
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(IDXGIAdapter1_GetDesc1(dxgi_adapter, &adapter_desc));

        renderer->caps.backend = VGPU_BACKEND_D3D11;
        renderer->caps.vendor_id = adapter_desc.VendorId;
        renderer->caps.device_id = adapter_desc.DeviceId;

        char description[_countof(adapter_desc.Description)];
        wcstombs(description, adapter_desc.Description, _countof(adapter_desc.Description));
        memcpy(renderer->caps.adapter_name, description, _countof(adapter_desc.Description));

        renderer->caps.features.independentBlend = true;
        renderer->caps.features.computeShader = true;
        renderer->caps.features.geometryShader = true;
        renderer->caps.features.tessellationShader = true;
        renderer->caps.features.multiViewport = true;
        renderer->caps.features.indexUint32 = true;
        renderer->caps.features.multiDrawIndirect = true;
        renderer->caps.features.fillModeNonSolid = true;
        renderer->caps.features.samplerAnisotropy = true;
        renderer->caps.features.textureCompressionETC2 = false;
        renderer->caps.features.textureCompressionASTC_LDR = false;
        renderer->caps.features.textureCompressionBC = true;
        renderer->caps.features.textureCubeArray = true;
        renderer->caps.features.raytracing = false;

        // Limits
        renderer->caps.limits.max_vertex_attributes = VGPU_MAX_VERTEX_ATTRIBUTES;
        renderer->caps.limits.max_vertex_bindings = VGPU_MAX_VERTEX_ATTRIBUTES;
        renderer->caps.limits.max_vertex_attribute_offset = VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        renderer->caps.limits.max_vertex_binding_stride = VGPU_MAX_VERTEX_BUFFER_STRIDE;

        renderer->caps.limits.max_texture_size_1d = D3D11_REQ_TEXTURE1D_U_DIMENSION;
        renderer->caps.limits.max_texture_size_2d = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        renderer->caps.limits.max_texture_size_3d = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        renderer->caps.limits.max_texture_size_cube = D3D11_REQ_TEXTURECUBE_DIMENSION;
        renderer->caps.limits.max_texture_array_layers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        renderer->caps.limits.max_color_attachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
        renderer->caps.limits.max_uniform_buffer_size = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        renderer->caps.limits.min_uniform_buffer_offset_alignment = 256u;
        renderer->caps.limits.max_storage_buffer_size = UINT32_MAX;
        renderer->caps.limits.min_storage_buffer_offset_alignment = 16;
        renderer->caps.limits.max_sampler_anisotropy = D3D11_MAX_MAXANISOTROPY;
        renderer->caps.limits.max_viewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        renderer->caps.limits.max_viewport_width = D3D11_VIEWPORT_BOUNDS_MAX;
        renderer->caps.limits.max_viewport_height = D3D11_VIEWPORT_BOUNDS_MAX;
        renderer->caps.limits.max_tessellation_patch_size = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        renderer->caps.limits.point_size_range_min = 1.0f;
        renderer->caps.limits.point_size_range_max = 1.0f;
        renderer->caps.limits.line_width_range_min = 1.0f;
        renderer->caps.limits.line_width_range_max = 1.0f;
        renderer->caps.limits.max_compute_shared_memory_size = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        renderer->caps.limits.max_compute_work_group_count_x = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_count_y = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_count_z = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        renderer->caps.limits.max_compute_work_group_invocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        renderer->caps.limits.max_compute_work_group_size_x = D3D11_CS_THREAD_GROUP_MAX_X;
        renderer->caps.limits.max_compute_work_group_size_y = D3D11_CS_THREAD_GROUP_MAX_Y;
        renderer->caps.limits.max_compute_work_group_size_z = D3D11_CS_THREAD_GROUP_MAX_Z;

        /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
        UINT dxgi_fmt_caps = 0;
        for (int fmt = (VGPU_PIXELFORMAT_UNDEFINED + 1); fmt < VGPU_PIXEL_FORMAT_COUNT; fmt++)
        {
            DXGI_FORMAT dxgi_fmt = _vgpu_d3d_get_format((vgpu_pixel_format)fmt);
            if (dxgi_fmt != DXGI_FORMAT_UNKNOWN)
            {
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
        }
    }

    IDXGIAdapter1_Release(dxgi_adapter);

    /* Create main swap chain if required */
    if (config->swapchain)
    {
        renderer->swapchains[0].width = config->swapchain->width;
        renderer->swapchains[0].height = config->swapchain->height;
        renderer->swapchains[0].clear_color = config->swapchain->clear_color;
        renderer->swapchains[0].depthStencilFormat = config->swapchain->depth_stencil_format;
        renderer->swapchains[0].sync_interval = vgpuD3DGetSyncInterval(config->swapchain->present_mode);
        renderer->swapchains[0].present_flags = 0u;

        if (!renderer->swapchains[0].sync_interval &&
            renderer->tearing_supported)
        {
            renderer->swapchains[0].present_flags |= DXGI_PRESENT_ALLOW_TEARING;
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        renderer->swapchains[0].window = (HWND)config->swapchain->handle.window_handle;
        VGPU_ASSERT(IsWindow(renderer->swapchains[0].window));

        if (renderer->swapchains[0].width == 0 ||
            renderer->swapchains[0].height == 0)
        {
            RECT rect;
            GetClientRect(renderer->swapchains[0].window, &rect);
            renderer->swapchains[0].width = rect.right - rect.left;
            renderer->swapchains[0].height = rect.bottom - rect.top;
        }
#else
        renderer->swapchains[0].window = (IUnknown*)desc->swapchain->native_handle;
#endif

        vgpu_d3d11_init_or_resize_swapchain(
            renderer,
            &renderer->swapchains[0],
            renderer->swapchains[0].width, renderer->swapchains[0].height,
            config->swapchain->fullscreen);
    }

    return true;
}

static void d3d11_destroy(VGPUDevice device)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)device->renderer;

    if (renderer->d3d_device)
    {
        /* Destroy swap chains.*/
        for (uint32_t i = 0; i < _VGPU_MAX_SWAPCHAINS; i++)
        {
            if (!renderer->swapchains[i].handle)
                continue;

            vgpu_d3d11_destroy_swapchain(renderer, &renderer->swapchains[i]);
        }

        SAFE_RELEASE(renderer->d3d_context);
        SAFE_RELEASE(renderer->d3d_annotation);

#if !defined(NDEBUG)
        ULONG ref_count = ID3D11Device1_Release(renderer->d3d_device);
        if (ref_count > 0)
        {
            vgpu_log_error_format("Direct3D11: There are %d unreleased references left on the device", ref_count);

            ID3D11Debug* d3d_debug = nullptr;
            if (SUCCEEDED(ID3D11Device1_QueryInterface(renderer->d3d_device, &vgpu_IID_ID3D11Debug, (void**)&d3d_debug)))
            {
                ID3D11Debug_ReportLiveDeviceObjects(d3d_debug, D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
                ID3D11Debug_Release(d3d_debug);
            }
        }
#else
        ID3D11Device1_Release(renderer->d3d_device);
#endif
    }

    SAFE_RELEASE(renderer->dxgi_factory);

#ifdef _DEBUG
    IDXGIDebug* dxgi_debug;
    if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, &vgpu_IID_IDXGIDebug, (void**)&dxgi_debug)))
    {
        IDXGIDebug_ReportLiveObjects(dxgi_debug, vgpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    }
    SAFE_RELEASE(dxgi_debug);
#endif

    VGPU_FREE(renderer);
    VGPU_FREE(device);
}

static vgpu_caps d3d11_query_caps(VGPURenderer* driverData)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    return renderer->caps;
}

static VGPURenderPass d3d11_get_default_render_pass(VGPURenderer* driverData) {
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    return renderer->swapchains[0].renderPass;
}


static vgpu_pixel_format d3d11_get_default_depth_format(VGPURenderer* driver_data)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;

    UINT dxgi_fmt_caps = 0;
    HRESULT hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, DXGI_FORMAT_D32_FLOAT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return VGPU_PIXELFORMAT_DEPTH32_FLOAT;
    }

    hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, DXGI_FORMAT_D16_UNORM, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));
    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return VGPU_PIXELFORMAT_DEPTH16_UNORM;
    }

    return VGPU_PIXELFORMAT_UNDEFINED;
}

static vgpu_pixel_format d3d11_get_default_depth_stencil_format(VGPURenderer* driver_data)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;

    UINT dxgi_fmt_caps = 0;
    HRESULT hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, DXGI_FORMAT_D24_UNORM_S8_UINT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return VGPU_PIXELFORMAT_DEPTH24_PLUS;
    }

    hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, DXGI_FORMAT_D32_FLOAT_S8X24_UINT, &dxgi_fmt_caps);
    VGPU_ASSERT(SUCCEEDED(hr));

    if ((dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0)
    {
        return VGPU_PIXELFORMAT_DEPTH24_PLUS_STENCIL8;
    }

    return VGPU_PIXELFORMAT_UNDEFINED;
}

static void d3d11_wait_idle(VGPURenderer* driver_data) {
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    ID3D11DeviceContext1_Flush(renderer->d3d_context);
}

static void d3d11_begin_frame(VGPURenderer* driver_data) {
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
}

static void d3d11_end_frame(VGPURenderer* driverData) {
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    HRESULT hr = S_OK;

    for (uint32_t i = 0; i < _VGPU_MAX_SWAPCHAINS; i++)
    {
        if (!renderer->swapchains[i].handle)
            continue;

        hr = IDXGISwapChain1_Present(
            renderer->swapchains[i].handle,
            renderer->swapchains[i].sync_interval,
            renderer->swapchains[i].present_flags);

        if (hr == DXGI_ERROR_DEVICE_REMOVED
            || hr == DXGI_ERROR_DEVICE_HUNG
            || hr == DXGI_ERROR_DEVICE_RESET
            || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
            || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            //HandleDeviceLost();
            return;
        }
    }
}

/* Buffer */
static vgpu_buffer d3d11_create_buffer(VGPURenderer* driver_data, const vgpu_buffer_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;

    D3D11_BUFFER_DESC d3d11_desc;
    d3d11_desc.ByteWidth = (UINT)desc->size;
    d3d11_desc.Usage = D3D11_USAGE_DEFAULT;
    d3d11_desc.BindFlags = _vgpu_d3d11_get_bind_flags(desc->usage);
    d3d11_desc.CPUAccessFlags = 0;
    d3d11_desc.MiscFlags = 0;
    d3d11_desc.StructureByteStride = 0;

    if ((desc->usage & VGPU_BUFFER_USAGE_INDIRECT))
    {
        d3d11_desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    }

    if ((desc->usage & VGPU_BUFFER_USAGE_DYNAMIC))
    {
        d3d11_desc.Usage = D3D11_USAGE_DYNAMIC;
        d3d11_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else if ((desc->usage & VGPU_BUFFER_USAGE_STAGING))
    {
        d3d11_desc.Usage = D3D11_USAGE_STAGING;
        d3d11_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    }

    D3D11_SUBRESOURCE_DATA* initialDataPtr = 0;
    D3D11_SUBRESOURCE_DATA initData;
    if (desc->content != nullptr) {
        initData.pSysMem = desc->content;
        initData.SysMemPitch = 0;
        initData.SysMemSlicePitch = 0;
        initialDataPtr = &initData;
    }

    ID3D11Buffer* handle;
    HRESULT hr = ID3D11Device1_CreateBuffer(renderer->d3d_device, &d3d11_desc, initialDataPtr, &handle);
    if (FAILED(hr)) {
        return nullptr;
    }

    VGPUBufferD3D11* result = _VGPU_ALLOC_HANDLE(VGPUBufferD3D11);
    result->handle = handle;
    return (vgpu_buffer)result;
}

static void d3d11_destroy_buffer(VGPURenderer* driver_data, vgpu_buffer handle) {
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    VGPUBufferD3D11* buffer = (VGPUBufferD3D11*)handle;
    ID3D11Buffer_Release(buffer->handle);
    VGPU_FREE(buffer);
}

/* Texture */
static vgpu_texture d3d11_create_texture(VGPURenderer* driverData, const vgpu_texture_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;

    VGPUTextureD3D11* texture = _VGPU_ALLOC_HANDLE(VGPUTextureD3D11);
    texture->dxgi_format = _vgpu_d3d_get_texture_format(desc->format, desc->usage);
    if (desc->external_handle != nullptr)
    {
        texture->handle.resource = (ID3D11Resource*)desc->external_handle;
        ID3D11Resource_AddRef(texture->handle.resource);
    }
    else
    {
        HRESULT hr = S_OK;

        if (desc->type == VGPU_TEXTURE_TYPE_3D)
        {
        }
        else
        {
            const uint32_t multiplier = (desc->type == VGPU_TEXTURE_TYPE_CUBE) ? 6 : 1;

            const D3D11_TEXTURE2D_DESC d3d11_desc = {
                .Width = desc->width,
                .Height = desc->height,
                .MipLevels = desc->mip_levels,
                .ArraySize = desc->layers * multiplier,
                .Format = texture->dxgi_format,
                .SampleDesc = {
                    .Count = desc->sample_count,
                    .Quality = desc->sample_count > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0
                },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = (desc->type == VGPU_TEXTURE_TYPE_CUBE) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0
            };
            hr = ID3D11Device1_CreateTexture2D(renderer->d3d_device, &d3d11_desc, NULL, &texture->handle.tex2d);
        }

        if (FAILED(hr)) {
            VGPU_FREE(texture);
            return nullptr;
        }
    }

    memcpy(&(texture->desc), desc, sizeof(*desc));
    return (vgpu_texture)texture;
}

static void d3d11_destroy_texture(VGPURenderer* driverData, vgpu_texture handle)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    VGPUTextureD3D11* texture = (VGPUTextureD3D11*)handle;
    ID3D11Resource_Release(texture->handle.resource);
    VGPU_FREE(texture);
}

static vgpu_texture_desc d3d11_query_texture_desc(vgpu_texture handle)
{
    VGPUTextureD3D11* texture = (VGPUTextureD3D11*)handle;
    return texture->desc;
}

/* Sampler */
D3D11_FILTER_TYPE get_d3d11_filterType(vgpu_filter filter)
{
    switch (filter)
    {
    case VGPU_FILTER_NEAREST:
        return D3D11_FILTER_TYPE_POINT;
    case VGPU_FILTER_LINEAR:
        return D3D11_FILTER_TYPE_LINEAR;
    default:
        VGPU_UNREACHABLE();
        return (D3D11_FILTER_TYPE)-1;
    }
}

D3D11_FILTER get_d3d11_filter(vgpu_filter minFilter, vgpu_filter magFilter, vgpu_filter mipFilter, bool isComparison, bool isAnisotropic)
{
    D3D11_FILTER filter;
    D3D11_FILTER_REDUCTION_TYPE reduction = isComparison ? D3D11_FILTER_REDUCTION_TYPE_COMPARISON : D3D11_FILTER_REDUCTION_TYPE_STANDARD;

    if (isAnisotropic)
    {
        filter = D3D11_ENCODE_ANISOTROPIC_FILTER(reduction);
    }
    else
    {
        D3D11_FILTER_TYPE dxMin = get_d3d11_filterType(minFilter);
        D3D11_FILTER_TYPE dxMag = get_d3d11_filterType(magFilter);
        D3D11_FILTER_TYPE dxMip = get_d3d11_filterType(mipFilter);
        filter = D3D11_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, reduction);
    }

    return filter;
};


static D3D11_TEXTURE_ADDRESS_MODE get_d3d11_addressMode(vgpu_address_mode mode)
{
    switch (mode)
    {
    default:
    case VGPU_ADDRESS_MODE_CLAMP_TO_EDGE:
        return D3D11_TEXTURE_ADDRESS_CLAMP;

    case VGPU_ADDRESS_MODE_REPEAT:
        return D3D11_TEXTURE_ADDRESS_WRAP;

    case VGPU_ADDRESS_MODE_MIRROR_REPEAT:
        return D3D11_TEXTURE_ADDRESS_MIRROR;

    case VGPU_ADDRESS_MODE_CLAMP_TO_BORDER:
        return D3D11_TEXTURE_ADDRESS_BORDER;
    }
}

static vgpu_sampler d3d11_samplerCreate(VGPURenderer* driver_data, const vgpu_sampler_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;

    const bool isComparison = desc->compare != VGPU_COMPARE_FUNCTION_UNDEFINED;

    D3D11_SAMPLER_DESC sampler_desc = {
        .Filter = get_d3d11_filter(desc->min_filter, desc->mag_filter, desc->mipmap_filter, isComparison, desc->max_anisotropy > 1),
        .AddressU = get_d3d11_addressMode(desc->address_mode_u),
        .AddressV = get_d3d11_addressMode(desc->address_mode_v),
        .AddressW = get_d3d11_addressMode(desc->address_mode_w),
        .MipLODBias = 0.0f,
        .MaxAnisotropy = desc->max_anisotropy,
        .MinLOD = desc->lod_min_clamp,
        .MaxLOD = desc->lod_max_clamp,
    };

    if (isComparison) {
        sampler_desc.ComparisonFunc = get_d3d11_comparison_func(desc->compare);
    }
    else {
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    }

    switch (desc->border_color) {
    case VGPU_BORDER_COLOR_TRANSPARENT_BLACK:
        sampler_desc.BorderColor[0] = 0.0f;
        sampler_desc.BorderColor[1] = 0.0f;
        sampler_desc.BorderColor[2] = 0.0f;
        sampler_desc.BorderColor[3] = 0.0f;
        break;
    case VGPU_BORDER_COLOR_OPAQUE_BLACK:
        sampler_desc.BorderColor[0] = 0.0f;
        sampler_desc.BorderColor[1] = 0.0f;
        sampler_desc.BorderColor[2] = 0.0f;
        sampler_desc.BorderColor[3] = 1.0f;
        break;
    case VGPU_BORDER_COLOR_OPAQUE_WHITE:
        sampler_desc.BorderColor[0] = 1.0f;
        sampler_desc.BorderColor[1] = 1.0f;
        sampler_desc.BorderColor[2] = 1.0f;
        sampler_desc.BorderColor[3] = 1.0f;
        break;
    }

    ID3D11SamplerState* handle;
    HRESULT hr = ID3D11Device1_CreateSamplerState(renderer->d3d_device, &sampler_desc, &handle);
    if (FAILED(hr)) {
        return nullptr;
    }

    VGPUSamplerD3D11* result = _VGPU_ALLOC_HANDLE(VGPUSamplerD3D11);
    result->handle = handle;
    return (vgpu_sampler)result;
}

static void d3d11_samplerDestroy(VGPURenderer* driver_data, vgpu_sampler handle)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    VGPUSamplerD3D11* sampler = (VGPUSamplerD3D11*)handle;
    ID3D11SamplerState_Release(sampler->handle);
    VGPU_FREE(sampler);
}

/* RenderPass */
static VGPURenderPass d3d11_renderPassCreate(VGPURenderer* driverData, const VGPURenderPassDescriptor* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    VGPURenderPassD3D11* renderPass = _VGPU_ALLOC_HANDLE(VGPURenderPassD3D11);
    renderPass->width = UINT32_MAX;
    renderPass->height = UINT32_MAX;

    for (uint32_t i = 0; i < VGPU_MAX_COLOR_ATTACHMENTS; i++) {
        if (!desc->color_attachments[i].texture) {
            continue;
        }

        uint32_t mip_level = desc->color_attachments[i].mip_level;
        VGPUTextureD3D11* d3d11_texture = (VGPUTextureD3D11*)desc->color_attachments[i].texture;
        renderPass->width = _vgpu_min(renderPass->width, _vgpu_max(1u, d3d11_texture->desc.width >> mip_level));
        renderPass->height = _vgpu_min(renderPass->height, _vgpu_max(1u, d3d11_texture->desc.height >> mip_level));

        HRESULT hr = ID3D11Device1_CreateRenderTargetView(
            renderer->d3d_device,
            d3d11_texture->handle.resource,
            nullptr,
            &renderPass->color_rtvs[renderPass->color_attachment_count]);
        VHR(hr);
        memcpy(&renderPass->clear_colors[i].r, &desc->color_attachments[i].clear_color, sizeof(vgpu_color));
        renderPass->color_attachment_count++;
    }

    return (VGPURenderPass)renderPass;
}

static void d3d11_renderPassDestroy(VGPURenderer* driverData, VGPURenderPass handle)
{
    _VGPU_UNUSED(driverData);
    VGPURenderPassD3D11* render_pass = (VGPURenderPassD3D11*)handle;
    for (uint32_t i = 0; i < render_pass->color_attachment_count; i++) {
        ID3D11RenderTargetView_Release(render_pass->color_rtvs[i]);
    }

    if (render_pass->dsv) {
        ID3D11DepthStencilView_Release(render_pass->dsv);
    }

    VGPU_FREE(render_pass);
}

static void d3d11_renderPassGetExtent(VGPURenderer* driverData, VGPURenderPass handle, uint32_t* width, uint32_t* height)
{
    _VGPU_UNUSED(driverData);
    VGPURenderPassD3D11* render_pass = (VGPURenderPassD3D11*)handle;
    if (width) {
        *width = render_pass->width;
    }

    if (height) {
        *height = render_pass->height;
    }
}

static void d3d11_render_pass_set_color_clear_value(VGPURenderPass handle, uint32_t attachment_index, const float colorRGBA[4])
{
    VGPURenderPassD3D11* render_pass = (VGPURenderPassD3D11*)handle;
    VGPU_ASSERT(attachment_index < render_pass->color_attachment_count);

    render_pass->clear_colors[attachment_index].r = colorRGBA[0];
    render_pass->clear_colors[attachment_index].g = colorRGBA[1];
    render_pass->clear_colors[attachment_index].b = colorRGBA[2];
    render_pass->clear_colors[attachment_index].a = colorRGBA[3];
}

static void d3d11_render_pass_set_depth_stencil_clear_value(VGPURenderPass handle, float depth, uint8_t stencil)
{

}

/* Shader */
static bool _vgpu_d3d11_load_d3dcompiler_dll(void)
{
    /* on UWP, don't do anything (not tested) */
#if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
    return true;
#else
    /* load DLL on demand */
    if ((0 == d3d11.d3dcompiler_dll) && !d3d11.d3dcompiler_dll_load_failed)
    {
        d3d11.d3dcompiler_dll = LoadLibraryW(L"d3dcompiler_47.dll");
        if (0 == d3d11.d3dcompiler_dll) {
            /* don't attempt to load missing DLL in the future */
            vgpu_log_error("failed to load d3dcompiler_47.dll!");
            d3d11.d3dcompiler_dll_load_failed = true;
            return false;
        }

        /* look up function pointers */
        d3d11.D3DCompile = (pD3DCompile)GetProcAddress(d3d11.d3dcompiler_dll, "D3DCompile");
        VGPU_ASSERT(d3d11.D3DCompile);
    }

    return 0 != d3d11.d3dcompiler_dll;
#endif

}
static ID3DBlob* _vgpu_d3d11_compile_shader(const vgpu_shader_stage_desc* stage_desc, const char* target)
{
    if (!_vgpu_d3d11_load_d3dcompiler_dll()) {
        return NULL;
    }

    ID3DBlob* output = NULL;
    ID3DBlob* errors_or_warnings = NULL;
    HRESULT hr = _vgpu_d3d11_D3DCompile(
        stage_desc->source,             /* pSrcData */
        strlen(stage_desc->source),     /* SrcDataSize */
        NULL,                           /* pSourceName */
        NULL,                           /* pDefines */
        NULL,                           /* pInclude */
        stage_desc->entry_point ? stage_desc->entry_point : "main",     /* pEntryPoint */
        target,     /* pTarget (vs_5_0 or ps_5_0) */
        D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,   /* Flags1 */
        0,          /* Flags2 */
        &output,    /* ppCode */
        &errors_or_warnings);   /* ppErrorMsgs */
    if (errors_or_warnings) {
        vgpu_log_error((LPCSTR)ID3D10Blob_GetBufferPointer(errors_or_warnings));
        ID3D10Blob_Release(errors_or_warnings); errors_or_warnings = NULL;
    }
    if (FAILED(hr)) {
        /* just in case, usually output is NULL here */
        if (output) {
            ID3D10Blob_Release(output);
            output = NULL;
        }
    }
    return output;
}

static vgpu_shader d3d11_create_shader(VGPURenderer* driver_data, const vgpu_shader_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    HRESULT hr;

    const void* vs_ptr = 0, * fs_ptr = 0;
    SIZE_T vs_length = 0, fs_length = 0;

    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* fs_blob = nullptr;
    if (desc->vertex.byte_code && desc->fragment.byte_code)
    {
    }
    else
    {
        /* compile from shader source code */
        vs_blob = _vgpu_d3d11_compile_shader(&desc->vertex, "vs_5_0");
        fs_blob = _vgpu_d3d11_compile_shader(&desc->fragment, "ps_5_0");
        if (vs_blob && fs_blob) {
            vs_ptr = ID3D10Blob_GetBufferPointer(vs_blob);
            vs_length = ID3D10Blob_GetBufferSize(vs_blob);
            fs_ptr = ID3D10Blob_GetBufferPointer(fs_blob);
            fs_length = ID3D10Blob_GetBufferSize(fs_blob);
        }
    }

    vgpu_shader_d3d11* shader = _VGPU_ALLOC_HANDLE(vgpu_shader_d3d11);

    if (vs_ptr && fs_ptr && (vs_length > 0) && (fs_length > 0))
    {
        /* create the D3D vertex- and pixel-shader objects */
        hr = ID3D11Device_CreateVertexShader(renderer->d3d_device, vs_ptr, vs_length, NULL, &shader->vertex);
        VGPU_ASSERT(SUCCEEDED(hr) && shader->vertex);
        hr = ID3D11Device_CreatePixelShader(renderer->d3d_device, fs_ptr, fs_length, NULL, &shader->fragment);
        VGPU_ASSERT(SUCCEEDED(hr) && shader->fragment);

        /* need to store the vertex shader byte code, this is needed later in sg_create_pipeline */
        shader->vs_blob_length = vs_length;
        shader->vs_blob = VGPU_MALLOC(vs_length);
        VGPU_ASSERT(shader->vs_blob);
        memcpy(shader->vs_blob, vs_ptr, vs_length);

    }
    if (vs_blob) {
        ID3D10Blob_Release(vs_blob); vs_blob = 0;
    }
    if (fs_blob) {
        ID3D10Blob_Release(fs_blob); fs_blob = 0;
    }
    
    return (vgpu_shader)shader;
}

static void d3d11_destroy_shader(VGPURenderer* driver_data, vgpu_shader handle)
{
    vgpu_shader_d3d11* shader = (vgpu_shader_d3d11*)handle;
    VGPU_FREE(shader);
}

/* Pipeline */
static vgpu_pipeline d3d11_create_render_pipeline(VGPURenderer* driver_data, const vgpu_render_pipeline_desc* desc)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driver_data;
    vgpu_pipeline_d3d11* pipeline = _VGPU_ALLOC_HANDLE(vgpu_pipeline_d3d11);
    pipeline->shader = (vgpu_shader_d3d11*)desc->shader;

    HRESULT hr;
    D3D11_INPUT_ELEMENT_DESC d3d11_comps[VGPU_MAX_VERTEX_ATTRIBUTES];
    memset(d3d11_comps, 0, sizeof(d3d11_comps));
    uint32_t attr_count = 0;
    for (uint32_t i = 0; i < VGPU_MAX_VERTEX_ATTRIBUTES; i++)
    {
        /*const VGpuVertexAttributeDescriptor* attrDesc = &desc->vertex_state.attributes[i];
        if (attrDesc->format == VGPU_VERTEX_FORMAT_INVALID) {
            continue;
        }

        assert((attrDesc->bufferIndex >= 0) && (attrDesc->bufferIndex < VGPU_MAX_VERTEX_BUFFER_BINDINGS));
        const VGpuVertexBufferLayoutDescriptor* layoutDesc = &desc->vertex_state.layouts[attrDesc->bufferIndex];

        D3D11_INPUT_ELEMENT_DESC* d3d11ElementDesc = &d3d11InputElements[numElements++];
        d3d11ElementDesc->SemanticName = "ATTRIBUTE";
        d3d11ElementDesc->SemanticIndex = i;
        d3d11ElementDesc->Format = _vgpu_d3d_vertex_format(attrDesc->format);
        d3d11ElementDesc->InputSlot = attrDesc->bufferIndex;
        d3d11ElementDesc->AlignedByteOffset = attrDesc->offset;
        if (layoutDesc->inputRate == VGPU_VERTEX_INPUT_RATE_INSTANCE) {
            d3d11ElementDesc->InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
            d3d11ElementDesc->InstanceDataStepRate = 1u;
        }
        else {
            d3d11ElementDesc->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            d3d11ElementDesc->InstanceDataStepRate = 0u;
        }*/
    }

    hr = ID3D11Device1_CreateInputLayout(renderer->d3d_device,
        d3d11_comps,                
        attr_count,
        pipeline->shader->vs_blob,  
        pipeline->shader->vs_blob_length,
        &pipeline->input_layout);
    VGPU_ASSERT(SUCCEEDED(hr) && pipeline->input_layout);

    return (vgpu_pipeline)pipeline;
}

static vgpu_pipeline d3d11_create_compute_pipeline(VGPURenderer* driver_data, const VgpuComputePipelineDescriptor* desc)
{
    return nullptr;
}

static void d3d11_destroy_pipeline(VGPURenderer* driver_data, vgpu_pipeline handle)
{
    vgpu_pipeline_d3d11* pipeline = (vgpu_pipeline_d3d11*)handle;
    VGPU_FREE(pipeline);
}

static void d3d11_cmdBeginRenderPass(VGPURenderer* driverData, VGPURenderPass handle)
{
    VGPURendererD3D11* renderer = (VGPURendererD3D11*)driverData;
    VGPURenderPassD3D11* render_pass = (VGPURenderPassD3D11*)handle;

    ID3D11DeviceContext1_OMSetRenderTargets(
        renderer->d3d_context,
        render_pass->color_attachment_count, render_pass->color_rtvs,
        render_pass->dsv);

    /* Apply viewport and scissor rect to the render pass */
    const D3D11_VIEWPORT viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = (FLOAT)render_pass->width,
        .Height = (FLOAT)render_pass->height,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f
    };

    const D3D11_RECT scissor_rect = {
        .left = 0,
        .top = 0,
        .right = (LONG)render_pass->width,
        .bottom = (LONG)render_pass->height,
    };

    ID3D11DeviceContext_RSSetViewports(renderer->d3d_context, 1, &viewport);
    ID3D11DeviceContext_RSSetScissorRects(renderer->d3d_context, 1, &scissor_rect);

    /* perform load action */
    for (uint32_t i = 0; i < render_pass->color_attachment_count; i++)
    {
        ID3D11DeviceContext_ClearRenderTargetView(
            renderer->d3d_context, render_pass->color_rtvs[i],
            &render_pass->clear_colors[i].r);
    }
}

static void d3d11_cmdEndRenderPass(VGPURenderer* driverData) {

}

bool vgpu_d3d11_supported(void) {
    if (d3d11.available_initialized) {
        return d3d11.available;
    }
    d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d11.dxgi_handle = LoadLibraryW(L"dxgi.dll");
    if (d3d11.dxgi_handle == nullptr) {
        return false;
    }

    d3d11.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgi_handle, "CreateDXGIFactory2");
    if (d3d11.CreateDXGIFactory2 == nullptr)
    {
        d3d11.CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgi_handle, "CreateDXGIFactory1");
        if (d3d11.CreateDXGIFactory1 == nullptr)
        {
            return false;
        }

        return false;
    }
    else
    {
        d3d11.can_use_new_features = true;
        d3d11.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgi_handle, "DXGIGetDebugInterface1");
    }

    d3d11.d3d11_handle = LoadLibraryW(L"d3d11.dll");
    if (d3d11.d3d11_handle == nullptr) {
        return false;
    }

    d3d11.D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.d3d11_handle, "D3D11CreateDevice");
    if (d3d11.D3D11CreateDevice == nullptr) {
        return false;
    }

#endif

    return true;
}

VGPUDevice d3d11_create_device(void) {
    VGPUDevice device;
    VGPURendererD3D11* renderer;

    device = (VGPUDeviceImpl*)VGPU_MALLOC(sizeof(VGPUDeviceImpl));
    ASSIGN_DRIVER(d3d11);

    /* Init the d3d11 renderer */
    renderer = _VGPU_ALLOC_HANDLE(VGPURendererD3D11);

    /* Reference vgpu_device and renderer together. */
    renderer->gpu_device = device;
    device->renderer = (VGPURenderer*)renderer;

    return device;
}
