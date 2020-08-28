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

#include "application.h"
#include "gpu_driver.h"

#if defined(__EMSCRIPTEN__)
#   include <GLES3/gl3.h>
#   include <GLES2/gl2ext.h>
#   include <GL/gl.h>
#   include <GL/glext.h>
#else
#   include "glad/glad.h"
#endif

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

        /* Global data */
        static struct {
            Pool<GL_Texture, GL_Texture::MAX_COUNT> textures;
            Pool<GL_Buffer, GL_Buffer::MAX_COUNT> buffers;
        } d3d11;

        /* Renderer functions */
        static bool gl_init(const Config* config)
        {
            // Init pools.
            d3d11.textures.Init();
            d3d11.buffers.Init();

            return true;
        }

        static void gl_shutdown(void)
        {
        }

        /* Driver functions */
        static bool opengl_supported(void)
        {
            return true;
        }

        static Renderer* gl_create_renderer(void)
        {
            static Renderer renderer = { nullptr };
            renderer.init = gl_init;
            renderer.shutdown = gl_shutdown;
            return &renderer;
        }

        Driver gl_driver = {
            BackendType::OpenGL,
            opengl_supported,
            gl_create_renderer
        };
    }
}

#endif /* defined(ALIMER_ENABLE_OPENGL) */
