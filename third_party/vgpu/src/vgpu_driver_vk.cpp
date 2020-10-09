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

#if defined(VGPU_DRIVER_VULKAN)
#include "vgpu_driver.h"

#if defined(__ANDROID__)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#elif _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// Functions that don't require an instance
#define VGPU_FOREACH_ANONYMOUS(X)\
  X(vkCreateInstance)

#define VGPU_DECLARE(fn) static PFN_##fn fn;

// Declare function pointers
VGPU_FOREACH_ANONYMOUS(VGPU_DECLARE);

struct vk_buffer {
};

struct vk_shader {
};

struct vk_pipeline {
};

/* Global data */
static struct {
    bool available_initialized;
    bool available;

    agpu_config config;
    agpu_caps caps;
} vk;

/* Device/Renderer */
static bool vk_init(const char* app_name, const agpu_config* config)
{
    memcpy(&vk.config, config, sizeof(vk.config));

    /* Log some info */
    agpu_log(AGPU_LOG_LEVEL_INFO, "AGPU driver: Vulkan");
    return true;
}

static void vk_shutdown(void)
{
}

static bool vk_frame_begin(void)
{
    return true;
}

static void vk_frame_finish(void)
{
}

static void vk_query_caps(agpu_caps* caps) {
    *caps = vk.caps;
}

/* Buffer */
static agpu_buffer vk_buffer_create(const agpu_buffer_info* info)
{
    vk_buffer* buffer = new vk_buffer();
    return (agpu_buffer)buffer;
}

static void vk_buffer_destroy(agpu_buffer handle)
{
}

/* Shader */
static agpu_shader vk_shader_create(const agpu_shader_info* info)
{
    vk_shader* shader = new vk_shader();
    return (agpu_shader)shader;
}

static void vk_shader_destroy(agpu_shader handle)
{

}

/* Texture */
static agpu_texture vk_texture_create(const agpu_texture_info* info)
{
    return nullptr;
}

static void vk_texture_destroy(agpu_texture handle)
{
}

/* Pipeline */
static agpu_pipeline vk_pipeline_create(const agpu_pipeline_info* info)
{
    vk_pipeline* pipeline = new vk_pipeline();
    return (agpu_pipeline)pipeline;
}

static void vk_pipeline_destroy(agpu_pipeline handle)
{
}

/* Commands */
static void vk_push_debug_group(const char* name)
{
}

static void vk_pop_debug_group(void)
{
}

static void vk_insert_debug_marker(const char* name)
{
}

static void vk_begin_render_pass(const agpu_render_pass_info* info)
{

}

static void vk_end_render_pass(void)
{
}

static void vk_bind_pipeline(agpu_pipeline handle)
{
    vk_pipeline* pipeline = (vk_pipeline*)handle;
}

static void vk_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex)
{
}


/* Driver */
static bool vk_is_supported(void)
{
    if (vk.available_initialized) {
        return vk.available;
    }

    vk.available_initialized = true;
    vk.available = true;
    return true;
};

static agpu_renderer* vk_create_renderer(void)
{
    static agpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(vk);
    return &renderer;
}

agpu_driver vulkan_driver = {
    VGPU_BACKEND_TYPE_VULKAN,
    vk_is_supported,
    vk_create_renderer
};

#endif /* defined(VGPU_DRIVER_VULKAN)  */
