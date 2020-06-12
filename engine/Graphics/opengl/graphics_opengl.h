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

#pragma once

#include "graphics/graphics_driver.h"

#if defined(_WIN32) // Windows
#   define OPENGL_INTERFACE_WGL
#elif defined(__ANDROID__) // Android (check this before Linux because __linux__ is also defined for Android)
#   define GRAPHICS_GLES
#   define OPENGL_INTERFACE_EGL
#elif defined(__linux__) // Linux
#   if defined(__x86_64__) || defined(__i386__) // x86 Linux
#       define OPENGL_INTERFACE_GLX
#   elif defined(__arm64__) || defined(__aarch64__) || defined(__arm__) // ARM Linux
#       define GRAPHICS_GLES
#       define OPENGL_INTERFACE_EGL
#   else
#       error "Unsupported architecture"
#   endif
#elif defined(__EMSCRIPTEN__) // Emscripten
#   define GRAPHICS_GLES 1
#   define OPENG_WEBGL 1
#   define OPENG_INTERFACE_EGL 1
#   include <emscripten.h>
#   include <emscripten/html5.h>
#else
#  error "Unsupported platform"
#endif

#if defined(GRAPHICS_GLES)
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

namespace alimer
{
    namespace graphics
    {
        typedef struct GLContextImpl* GLContext;

        enum class GLSLShaderVersion : uint32_t
        {
            // GLSL
            GLSL_330,		// GL 3.3+
            GLSL_400,		// GL 4.0+
            GLSL_410,		// GL 4.1+
            GLSL_420,		// GL 4.2+
            GLSL_430,		// GL 4.3+
            GLSL_440,		// GL 4.4+
            GLSL_450,		// GL 4.5+
            GLSL_460,		// GL 4.6+

            // ESSL
            ESSL_100,	// GL ES 2.0+
            ESSL_300,	// GL ES 3.0+
            ESSL_310,	// GL ES 3.1+
        };

        enum class GLProfile : uint32_t
        {
            Core,
            Compatibility,
            ES
        };

        struct GLVersion
        {
            int major;
            int minor;
            GLProfile profile;
            GLSLShaderVersion shaderVersion;
        };

        GLContext CreateGLContext(bool validation, void* windowHandle, PixelFormat colorFormat, PixelFormat depthStencilFormat, uint32_t sampleCount);
        void DestroyGLContext(GLContext context);
        void* GetGLProcAddress(GLContext context, const char* name);
        template<typename FT>
        void GetGLProcAddress(GLContext context, FT& functionPointer, const char* functionName)
        {
            functionPointer = reinterpret_cast<FT> (GetGLProcAddress(context, functionName));
        }

        void MakeCurrent(GLContext context);
        void SwapBuffers(GLContext context);
    }
}
