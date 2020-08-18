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

#if defined(AGPU_DRIVER_OPENGL)
#include "agpu_driver.h"

#if defined(_WIN32) // Windows
#   define AGPU_INTERFACE_WGL 1
#elif defined(__APPLE__) // macOS, iOS, tvOS
#   include <TargetConditionals.h>
#   if TARGET_OS_WATCH // watchOS
#       error "Apple Watch is not supported"
#   elif TARGET_OS_IOS || TARGET_OS_TV // iOS, tvOS
#       define ALIMER_GLES 1
#       define AGPU_INTERFACE_EAGL 1
#   elif TARGET_OS_MAC // any other Apple OS (check this last because it is defined for all Apple platforms)
#       define AGPU_INTERFACE_CGL 1
#   endif
#elif defined(__ANDROID__) // Android (check this before Linux because __linux__ is also defined for Android)
#   define ALIMER_GLES 1
#   define GFX_EGL 1
#elif defined(__linux__) // Linux
#   if defined(__x86_64__) || defined(__i386__) // x86 Linux
#       define AGPU_INTERFACE_GLX 1
#   elif defined(__arm64__) || defined(__aarch64__) || defined(__arm__) // ARM Linux
#       define ALIMER_GLES 1
#       define AGPU_INTERFACE_EGL 1
#   else
#       error "Unsupported architecture"
#   endif
#elif defined(__EMSCRIPTEN__) // Emscripten
#   define ALIMER_GLES 1
#   define AGPU_INTERFACE_EGL 1
#else
#   error "Unsupported platform"
#endif

#if defined(AGPU_WEBGL)
#   include <GLES3/gl3.h>
#   include <GLES2/gl2ext.h>
#   include <GL/gl.h>
#   include <GL/glext.h>
#else
#   include <glad/glad.h>
#endif

/* Global data */
static struct {
    bool available_initialized;
    bool available;

    agpu_device_caps caps;
    GLuint default_framebuffer;
    GLuint default_vao;
} gl;


/* Device/Renderer */
static bool gl_init(const agpu_init_info* info)
{
    return true;
}

static void gl_shutdown(void)
{
}

static agpu_context gl_create_context(const agpu_context_info* info) {
    return NULL;
}

static void gl_destroy_context(agpu_context context) {

}

static void gl_set_context(agpu_context context) {

}

static void gl_begin_frame(void) {
}

static void gl_end_frame(void) {
}

static agpu_device_caps gl_query_caps(void) {
    return gl.caps;
}

static agpu_texture_format_info gl_query_texture_format_info(agpu_texture_format format) {
    agpu_texture_format_info info;
    memset(&info, 0, sizeof(info));
    return info;
}

/* Driver */
static bool gl_is_supported(void)
{
    if (gl.available_initialized) {
        return gl.available;
    }

    gl.available_initialized = true;
    gl.available = true;
    return true;
};

static agpu_renderer* gl_create_renderer(void)
{
    static agpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(gl);
    return &renderer;
}

agpu_driver gl_driver = {
    AGPU_BACKEND_TYPE_OPENGL,
    gl_is_supported,
    gl_create_renderer
};

#endif /* defined(AGPU_DRIVER_OPENGL)  */
