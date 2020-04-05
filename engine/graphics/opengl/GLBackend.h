//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "graphics/Types.h"

#if defined(_WIN32) // Windows
#   define ALIMER_OPENGL_INTERFACE_WGL 1
#elif defined(__APPLE__) // macOS, iOS, tvOS
#   include <TargetConditionals.h>
#   if TARGET_OS_WATCH // watchOS
#       error "Apple Watch is not supported"
#   elif TARGET_OS_IOS || TARGET_OS_TV // iOS, tvOS
#       define ALIMER_OPENGLES 1
#       define ALIMER_OPENGL_INTERFACE_EAGL 1
#   elif TARGET_OS_MAC // any other Apple OS (check this last because it is defined for all Apple platforms)
#       define ALIMER_OPENGL_INTERFACE_CGL 1
#   endif
#elif defined(__ANDROID__) // Android (check this before Linux because __linux__ is also defined for Android)
#   define ALIMER_OPENGLES 1
#   define ALIMER_OPENGL_INTERFACE_EGL 1
#elif defined(__linux__) // Linux
#   if defined(__x86_64__) || defined(__i386__) // x86 Linux
#       define ALIMER_OPENGL_INTERFACE_GLX 1
#   elif defined(__arm64__) || defined(__aarch64__) || defined(__arm__) // ARM Linux
#       define ALIMER_OPENGLES 1
#       define ALIMER_OPENGL_INTERFACE_EGL 1
#   else
#       error "Unsupported architecture"
#   endif
#elif defined(__EMSCRIPTEN__) // Emscripten
#   define ALIMER_OPENGLES 1
#   define ALIMER_OPENGL_INTERFACE_EGL 1
#else
#   error "Unsupported platform"
#endif

#if ALIMER_OPENGLES
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
}
