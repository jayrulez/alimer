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

#if defined(VGPU_DRIVER_OPENGL)&& defined(TODO)
#include "vgpu_driver.h"

#if defined(_WIN32) // Windows
#   define VGPU_INTERFACE_WGL 1
#elif defined(__ANDROID__) // Android (check this before Linux because __linux__ is also defined for Android)
#   define VGPU_GLES 1
#   define VGPU_INTERFACE_EGL 1
#elif defined(__linux__) // Linux
#   if defined(__x86_64__) || defined(__i386__) // x86 Linux
#       define VGPU_INTERFACE_GLX 1
#   elif defined(__arm64__) || defined(__aarch64__) || defined(__arm__) // ARM Linux
#       define VGPU_GLES 1
#       define VGPU_INTERFACE_EGL 1
#   else
#       error "Unsupported architecture"
#   endif
#elif defined(__EMSCRIPTEN__) // Emscripten
#   define VGPU_GLES 1
#   define VGPU_WEBGL 1
#   define VGPU_INTERFACE_EGL 1
#   include <emscripten.h>
#   include <emscripten/html5.h>
#else
#  error "Unsupported platform"
#endif

#if VGPU_GLES
#   include "GLES/gl.h"
#   include "GLES2/gl2.h"
#   include "GLES2/gl2ext.h"
#   include "GLES3/gl3.h"
#   include "GLES3/gl31.h"
#   include "GLES3/gl32.h"
#else
#   include "GL/glcorearb.h"
#   include "GL/glext.h"
#   if defined(VGPU_INTERFACE_WGL)
#       include "GL/wglext.h"
#   elif defined(VGPU_INTERFACE_GLX)
#       include "GL/glcorearb.h"
#       include <GL/glx.h>
#   elif defined(VGPU_INTERFACE_EGL)
#       include "EGL/egl.h"
#       include "EGL/eglext.h"
#   endif
#endif

typedef struct gl_texture {
    GLuint id;
    uint32_t width;
    uint32_t height;
} gl_texture;

typedef struct gl_framebuffer {
    GLuint id;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
} gl_framebuffer;

typedef struct gl_swapchain {
    GLuint id;
} gl_swapchain;

/* Global data */
static struct {
    bool available_initialized;
    bool available;

    vgpu_caps caps;
} gl;

static bool gl_init(const vgpu_config* config) {
    return true;
}

static void gl_shutdown(void) {
}

static void gl_get_caps(vgpu_caps* caps) {
    *caps = gl.caps;
}

static bool gl_frame_begin(void) {
    return true;
}

static void gl_frame_end(void) {

}

static void gl_insertDebugMarker(const char* name)
{
}

static void gl_pushDebugGroup(const char* name)
{
}

static void gl_popDebugGroup(void)
{
}

static void gl_render_begin(vgpu_framebuffer framebuffer) {
}

static void gl_render_finish(void) {

}

/* Texture */
static vgpu_texture gl_texture_create(const vgpu_texture_info* info) {
    gl_texture* texture = VGPU_ALLOC_HANDLE(gl_texture);
    return (vgpu_texture)texture;
}

static void gl_texture_destroy(vgpu_texture handle) {
    gl_texture* texture = (gl_texture*)handle;
    //GL_CHECK(glDeleteTextures(1, &texture->id));
    VGPU_FREE(texture);
}

static uint32_t gl_texture_get_width(vgpu_texture handle, uint32_t mipLevel) {
    gl_texture* texture = (gl_texture*)handle;
    return _vgpu_max(1, texture->width >> mipLevel);
}

static uint32_t gl_texture_get_height(vgpu_texture handle, uint32_t mipLevel) {
    gl_texture* texture = (gl_texture*)handle;
    return _vgpu_max(1, texture->height >> mipLevel);
}

/* Framebuffer */
static vgpu_framebuffer gl_framebuffer_create(const vgpu_framebuffer_info* info) {
    gl_framebuffer* framebuffer = VGPU_ALLOC_HANDLE(gl_framebuffer);
    return (vgpu_framebuffer)framebuffer;
}

static void gl_framebuffer_destroy(vgpu_framebuffer handle) {
    gl_framebuffer* framebuffer = (gl_framebuffer*)handle;
    VGPU_FREE(framebuffer);
}

/* Swapchain */
static vgpu_swapchain gl_swapchain_create(uintptr_t window_handle, vgpu_texture_format color_format, vgpu_texture_format depth_stencil_format) {
    gl_swapchain* swapchain = VGPU_ALLOC_HANDLE(gl_swapchain);
    return (vgpu_swapchain)swapchain;
}

static void gl_swapchain_destroy(vgpu_swapchain handle) {
    gl_swapchain* swapchain = (gl_swapchain*)handle;
    VGPU_FREE(swapchain);
}

static void gl_swapchain_resize(vgpu_swapchain handle, uint32_t width, uint32_t height) {
    gl_swapchain* swapchain = (gl_swapchain*)handle;
}

static void gl_swapchain_present(vgpu_swapchain handle) {
    gl_swapchain* swapchain = (gl_swapchain*)handle;
}

/* Driver functions */
static bool gl_is_supported(void) {

    if (gl.available_initialized) {
        return gl.available;
    }

    gl.available_initialized = true;
    gl.available = true;
    return true;
}

static vgpu_context* gl_create_context(void) {
    static vgpu_context context = { 0 };
    ASSIGN_DRIVER(gl);
    return &context;
}

vgpu_driver gl_driver = {
    VGPU_BACKEND_TYPE_OPENGL,
    gl_is_supported,
    gl_create_context
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
