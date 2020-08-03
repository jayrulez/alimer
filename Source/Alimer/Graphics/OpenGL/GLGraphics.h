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

#include "Graphics/Graphics.h"

#if defined(_WIN32) // Windows
#   define GFX_WGL 1
#elif defined(__APPLE__) // macOS, iOS, tvOS
#   include <TargetConditionals.h>
#   if TARGET_OS_WATCH // watchOS
#       error "Apple Watch is not supported"
#   elif TARGET_OS_IOS || TARGET_OS_TV // iOS, tvOS
#       define ALIMER_GLES 1
#       define GFX_EAGL 1
#   elif TARGET_OS_MAC // any other Apple OS (check this last because it is defined for all Apple platforms)
#       define GFX_CGL 1
#   endif
#elif defined(__ANDROID__) // Android (check this before Linux because __linux__ is also defined for Android)
#   define ALIMER_GLES 1
#   define GFX_EGL 1
#elif defined(__linux__) // Linux
#   if defined(__x86_64__) || defined(__i386__) // x86 Linux
#       define GFX_GLX 1
#   elif defined(__arm64__) || defined(__aarch64__) || defined(__arm__) // ARM Linux
#       define ALIMER_GLES 1
#       define GFX_EGL 1
#   else
#       error "Unsupported architecture"
#   endif
#elif defined(__EMSCRIPTEN__) // Emscripten
#   define ALIMER_GLES 1
#   define GFX_EGL 1
#else
#   error "Unsupported platform"
#endif

#if defined(ALIMER_GLES)
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
    class GLGraphics final : public Graphics
    {
    public:
        GLGraphics(Window& window, const GraphicsSettings& settings);
        ~GLGraphics();

    private:
        void InitProc();
        void WaitForGPU() override;
        bool BeginFrame() override;
        void EndFrame() override;

        template<typename FT>
        void LoadGLFunction(FT& functionPointer, const char* functionName)
        {
            functionPointer = reinterpret_cast<FT> (GetGLProcAddress(functionName));
        }

        void* GetGLProcAddress(const char* procName);

        PFNGLGETERRORPROC glGetError = nullptr;
        PFNGLGETINTEGERVPROC glGetIntegerv = nullptr;
        PFNGLGETSTRINGPROC glGetString = nullptr;
        PFNGLENABLEPROC glEnable = nullptr;
        PFNGLDISABLEPROC glDisable = nullptr;
        PFNGLVIEWPORTPROC glViewport = nullptr;
        PFNGLCLEARBUFFERFVPROC glClearBufferfv = nullptr;
    };
}

