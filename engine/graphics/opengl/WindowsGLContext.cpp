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

#include "GLBackend.h"
#if ALIMER_OPENGL_INTERFACE_WGL
#include "WindowsGLContext.h"
#include "GL/glcorearb.h"
#include "GL/glext.h"
#include "GL/wglext.h"
#include "core/Log.h"
#include <vector>

namespace alimer
{
    namespace
    {
        constexpr LPCWSTR TEMP_WINDOW_CLASS_NAME = L"TEMP_GL_WINDOW";

        LRESULT CALLBACK DummyWindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            return DefWindowProcW(window, msg, wParam, lParam);
        }

        class TempContext final
        {
        public:
            TempContext(HINSTANCE hInstance_)
                : hInstance(hInstance_)
            {
                WNDCLASSW wc;
                wc.style = CS_OWNDC;
                wc.lpfnWndProc = DummyWindowProc;
                wc.cbClsExtra = 0;
                wc.cbWndExtra = 0;
                wc.hInstance = hInstance_;
                wc.hIcon = 0;
                wc.hCursor = 0;
                wc.hbrBackground = 0;
                wc.lpszMenuName = nullptr;
                wc.lpszClassName = TEMP_WINDOW_CLASS_NAME;

                windowClass = RegisterClassW(&wc);
                if (!windowClass)
                {
                    ALIMER_LOGERROR("Wgl: Failed to register window class");
                    return;
                }

                hwnd = CreateWindowW(TEMP_WINDOW_CLASS_NAME, L"TempWindow", 0,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    0, 0, hInstance_, 0);
                if (!hwnd)
                {
                    ALIMER_LOGERROR("Wgl: Failed to create window");
                    return;
                }

                hdc = GetDC(hwnd);
                if (!hdc)
                {
                    ALIMER_LOGERROR("Wgl: Failed to get device context");
                    return;
                }

                PIXELFORMATDESCRIPTOR pfd = {};
                pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
                pfd.nVersion = 1;
                pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
                pfd.iPixelType = PFD_TYPE_RGBA;
                pfd.cColorBits = 32;
                pfd.cRedBits = 0;
                pfd.cRedShift = 0;
                pfd.cGreenBits = 0;
                pfd.cGreenShift = 0;
                pfd.cBlueBits = 0;
                pfd.cBlueShift = 0;
                pfd.cAlphaBits = 0;
                pfd.cAlphaShift = 0;
                pfd.cAccumBits = 0;
                pfd.cAccumRedBits = 0;
                pfd.cAccumGreenBits = 0;
                pfd.cAccumBlueBits = 0;
                pfd.cAccumAlphaBits = 0;
                pfd.cDepthBits = 0;
                pfd.cStencilBits = 0;
                pfd.cAuxBuffers = 0;
                pfd.iLayerType = PFD_MAIN_PLANE;
                pfd.bReserved = 0;
                pfd.dwLayerMask = 0;
                pfd.dwVisibleMask = 0;
                pfd.dwDamageMask = 0;

                const int pixelFormat = ChoosePixelFormat(hdc, &pfd);
                if (!pixelFormat)
                {
                    ALIMER_LOGERROR("Wgl: Failed to choose pixel format");
                }

                SetPixelFormat(hdc, pixelFormat, &pfd);

                context = wglCreateContext(hdc);
                if (!context)
                {
                    ALIMER_LOGERROR("Wgl: Failed to create OpenGL rendering context");
                    return;
                }

                if (!wglMakeCurrent(hdc, context))
                {
                    ALIMER_LOGERROR("Wgl: Failed to set OpenGL rendering context");
                    return;
                }
            }

            ~TempContext()
            {
                if (context)
                {
                    if (wglGetCurrentContext() == context)
                        wglMakeCurrent(hdc, nullptr);
                    wglDeleteContext(context);
                }

                if (hwnd)
                {
                    if (hdc)
                        ReleaseDC(hwnd, hdc);

                    DestroyWindow(hwnd);
                }

                if (windowClass)
                    UnregisterClassW(TEMP_WINDOW_CLASS_NAME, hInstance);
            }

            TempContext(const TempContext&) = delete;
            TempContext& operator=(const TempContext&) = delete;
            TempContext(TempContext&&) = delete;
            TempContext& operator=(TempContext&&) = delete;

        private:
            HINSTANCE hInstance;
            ATOM windowClass = 0;
            HWND hwnd = nullptr;
            HDC hdc = nullptr;
            HGLRC context = nullptr;
        };
    }

    WindowsGLContext::WindowsGLContext(HINSTANCE hInstance_, HWND hwnd_, bool validation, bool depth, bool stencil, bool srgb, uint32_t sampleCount)
        : hInstance(hInstance_)
        , hwnd(hwnd_)
        , hdc(GetDC(hwnd_))
        , opengl32dll(LoadLibraryW(L"opengl32.dll"))
    {
        TempContext tempContext(hInstance_);

        auto wglChoosePixelFormat = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
        auto wglCreateContextAttribs = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        PIXELFORMATDESCRIPTOR pfd;
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_GENERIC_ACCELERATED;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cRedBits = 0;
        pfd.cRedShift = 0;
        pfd.cGreenBits = 0;
        pfd.cGreenShift = 0;
        pfd.cBlueBits = 0;
        pfd.cBlueShift = 0;
        pfd.cAlphaBits = 0;
        pfd.cAlphaShift = 0;
        pfd.cAccumBits = 0;
        pfd.cAccumRedBits = 0;
        pfd.cAccumGreenBits = 0;
        pfd.cAccumBlueBits = 0;
        pfd.cAccumAlphaBits = 0;
        pfd.cDepthBits = depth ? 24 : 0;
        pfd.cStencilBits = stencil ? 8 : 0;
        pfd.cAuxBuffers = 0;
        pfd.iLayerType = PFD_MAIN_PLANE;
        pfd.bReserved = 0;
        pfd.dwLayerMask = 0;
        pfd.dwVisibleMask = 0;
        pfd.dwDamageMask = 0;

        int pixelFormat = 0;
        if (wglChoosePixelFormat)
        {
            int attribList[] =
            {
                WGL_SAMPLE_BUFFERS_ARB, sampleCount > 0 ? 1 : 0,
                WGL_SAMPLES_ARB, static_cast<int>(sampleCount),
                WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, srgb ? 1 : 0,
                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                WGL_COLOR_BITS_ARB, 32,
                WGL_ALPHA_BITS_ARB, 8,
                WGL_DEPTH_BITS_ARB, depth ? 24 : 0,
                WGL_STENCIL_BITS_ARB, stencil ? 8 : 0,
                0,
            };

            UINT numFormats;

            if (!wglChoosePixelFormat(hdc, attribList, nullptr, 1, &pixelFormat, &numFormats) ||
                numFormats == 0)
            {
                bool valid = false;
                if (sampleCount > 0)
                {
                    //ALIMER_LOGWARN("Failed to choose pixel format with WGL_SAMPLES_ARB == %d. Attempting to fallback to lower samples setting.", sampleCount);
                    while (sampleCount > 0)
                    {
                        sampleCount /= 2;
                        attribList[1] = sampleCount;
                        attribList[3] = sampleCount > 0 ? 1 : 0;
                        if (wglChoosePixelFormat(hdc, attribList, NULL, 1, &pixelFormat, &numFormats) && numFormats > 0)
                        {
                            valid = true;
                            //LOGW("Found pixel format with WGL_SAMPLES_ARB == %d.", sampleCount);
                            break;
                        }
                    }
                }
            }
            else
            {
                pixelFormat = ChoosePixelFormat(hdc, &pfd);
                if (!pixelFormat)
                {
                    ALIMER_LOGERROR("Failed to choose pixel format");
                    return;
                }
            }

            if (!SetPixelFormat(hdc, pixelFormat, &pfd))
            {
                ALIMER_LOGERROR("Failed to set pixel format");
                return;
            }

            // Now create actual context
            if (wglCreateContextAttribs)
            {
                GLVersion versions[] =
                {
                    {4, 6, GLProfile::Core, GLSLShaderVersion::GLSL_460},
                    {4, 5, GLProfile::Core, GLSLShaderVersion::GLSL_450},
                    {4, 4, GLProfile::Core, GLSLShaderVersion::GLSL_440},
                    {4, 3, GLProfile::Core, GLSLShaderVersion::GLSL_430},
                    {4, 2, GLProfile::Core, GLSLShaderVersion::GLSL_420},
                    {4, 1, GLProfile::Core, GLSLShaderVersion::GLSL_410},
                    {4, 0, GLProfile::Core, GLSLShaderVersion::GLSL_400},
                    {3, 3, GLProfile::Core, GLSLShaderVersion::GLSL_330},
                    {3, 2, GLProfile::ES, GLSLShaderVersion::ESSL_310},
                    {3, 1, GLProfile::ES, GLSLShaderVersion::ESSL_310},
                    {3, 0, GLProfile::ES, GLSLShaderVersion::ESSL_300},
                    {2, 0, GLProfile::ES, GLSLShaderVersion::ESSL_100}
                };

                for (auto createVersion : versions)
                {
                    int mask = 0;
                    int flags = 0;

                    std::vector<int> contextAttribs = {
                        WGL_CONTEXT_MAJOR_VERSION_ARB, createVersion.major,
                        WGL_CONTEXT_MINOR_VERSION_ARB, createVersion.minor
                    };

                    switch (createVersion.profile)
                    {
                    case GLProfile::Core:
                        mask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
                        break;

                    case GLProfile::Compatibility:
                        mask |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
                        break;

                    case GLProfile::ES:
                        mask |= WGL_CONTEXT_ES2_PROFILE_BIT_EXT;
                        break;

                    default:
                        break;
                    }

                    if (validation)
                        flags |= WGL_CONTEXT_DEBUG_BIT_ARB;

                    const bool forward = false;
                    if (forward)
                        flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

                    if (flags)
                    {
                        contextAttribs.push_back(WGL_CONTEXT_FLAGS_ARB);
                        contextAttribs.push_back(flags);
                    }

                    if (mask)
                    {
                        contextAttribs.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
                        contextAttribs.push_back(mask);
                    }

                    contextAttribs.push_back(0);

                    context = wglCreateContextAttribs(hdc, 0, contextAttribs.data());

                    if (context)
                    {
                        version = createVersion;
                        break;
                    }
                }
            }
            else
            {
                context = wglCreateContext(hdc);
            }

            if (!wglMakeCurrent(hdc, context))
            {
                ALIMER_LOGERROR("Failed to set OpenGL rendering context");
                return;
            }
        }
    }

    WindowsGLContext::~WindowsGLContext()
    {
        if (context)
        {
            if (wglGetCurrentContext() == context)
            {
                wglMakeCurrent(hdc, nullptr);
            }

            wglDeleteContext(context);
        }

        if (hdc)
        {
            ReleaseDC(hwnd, hdc);
        }

        if (opengl32dll) {
            FreeLibrary(opengl32dll);
            opengl32dll = nullptr;
        }
    }

    void* WindowsGLContext::GetGLProcAddress(const char* name)
    {
        void* ptr = wglGetProcAddress(name);
        if (!ptr) {
            ptr = ::GetProcAddress(opengl32dll, name);
        }

        return ptr;
    }

    void WindowsGLContext::MakeCurrent()
    {
        if (!wglMakeCurrent(hdc, context))
        {
            ALIMER_LOGERROR("Failed to set OpenGL rendering context");
        }
    }

    void WindowsGLContext::SwapBuffers()
    {
        ::SwapBuffers(hdc);
    }

    GLContext* GLContext::Create(void* nativeHandle, bool validation, bool depth, bool stencil, bool srgb, uint32_t sampleCount)
    {
        HINSTANCE hInstance = GetModuleHandleW(nullptr);
        if (!hInstance)
        {
            ALIMER_LOGERROR("Failed to get module handle");
            return nullptr;
        }

        HWND hwnd = reinterpret_cast<HWND>(nativeHandle);
        if (!IsWindow(hwnd)) {
            return nullptr;
        }

        return new WindowsGLContext(hInstance, hwnd, validation, depth, stencil, srgb, sampleCount);
    }

}
#endif
