//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#if defined(ALIMER_ENABLE_OPENGL)

#include "platform/platform.h"
#include "application.h"
#include "gpu_driver.h"

#if defined(__EMSCRIPTEN__)
#   include "GLES2/gl2.h"
#   include "GLES2/gl2ext.h"
#   include "GLES3/gl3.h"
#else
#   include "GL/glcorearb.h"
#   include "GL/glext.h"
#endif

#define GL_FOREACH(X)\
    X(glGetError, GLGETERROR)\
    X(glGetIntegerv, GLGETINTEGERV)\
    X(glGetString, GLGETSTRING)\
    X(glGetStringI, GLGETSTRINGI)\
    X(glEnable, GLENABLE)\
    X(glDisable, GLDISABLE)\
    X(glCullFace, GLCULLFACE)\
    X(glFrontFace, GLFRONTFACE)\
    X(glPolygonOffset, GLPOLYGONOFFSET)\
    X(glDepthMask, GLDEPTHMASK)\
    X(glDepthFunc, GLDEPTHFUNC)\
    X(glColorMask, GLCOLORMASK)\
    X(glClearBufferfv, GLCLEARBUFFERFV)\
    X(glClearBufferfi, GLCLEARBUFFERFI)\
    X(glClearBufferiv, GLCLEARBUFFERIV)

#define GL_DECLARE(fn, upper) static PFN##upper##PROC fn;
#define GL_LOAD(fn, upper) fn = (PFN##upper##PROC) Platform::get_gl_proc_address(#fn);

namespace Alimer
{
    namespace graphics
    {
        struct GL_Texture
        {
            enum { MAX_COUNT = 8192 };

            GLuint handle;
        };

        struct GL_Buffer
        {
            enum { MAX_COUNT = 8192 };

            GLuint handle;
        };

        GL_FOREACH(GL_DECLARE);

        /* Global data */
        static struct {
            Pool<GL_Texture, GL_Texture::MAX_COUNT> textures;
            Pool<GL_Buffer, GL_Buffer::MAX_COUNT> buffers;
        } d3d11;

        /* Renderer functions */
        static bool gl_init(const Config* config)
        {
#if !defined(__EMSCRIPTEN__)
            GL_FOREACH(GL_LOAD);
#endif

            // Init pools.
            d3d11.textures.Init();
            d3d11.buffers.Init();

            return true;
        }

        static void gl_shutdown(void)
        {
        }

        static void gl_begin_frame(void)
        {
            float clear_color[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
            glClearBufferfv(GL_COLOR, 0, clear_color);
        }

        static void gl_end_frame(void)
        {
            // Swap buffers
            Platform::swap_buffers();
        }

        /* Driver functions */
        static bool gl_supported(void)
        {
            return true;
        }

        static Renderer* gl_create_renderer(void)
        {
            static Renderer renderer = { nullptr };
            ASSIGN_DRIVER(gl);
            return &renderer;
        }

        Driver gl_driver = {
            BackendType::OpenGL,
            gl_supported,
            gl_create_renderer
        };
    }
}

#endif /* defined(ALIMER_ENABLE_OPENGL) */
