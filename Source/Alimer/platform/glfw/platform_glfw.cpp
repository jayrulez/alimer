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

#include "platform/platform.h"
#include "core/log.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Alimer
{
    namespace platform
    {
        static void on_glfw_error(int code, const char* description)
        {
            LOGE("GLFW Error {} - {}", description, code);
        }

        bool init(bool opengl)
        {
            glfwSetErrorCallback(on_glfw_error);
#ifdef __APPLE__
            glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif
            if (!glfwInit())
            {
                return false;
            }

            if (opengl)
            {
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifdef _DEBUG
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
                glfwWindowHint(GLFW_CONTEXT_NO_ERROR, false);
#endif
                //glfwWindowHint(GLFW_SAMPLES, msaa);
                glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
            }
            else
            {
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            }

            return true;
        }

        void shutdown() noexcept
        {
            glfwTerminate();
        }

        void pump_events() noexcept
        {
            glfwPollEvents();
        }
    }
}
