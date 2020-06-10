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

#if defined(VGPU_DRIVER_OPENGL)
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

static bool gl_init(const vgpu_config* config) {
    return true;
}

static void gl_shutdown(void) {
}

static bool gl_frame_begin(void) {
    return true;
}

static void gl_frame_end(void) {

}

/* Driver functions */
static bool gl_is_supported(void) {
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
