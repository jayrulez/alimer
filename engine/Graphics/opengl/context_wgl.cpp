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

#include "Core/Log.h"
#include "Core/Vector.h"
#include "graphics_opengl.h"
#include "GL/wglext.h"

namespace alimer
{
    namespace graphics
    {
        namespace
        {
            constexpr LPCWSTR tempWindowClassName = L"TempWindow";

            LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
            {
                return DefWindowProc(window, msg, wParam, lParam);
            }

            class TempContext final
            {
            public:
                TempContext()
                {
                    HINSTANCE hInstance = GetModuleHandleW(nullptr);

                    WNDCLASSW wc;
                    wc.style = CS_OWNDC;
                    wc.lpfnWndProc = windowProc;
                    wc.cbClsExtra = 0;
                    wc.cbWndExtra = 0;
                    wc.hInstance = hInstance;
                    wc.hIcon = 0;
                    wc.hCursor = 0;
                    wc.hbrBackground = 0;
                    wc.lpszMenuName = nullptr;
                    wc.lpszClassName = tempWindowClassName;

                    windowClass = RegisterClassW(&wc);
                    if (!windowClass)
                        LOG_ERROR("Wgl: Failed to register window class");

                    window = CreateWindowW(tempWindowClassName, L"TempWindow", 0,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        0, 0, hInstance, 0);
                    if (!window)
                        LOG_ERROR("Wgl: Failed to create window");

                    deviceContext = GetDC(window);
                    if (!deviceContext)
                        LOG_ERROR("Wgl: Failed to get device context");

                    PIXELFORMATDESCRIPTOR pfd;
                    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
                    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
                    pfd.nVersion = 1;
                    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
                    pfd.iPixelType = PFD_TYPE_RGBA;
                    pfd.cColorBits = 24;
                    pfd.iLayerType = PFD_MAIN_PLANE;

                    const int pixelFormat = ChoosePixelFormat(deviceContext, &pfd);

                    if (!pixelFormat)
                        LOG_ERROR("Wgl: Failed to choose pixel format");

                    if (!SetPixelFormat(deviceContext, pixelFormat, &pfd))
                        LOG_ERROR("Wgl: Failed to set pixel format");

                    renderContext = wglCreateContext(deviceContext);
                    if (!renderContext)
                        LOG_ERROR("Wgl: Failed to create OpenGL rendering context");

                    if (!wglMakeCurrent(deviceContext, renderContext))
                        LOG_ERROR("Wgl: Failed to set OpenGL rendering context");
                }

                ~TempContext()
                {
                    if (renderContext)
                    {
                        if (wglGetCurrentContext() == renderContext)
                            wglMakeCurrent(deviceContext, nullptr);
                        wglDeleteContext(renderContext);
                    }

                    if (window)
                    {
                        if (deviceContext)
                            ReleaseDC(window, deviceContext);

                        DestroyWindow(window);
                    }

                    if (windowClass)
                        UnregisterClassW(tempWindowClassName, GetModuleHandleW(nullptr));
                }

                TempContext(const TempContext&) = delete;
                TempContext& operator=(const TempContext&) = delete;
                TempContext(TempContext&&) = delete;
                TempContext& operator=(TempContext&&) = delete;

            private:
                ATOM windowClass = 0;
                HWND window = 0;
                HDC deviceContext = 0;
                HGLRC renderContext = 0;
            };
        }

        static HMODULE opengl32dll = nullptr;

        struct GLContextImpl {
            HWND hwnd;
            HDC hdc;
            HGLRC renderContext;
            GLVersion version;
        };

        GLContext CreateGLContext(bool validation, void* windowHandle, PixelFormat colorFormat, PixelFormat depthStencilFormat, uint32_t sampleCount)
        {
            if (!opengl32dll) {
                opengl32dll = LoadLibraryW(L"opengl32.dll");
            }

            if (!opengl32dll) {
                return nullptr;
            }

            GLContextImpl* context = new GLContextImpl();
            TempContext tempContext;

            context->hwnd = reinterpret_cast<HWND>(windowHandle);
            if (!IsWindow(context->hwnd)) {
                return nullptr;
            }

            context->hdc = GetDC(context->hwnd);
            if (!context->hdc)
                LOG_ERROR("Wgl: Failed to get window's device context");

            int pixelFormat = 0;

            PIXELFORMATDESCRIPTOR pfd;
            memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
            pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_GENERIC_ACCELERATED;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = 24;
            if (depthStencilFormat == PixelFormat::Depth32Float) {
                pfd.cDepthBits = 24;
            }
            else  if (depthStencilFormat == PixelFormat::Depth24PlusStencil8) {
                pfd.cDepthBits = 24;
                pfd.cStencilBits = 8;
            }
            pfd.iLayerType = PFD_MAIN_PLANE;

            const bool srgb = false;

            PFNWGLCHOOSEPIXELFORMATARBPROC  wglChoosePixelFormatProc = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
            PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsProc = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));

            if (wglChoosePixelFormatProc)
            {
                const int attributeList[] =
                {
                    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                    WGL_COLOR_BITS_ARB, 24,
                    WGL_ALPHA_BITS_ARB, 8,
                    WGL_DEPTH_BITS_ARB, pfd.cDepthBits,
                    WGL_STENCIL_BITS_ARB, pfd.cStencilBits,
                    WGL_SAMPLE_BUFFERS_ARB, sampleCount > 0 ? 1 : 0,
                    WGL_SAMPLES_ARB, static_cast<int>(sampleCount),
                    WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, srgb ? 1 : 0,
                    0,
                };

                UINT numFormats;

                if (!wglChoosePixelFormatProc(context->hdc, attributeList, nullptr, 1, &pixelFormat, &numFormats))
                    LOG_ERROR("Wgl: Failed to choose pixel format");
            }
            else
            {
                pixelFormat = ChoosePixelFormat(context->hdc, &pfd);
                if (!pixelFormat)
                    LOG_ERROR("Wgl: Failed to choose pixel format");
            }

            if (!SetPixelFormat(context->hdc, pixelFormat, &pfd))
                LOG_ERROR("Wgl: Failed to set pixel format");

            if (wglCreateContextAttribsProc)
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

                    Vector<int> contextAttribs = {
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
                        contextAttribs.Push(WGL_CONTEXT_FLAGS_ARB);
                        contextAttribs.Push(flags);
                    }

                    if (mask)
                    {
                        contextAttribs.Push(WGL_CONTEXT_PROFILE_MASK_ARB);
                        contextAttribs.Push(mask);
                    }

                    contextAttribs.Push(0);

                    context->renderContext = wglCreateContextAttribsProc(context->hdc, 0, contextAttribs.Data());

                    if (context->renderContext)
                    {
                        context->version = createVersion;
                        LOG_INFO("Create OpenGL context with %u.%u", createVersion.major, createVersion.minor);
                        break;
                    }
                }
            }
            else {
                context->renderContext = wglCreateContext(context->hdc);
            }

            if (!context->renderContext)
                LOG_ERROR("Wgl: Failed to create OpenGL rendering context");

            if (!wglMakeCurrent(context->hdc, context->renderContext))
                LOG_ERROR("Wgl: Failed to set OpenGL rendering context");

            return context;
        }

        void DestroyGLContext(GLContext context)
        {
            if (context->renderContext)
            {
                if (wglGetCurrentContext() == context->renderContext)
                {
                    wglMakeCurrent(context->hdc, nullptr);
                }

                wglDeleteContext(context->renderContext);
            }

            if (context->hdc)
            {
                ReleaseDC(context->hwnd, context->hdc);
            }

            /*if (opengl32dll) {
                FreeLibrary(opengl32dll);
                opengl32dll = nullptr;
            }*/

            delete context; context = nullptr;
        }

        void* GetGLProcAddress(GLContext context, const char* name) {
            void* ptr = wglGetProcAddress(name);
            if (!ptr) {
                ptr = ::GetProcAddress(opengl32dll, name);
            }

            return ptr;
        }

        void MakeCurrent(GLContext context) {
            if (!wglMakeCurrent(context->hdc, context->renderContext))
                LOG_ERROR("Wgl: Failed to set OpenGL rendering context");
        }

        void SwapBuffers(GLContext context) {
            if (!::SwapBuffers(context->hdc)) {
                LOG_ERROR("Wgl: Failed to swap buffers");
            }
        }
    }
}
