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

#if defined(VGPU_DRIVER_D3D12)

#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include "vgpu_driver_d3d_common.h"

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>


/* Global data */
static struct {
    bool available_initialized;
    bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HMODULE dxgiDLL;
    HMODULE d3d12DLL;

    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
#endif

    vgpu_caps caps;

} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define vgpuCreateDXGIFactory2 d3d12.CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 d3d12.DXGIGetDebugInterface1
#   define vgpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#   define vgpuD3D12CreateDevice d3d12.D3D12CreateDevice
#else
#   define vgpuCreateDXGIFactory2 CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define vgpuD3D12GetDebugInterface D3D12GetDebugInterface
#   define vgpuD3D12CreateDevice D3D12CreateDevice
#endif

static bool d3d12_init(const char* app_name, const vgpu_config* config)
{
    VGPU_UNUSED(app_name);

    return true;
}

static void d3d12_shutdown(void) {
}

static bool d3d12_frame_begin(void) {
    return true;
}

static void d3d12_frame_finish(void) {
}

static void d3d12_query_caps(vgpu_caps* caps) {
    *caps = d3d12.caps;
}

/* Buffer */
static vgpu_buffer d3d12_buffer_create(const vgpu_buffer_info* info) {
    return nullptr;
    //vk_buffer* buffer = VGPU_ALLOC(vk_buffer);
    //return (vgpu_buffer)buffer;
}

static void d3d12_buffer_destroy(vgpu_buffer handle) {
}

/* Shader */
static vgpu_shader d3d12_shader_create(const vgpu_shader_info* info) {
    return nullptr;
    //vk_shader* shader = VGPU_ALLOC(vk_shader);
    //return (vgpu_shader)shader;
}

static void d3d12_shader_destroy(vgpu_shader handle) {

}

/* Texture */
static vgpu_texture d3d12_texture_create(const vgpu_texture_info* info) {
    return NULL;
}

static void d3d12_texture_destroy(vgpu_texture handle) {
}

static uint64_t d3d12_texture_get_native_handle(vgpu_texture handle) {
    return 0;
    //vk_texture* texture = (vk_texture*)handle;
    //return VOIDP_TO_U64(texture->handle);
}

/* Pipeline */
static vgpu_pipeline d3d12_pipeline_create(const vgpu_pipeline_info* info)
{
    return nullptr;
    //vk_pipeline* pipeline = VGPU_ALLOC(vk_pipeline);
    //return (vgpu_pipeline)pipeline;
}

static void d3d12_pipeline_destroy(vgpu_pipeline handle) {
}

/* Commands */
static void d3d12_push_debug_group(const char* name) {
}

static void d3d12_pop_debug_group(void) {
}

static void d3d12_insert_debug_marker(const char* name) {
}

static void d3d12_begin_render_pass(const vgpu_render_pass_info* info) {

}

static void d3d12_end_render_pass(void) {
}

static void d3d12_bind_pipeline(vgpu_pipeline handle) {
    //vk_pipeline* pipeline = (vk_pipeline*)handle;
}

static void d3d12_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex) {
}

/* Driver */
static bool d3d12_Is_supported(void)
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

    d3d12.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d12.dxgiDLL, "CreateDXGIFactory2");
    if (!d3d12.CreateDXGIFactory2)
    {
        return false;
    }

    d3d12.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d12.dxgiDLL, "DXGIGetDebugInterface1");

    d3d12.d3d12DLL = LoadLibraryA("d3d12.dll");
    if (!d3d12.d3d12DLL) {
        return false;
    }

    d3d12.D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12.d3d12DLL, "D3D12GetDebugInterface");
    d3d12.D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12.d3d12DLL, "D3D12CreateDevice");
    if (!d3d12.D3D12CreateDevice) {
        return false;
    }
#endif

    if (SUCCEEDED(vgpuD3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
        d3d12.available = true;
        return true;
    }

    return false;
};

static vgpu_renderer* d3d12_create_renderer(void)
{
    static vgpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(d3d12);
    return &renderer;
}

vgpu_driver d3d12_driver = {
    VGPU_BACKEND_TYPE_D3D12,
    d3d12_Is_supported,
    d3d12_create_renderer
};

#endif /* defined(VGPU_DRIVER_D3D12)  */
