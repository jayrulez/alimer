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

#include "os_glfw.h"

namespace alimer
{
    namespace os
    {
        static void on_glfw_error(int code, const char* description) {
            //ALIMER_THROW(description);
        }

        auto init() -> bool
        {
            glfwSetErrorCallback(on_glfw_error);

#ifdef __APPLE__
            glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif

            int result = glfwInit();
            if (result == GLFW_FALSE)
            {
                //TODO_ERROR_HANDLER(result);
                return false;
            }

            return true;
        }

        void shutdown() noexcept
        {
            glfwTerminate();
        }

        std::string get_clipboard_text() noexcept
        {
            const char* text = glfwGetClipboardString(nullptr);
            if (text)
            {
                return text;
            }

            return {};
        }

        void set_clipboard_text(const std::string& text)
        {
            glfwSetClipboardString(nullptr, text.c_str());
        }
    }
}
