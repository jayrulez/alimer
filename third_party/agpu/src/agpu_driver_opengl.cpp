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

#if ALIMER_GLES
#   include "GLES/gl.h"
#   include "GLES2/gl2.h"
#   include "GLES2/gl2ext.h"
#   include "GLES3/gl3.h"
#   include "GLES3/gl31.h"
#   include "GLES3/gl32.h"
#else
#   include "GL/glcorearb.h"
#   include "GL/glext.h"
#endif

namespace agpu
{
    /* Global data */
    static struct {
        bool available_initialized;
        bool available;

        Caps caps;
        GLuint default_framebuffer;
        GLuint default_vao;
    } gl;

    /* Device/Renderer */
    static bool gl_init(InitFlags flags, void* windowHandle)
    {
        return true;
    }

    static void gl_Shutdown(void)
    {
        memset(&gl, 0, sizeof(gl));
    }

    static Swapchain gl_GetPrimarySwapchain(void)
    {
        return kInvalidSwapchain;
    }

    static FrameOpResult gl_BeginFrame(Swapchain swapchain)
    {
        return FrameOpResult::Success;
    }

    static FrameOpResult gl_EndFrame(Swapchain swapchain, EndFrameFlags flags)
    {
        return FrameOpResult::Success;
    }

    static const Caps* gl_QueryCaps(void) {
        return &gl.caps;
    }

    static Swapchain gl_CreateSwapchain(void* windowHandle)
    {
        return kInvalidSwapchain;
    }

    static void gl_DestroySwapchain(Swapchain handle)
    {

    }

    static Texture gl_GetCurrentTexture(Swapchain handle)
    {
        return kInvalidTexture;
    }

    static BufferHandle gl_CreateBuffer(uint32_t count, uint32_t stride, const void* initialData)
    {
        return kInvalidBuffer;
    }

    static void gl_DestroyBuffer(BufferHandle handle)
    {

    }

    static Texture gl_CreateTexture(uint32_t width, uint32_t height, PixelFormat format, uint32_t mipLevels, intptr_t handle)
    {
        return kInvalidTexture;
    }

    static void gl_DestroyTexture(Texture handle)
    {

    }

    static void gl_PushDebugGroup(const char* name)
    {
        AGPU_UNUSED(name);
    }

    static void gl_PopDebugGroup(void)
    {
    }

    static void gl_InsertDebugMarker(const char* name)
    {
        AGPU_UNUSED(name);
    }

    static void gl_BeginRenderPass(const RenderPassDescription* renderPass)
    {
    }

    static void gl_EndRenderPass(void)
    {

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

    static Renderer* gl_create_renderer(void)
    {
        static Renderer renderer = { 0 };
        ASSIGN_DRIVER(gl);
        return &renderer;
    }

    Driver GL_Driver = {
        BackendType::OpenGL,
        gl_is_supported,
        gl_create_renderer
    };
}

#endif /* defined(AGPU_DRIVER_OPENGL)  */
