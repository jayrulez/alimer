#if defined(GPU_DRIVER_D3D11)
#include "gpu_backend.h"

#define D3D11_NO_HELPERS
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#if defined(_DEBUG)
#include <dxgidebug.h>
static const IID D3D11_IID_IDXGIInfoQueue = { 0xD67441C7, 0x672A, 0x476f, {0x9E, 0x82, 0xCD, 0x55, 0xB4, 0x49, 0x49, 0xCE} };
static const IID D3D11_IID_IDXGIDebug = { 0x119E7452, 0xDE9E, 0x40fe, {0x88, 0x06, 0x88, 0xF9, 0x0C, 0x12, 0xB4, 0x41} };
static const IID D3D11_IID_IDXGIDebug1 = { 0xc5a05f0c, 0x16f2, 0x4adf, {0x9f, 0x4d, 0xa8, 0xc4, 0xd5, 0x8a, 0xc5, 0x50} };

static const IID D3D11_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8 } };
static const IID D3D11_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a } };
#endif

static const IID D3D11_IID_IDXGIFactory1 = { 0x770aae78,0xf26f,0x4dba,{0xa8,0x29,0x25,0x3c,0x83,0xd1,0xb3,0x87} };
static const IID D3D11_IID_IDXGIFactory2 = { 0x50c83a1c,0xe072,0x4c48,{0x87,0xb0,0x36,0x30,0xfa,0x36,0xa6,0xd0} };
static const IID D3D11_IID_IDXGIFactory4 = { 0x1bc6ea02, 0xef36, 0x464f, {0xbf,0x0c,0x21,0xca,0x39,0xe5,0x16,0x8a} };
static const IID D3D11_IID_IDXGIFactory5 = { 0x7632e1f5, 0xee65, 0x4dca, {0x87,0xfd,0x84,0xcd,0x75,0xf8,0x83,0x8d} };
static const IID D3D11_IID_IDXGIFactory6 = { 0xc1b6694f, 0xff09, 0x44a9, {0xb0,0x3c,0x77,0x90,0x0a,0x0a,0x1d,0x17} };
static const IID D3D11_IID_IDXGIAdapter1 = { 0x29038f61, 0x3839, 0x4626, {0x91,0xfd,0x08,0x68,0x79,0x01,0x1a,0x05} };

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);

static PFN_CREATE_DXGI_FACTORY1 createDXGIFactory1Func = NULL;
static PFN_CREATE_DXGI_FACTORY2 createDXGIFactory2Func = NULL;
static PFN_GET_DXGI_DEBUG_INTERFACE1 dxgiGetDebugInterface1Func = NULL;
static PFN_D3D11_CREATE_DEVICE d3d11CreateDeviceFunc = NULL;

#   define CreateDXGIFactory1Func createDXGIFactory1Func
#   define CreateDXGIFactory2Func createDXGIFactory2Func
#   define DXGIGetDebugInterface1Func dxgiGetDebugInterface1Func
#   define D3D11CreateDeviceFunc d3d11CreateDeviceFunc
#else
#   define CreateDXGIFactory2Func CreateDXGIFactory2
#   define DXGIGetDebugInterface1Func DXGIGetDebugInterface1
#   define D3D11CreateDeviceFunc D3D11CreateDevice
#endif

static const IID D3D11_IID_ID3D11Device1 = { 0xa04bfb29, 0x08ef, 0x43d6, {0xa4, 0x9c, 0xa9, 0xbd, 0xbd, 0xcb, 0xe6, 0x86} };
static const IID D3D11_IID_ID3D11DeviceContext1 = { 0xbb2c6faa, 0xb5fb, 0x4082, {0x8e, 0x6b, 0x38, 0x8b, 0x8c, 0xfa, 0x90, 0xe1} };
static const IID D3D11_IID_ID3DUserDefinedAnnotation = { 0xb2daad8b, 0x03d4, 0x4dbf, {0x95, 0xeb, 0x32, 0xab, 0x4b, 0x63, 0xd0, 0xab} };
static const IID D3D11_IID_ID3D11Texture2D = { 0x6f15aaf2,0xd208,0x4e89, {0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c } };

#if defined(_DEBUG)
static const IID D3D11_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
static const IID D3D11_IID_ID3D11Debug = { 0x79cf2233, 0x7536, 0x4948, {0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60} };
static const IID D3D11_IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, {0xab,0x82,0xe9,0x7e,0xc7,0x43,0x26,0xf6} };

static bool D3D11SdkLayersAvailable() {
    HRESULT hr = D3D11CreateDeviceFunc(
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
#endif

#define VHR(hr) if (FAILED(hr)) { GPU_ASSERT(false); }

typedef struct D3D11Buffer {
    ID3D11Buffer* handle;
} D3D11Buffer;

typedef struct D3D11Context {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HWND window;
    BOOL windowed;
#else
    IUnknown* window;
#endif

    uint32_t backBufferCount;
    uint32_t syncInterval;
    uint32_t presentFlags;
    IDXGISwapChain1* swapchain;
    ID3D11DeviceContext1* d3d_context;
    ID3DUserDefinedAnnotation* userDefinedAnnotation;
} D3D11Context;

typedef struct D3D11Renderer {
    IDXGIFactory2* factory;
    bool flipPresentSupported;
    bool tearingSupported;

    ID3D11Device1* device;
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11DeviceContext1* immediateContext;

    gpu_features features;
    gpu_limits limits;

    GPUContext* currentContext;
} D3D11Renderer;

/* Device functions */
static void D3D11_Destroy(GPUDevice* device)
{
    D3D11Renderer* renderer = (D3D11Renderer*)device->driverData;

    if (renderer->device)
    {
        if (renderer->currentContext) {
            gpuDeviceDestroyContext(device, renderer->currentContext);
        }

        ID3D11DeviceContext1_Release(renderer->immediateContext);

        ULONG refCount = ID3D11Device1_Release(renderer->device);
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            gpuLog(GPULogLevel_Error, "Direct3D11: There are %d unreleased references left on the device", refCount);

            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(ID3D11Device1_QueryInterface(renderer->device, &D3D11_IID_ID3D11Debug, (void**)&d3dDebug)))
            {
                ID3D11Debug_ReportLiveDeviceObjects(d3dDebug, D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
                ID3D11Debug_Release(d3dDebug);
            }
        }
#else
        (void)ref_count; // avoid warning
#endif
    }

    IDXGIFactory2_Release(renderer->factory);

#ifdef _DEBUG
    /*IDXGIDebug* dxgi_debug;
    if (SUCCEEDED(DXGIGetDebugInterface1Func(0, &agpu_IID_IDXGIDebug, (void**)&dxgi_debug)))
    {
        IDXGIDebug_ReportLiveObjects(dxgi_debug, agpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        IDXGIDebug_Release(dxgi_debug);
    }*/
#endif

    GPU_FREE(renderer);
    //GPU_FREE(device);
}

static void D3D11_BeginFrame(GPU_Renderer* driverData)
{
}

static void D3D11_EndFrame(GPU_Renderer* driverData)
{
    D3D11Renderer* renderer = (D3D11Renderer*)driverData;
    D3D11Context* context = (D3D11Context*)renderer->currentContext->backend;

    HRESULT hr = S_OK;

    ID3D11CommandList* commandList;
    VHR(ID3D11DeviceContext1_FinishCommandList(context->d3d_context, FALSE, &commandList));
    ID3D11DeviceContext1_ExecuteCommandList(renderer->immediateContext, commandList, FALSE);
    ID3D11CommandList_Release(commandList);

    if (context->swapchain)
    {
        /* Present! */
        hr = IDXGISwapChain1_Present(
            context->swapchain,
            context->syncInterval,
            context->presentFlags);
    }

    // If the device was removed either by a disconnection or a driver upgrade, we
    // must recreate all device resources.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        //HandleDeviceLost();
    }
    else
    {
        VHR(hr);

        if (!IDXGIFactory2_IsCurrent(renderer->factory))
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            //CreateFactory();
        }
    }
}

static gpu_features D3D11_GetFeatures(GPU_Renderer* driverData)
{
    D3D11Renderer* renderer = (D3D11Renderer*)driverData;
    return renderer->features;
}

static gpu_limits D3D11_GetLimits(GPU_Renderer* driverData)
{
    D3D11Renderer* renderer = (D3D11Renderer*)driverData;
    return renderer->limits;
}

static bool D3D11_ResizeContext(GPU_Renderer* driverData, GPUBackendContext* handle, uint32_t width, uint32_t height);
static GPUBackendContext* D3D11_CreateContext(GPU_Renderer* driverData, const GPUSwapChainDescriptor* descriptor)
{
    D3D11Renderer* renderer = (D3D11Renderer*)driverData;
    D3D11Context* context = (D3D11Context*)GPU_MALLOC(sizeof(D3D11Context));
    memset(context, 0, sizeof(D3D11Context));

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    context->window = (HWND)descriptor->windowHandle;
    if (!IsWindow(context->window)) {
        gpuLog(GPULogLevel_Error, "Invalid HWND handle");
        return NULL;
    }

    context->windowed = !descriptor->isFullScreen;
#else
    context->window = (IUnknown*)descriptor->windowHandle;
#endif
    context->backBufferCount = 2u;
    if (descriptor->presentationInterval == GPUPresentInterval_Default ||
        descriptor->presentationInterval == GPUPresentInterval_One)
    {
        context->syncInterval = 1;
    }
    else if (descriptor->presentationInterval == GPUPresentInterval_Two)
    {
        context->syncInterval = 2;
    }
    else if (descriptor->presentationInterval == GPUPresentInterval_Immediate)
    {
        context->syncInterval = 0;
    }

    context->presentFlags = 0;
    // Recommended to always use tearing if supported when using a sync interval of 0.
    if (context->syncInterval == 0 && renderer->tearingSupported)
    {
        context->presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    D3D11_ResizeContext(driverData, (GPUBackendContext*)context, descriptor->width, descriptor->height);
    return (GPUBackendContext*)context;
}

static void D3D11_DestroyContext(GPU_Renderer* driverData, GPUBackendContext* handle)
{
    D3D11Context* context = (D3D11Context*)handle;
    if (context->swapchain)
    {
        IDXGISwapChain1_Release(context->swapchain);
    }

    ID3DUserDefinedAnnotation_Release(context->userDefinedAnnotation);
    ID3D11DeviceContext1_Release(context->d3d_context);
    GPU_FREE(context);
}

static bool D3D11_ResizeContext(GPU_Renderer* driverData, GPUBackendContext* handle, uint32_t width, uint32_t height)
{
    D3D11Renderer* renderer = (D3D11Renderer*)driverData;
    D3D11Context* context = (D3D11Context*)handle;

    DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    DXGI_SCALING scaling = DXGI_SCALING_STRETCH;

    if (!renderer->flipPresentSupported) {
        swapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }
#else
    DXGI_SCALING scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
#endif

    UINT flags = 0;
    if (renderer->tearingSupported) {
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    if (context->swapchain)
    {
        // If the swap chain already exists, resize it.
        HRESULT hr = IDXGISwapChain1_ResizeBuffers(
            context->swapchain,
            context->backBufferCount,
            width,
            height,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            flags
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            //char buff[64] = {};
            //sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
            //    static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? renderer->device->GetDeviceRemovedReason() : hr));
            //OutputDebugStringA(buff);
#endif
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            //HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return false;
        }
        else
        {
            VHR(hr);
        }
    }
    else
    {
        // Create a descriptor for the swap chain.
        const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = width,
            .Height = height,
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = context->backBufferCount,
            .SampleDesc = {
                .Count = 1,
                .Quality = 0
            },
            .AlphaMode = scaling,
            .SwapEffect = swapEffect,
            .Flags = flags
        };


#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {
            .Windowed = context->windowed
        };

        // Create a SwapChain from a Win32 window.
        VHR(IDXGIFactory2_CreateSwapChainForHwnd(
            renderer->factory,
            (IUnknown*)renderer->device,
            context->window,
            &swapChainDesc,
            &fsSwapChainDesc,
            NULL,
            &context->swapchain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(IDXGIFactory2_MakeWindowAssociation(renderer->factory, context->window, DXGI_MWA_NO_ALT_ENTER));
#else
#endif
    }

    /* Create deferred context to use. */
    VHR(ID3D11Device1_CreateDeferredContext1(renderer->device, 0, &context->d3d_context));
    VHR(ID3D11DeviceContext1_QueryInterface(context->d3d_context, &D3D11_IID_ID3DUserDefinedAnnotation, (void**)&context->userDefinedAnnotation));

    return true;
}

/* Driver functions */
static bool D3D11_IsSupported(void) {
    static bool available_initialized;
    static bool available;

    if (available_initialized) {
        return available;
    }

    available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HMODULE dxgiDLL = LoadLibraryW(L"dxgi.dll");
    if (dxgiDLL == NULL) {
        return false;
    }

    createDXGIFactory1Func = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(dxgiDLL, "CreateDXGIFactory1");
    createDXGIFactory2Func = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiDLL, "CreateDXGIFactory2");
    dxgiGetDebugInterface1Func = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiDLL, "DXGIGetDebugInterface1");

    HMODULE d3d11DLL = LoadLibraryW(L"d3d11.dll");
    if (d3d11DLL == NULL) {
        return false;
    }

    d3d11CreateDeviceFunc = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11DLL, "D3D11CreateDevice");
    if (d3d11CreateDeviceFunc == NULL) {
        return false;
    }
#endif

    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    HRESULT hr = D3D11CreateDeviceFunc(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        s_featureLevels,
        GPU_COUNTOF(s_featureLevels),
        D3D11_SDK_VERSION,
        NULL,
        NULL,
        NULL
    );

    if (FAILED(hr)) {
        return false;
    }

    available = true;
    return true;
}

static void D3D11_GetDrawableSize(void* window, uint32_t* width, uint32_t* height)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    RECT rect;
    GetClientRect((HWND)window, &rect);

    if (width) {
        *width = (uint32_t)(rect.right - rect.left);
    }

    if (height) {
        *height = (uint32_t)(rect.bottom - rect.top);
    }
#else
#endif
}

static IDXGIAdapter1* D3D11_GetAdapter(D3D11Renderer* renderer, bool lowPower)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6;
    HRESULT hr = IDXGIFactory2_QueryInterface(renderer->factory, &D3D11_IID_IDXGIFactory6, (void**)&factory6);
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        if (lowPower) {
            gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
        }

        for (uint32_t i = 0;
            DXGI_ERROR_NOT_FOUND != IDXGIFactory6_EnumAdapterByGpuPreference(factory6, i, gpuPreference, &D3D11_IID_IDXGIAdapter1, (void**)&adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(IDXGIAdapter1_GetDesc1(adapter, &desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                IDXGIAdapter1_Release(adapter);

                // Don't select the Basic Render Driver adapter.
                continue;
            }

            break;
        }

        IDXGIFactory6_Release(factory6);
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != IDXGIFactory2_EnumAdapters1(renderer->factory, i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(IDXGIAdapter1_GetDesc1(adapter, &desc));

            // Don't select the Basic Render Driver adapter.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                IDXGIAdapter1_Release(adapter);

                continue;
            }

            break;
        }
    }

    return adapter;
}

static GPUDevice* D3D11_CreateDevice(bool debug, const GPUSwapChainDescriptor* descriptor)
{
    D3D11Renderer* renderer;

    if (!D3D11_IsSupported()) {
        return NULL;
    }

    /* Allocate and zero out the renderer */
    renderer = (D3D11Renderer*)GPU_MALLOC(sizeof(D3D11Renderer));
    memset(renderer, 0, sizeof(D3D11Renderer));

#if defined(_DEBUG)
    bool debugDXGI = false;
    if (debug)
    {
        bool canUseDebugInterface1 = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        canUseDebugInterface1 = dxgiGetDebugInterface1Func != NULL;
#endif

        IDXGIInfoQueue* dxgiInfoQueue;
        if (SUCCEEDED(canUseDebugInterface1 &&
            DXGIGetDebugInterface1Func(0, &D3D11_IID_IDXGIInfoQueue, (void**)&dxgiInfoQueue)))
        {
            debugDXGI = true;

            VHR(CreateDXGIFactory2Func(DXGI_CREATE_FACTORY_DEBUG, &D3D11_IID_IDXGIFactory2, (void**)&renderer->factory));

            IDXGIInfoQueue_SetBreakOnSeverity(dxgiInfoQueue, D3D11_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            IDXGIInfoQueue_SetBreakOnSeverity(dxgiInfoQueue, D3D11_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            IDXGIInfoQueue_AddStorageFilterEntries(dxgiInfoQueue, D3D11_DXGI_DEBUG_DXGI, &filter);
            IDXGIInfoQueue_Release(dxgiInfoQueue);
        }
    }

    if (!debugDXGI)
#endif
    {
        VHR(CreateDXGIFactory1Func(&D3D11_IID_IDXGIFactory2, (void**)&renderer->factory));
    }

    renderer->flipPresentSupported = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    // Disable FLIP if not on a supporting OS
    {
        IDXGIFactory4* factory4;
        HRESULT hr = IDXGIFactory2_QueryInterface(renderer->factory, &D3D11_IID_IDXGIFactory4, (void**)&factory4);
        if (FAILED(hr))
        {
            renderer->flipPresentSupported = false;
#ifdef _DEBUG
            OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
        }
        IDXGIFactory4_Release(factory4);
    }
#endif

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = IDXGIFactory2_QueryInterface(renderer->factory, &D3D11_IID_IDXGIFactory5, (void**)&factory5);
        if (SUCCEEDED(hr))
        {
            hr = IDXGIFactory5_CheckFeatureSupport(factory5, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            renderer->tearingSupported = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            renderer->tearingSupported = true;
        }

        IDXGIFactory5_Release(factory5);
    }

    IDXGIAdapter1* adapter = D3D11_GetAdapter(renderer, false);

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (debug && D3D11SdkLayersAvailable())
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
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        // Create the Direct3D 11 API device object and a corresponding context.
        ID3D11Device* temp_d3d_device;
        ID3D11DeviceContext* tempContext;

        HRESULT hr = E_FAIL;
        if (adapter)
        {
            hr = D3D11CreateDeviceFunc(
                (IDXGIAdapter*)adapter,
                D3D_DRIVER_TYPE_UNKNOWN,
                NULL,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &renderer->featureLevel,
                &tempContext
            );
        }
#if defined(NDEBUG)
        else
        {
            gpuLog(GPULogLevel_Error, "No Direct3D hardware device found");
            GPU_UNREACHABLE();
        }
#else
        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            hr = D3D11CreateDeviceFunc(
                NULL,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                NULL,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &renderer->featureLevel,
                &tempContext
            );

            if (SUCCEEDED(hr))
            {
                OutputDebugStringA("Direct3D Adapter - WARP\n");
            }
        }
#endif

        if (FAILED(hr)) {
            return NULL;
        }


#ifndef NDEBUG
        ID3D11Debug* d3dDebug;
        if (SUCCEEDED(ID3D11Device_QueryInterface(temp_d3d_device, &D3D11_IID_ID3D11Debug, (void**)&d3dDebug)))
        {
            ID3D11InfoQueue* d3dInfoQueue;
            if (SUCCEEDED(ID3D11Debug_QueryInterface(d3dDebug, &D3D11_IID_ID3D11InfoQueue, (void**)&d3dInfoQueue)))
            {
#ifdef _DEBUG
                ID3D11InfoQueue_SetBreakOnSeverity(d3dInfoQueue, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                ID3D11InfoQueue_SetBreakOnSeverity(d3dInfoQueue, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
#endif
                D3D11_MESSAGE_ID hide[] =
                {
                    D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                };
                D3D11_INFO_QUEUE_FILTER filter;
                memset(&filter, 0, sizeof(D3D11_INFO_QUEUE_FILTER));
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                ID3D11InfoQueue_AddStorageFilterEntries(d3dInfoQueue, &filter);
                ID3D11InfoQueue_Release(d3dInfoQueue);
            }
            ID3D11Debug_Release(d3dDebug);
        }
#endif

        VHR(ID3D11Device_QueryInterface(temp_d3d_device, &D3D11_IID_ID3D11Device1, (void**)&renderer->device));
        VHR(ID3D11DeviceContext_QueryInterface(tempContext, &D3D11_IID_ID3D11DeviceContext1, (void**)&renderer->immediateContext));
        ID3D11DeviceContext_Release(tempContext);
        ID3D11Device_Release(temp_d3d_device);
    }

    // Init features and limits.
    {
    }

    IDXGIAdapter1_Release(adapter);

    /* Create and return the Device */
    GPUDevice* device = (GPUDevice*)GPU_MALLOC(sizeof(GPUDevice));
    device->driverData = (GPU_Renderer*)renderer;
    ASSIGN_DRIVER(D3D11);

    /* Create main context and set it as current. */
    if (descriptor) {
        renderer->currentContext = gpuDeviceCreateContext(device, descriptor);
    }

    return device;
}

GPUDriver D3D11Driver = {
    GPUBackendType_Direct3D11,
    D3D11_IsSupported,
    D3D11_GetDrawableSize,
    D3D11_CreateDevice
};

#endif /* defined(GPU_DRIVER_D3D11) */
