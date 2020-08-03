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

#include "GLGraphics.h"
#include "Core/Window.h"

#if defined(ALIMER_GLFW)
#   define GLFW_INCLUDE_NONE
#   include <GLFW/glfw3.h>
#endif

namespace alimer
{
#if defined(ALIMER_GLFW)
    void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        //glViewport(0, 0, width, height);
    }
#endif
#define _GL_CHECK_ERROR() { ALIMER_ASSERT(glGetError() == GL_NO_ERROR); }

    GLGraphics::GLGraphics(Window& window, const GraphicsSettings& settings)
        : Graphics(window)
    {
        InitProc();

        _GL_CHECK_ERROR();
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFramebuffer);
        glGenVertexArrays(1, &defaultVao);
        glBindVertexArray(defaultVao);
        _GL_CHECK_ERROR();
    }

    GLGraphics::~GLGraphics()
    {
        glDeleteVertexArrays(1, &defaultVao);
        _GL_CHECK_ERROR();
    }

    void GLGraphics::InitProc()
    {
#define LOAD_FUNCTION(functionName) LoadGLFunction(functionName, #functionName)

        LOAD_FUNCTION(glGetError);
        LOAD_FUNCTION(glGetIntegerv);
        LOAD_FUNCTION(glGetString);
        LOAD_FUNCTION(glEnable);
        LOAD_FUNCTION(glDisable);
        LOAD_FUNCTION(glViewport);
        LOAD_FUNCTION(glClearBufferfv);

        LOAD_FUNCTION(glGenVertexArrays);
        LOAD_FUNCTION(glBindVertexArray);
        LOAD_FUNCTION(glDeleteVertexArrays);
#undef LOAD_FUNCTION
    }

    void* GLGraphics::GetGLProcAddress(const char* procName)
    {
#if defined(ALIMER_GLFW)
        return glfwGetProcAddress(procName);
#endif
    }

    bool GLGraphics::BeginFrame()
    {
        float clearColor[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
        glClearBufferfv(GL_COLOR, 0, clearColor);
        _GL_CHECK_ERROR();

        return true;
    }

    void GLGraphics::EndFrame()
    {
#if defined(ALIMER_GLFW)
        glfwSwapBuffers((GLFWwindow*)window.GetWindow());
#endif
    }
}
