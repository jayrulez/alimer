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

#include "GLFW_AppContext.h"
#include "GLFW_Window.h"
#include "Application/Application.h"
#include "Core/Log.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Alimer
{
    static void OnGlfwError(int code, const char* description) {
        ALIMER_LOGERROR(description);
    }

    GLFW_AppContext::GLFW_AppContext(Application* app)
        : AppContext(app, true)
    {
    }

    GLFW_AppContext::~GLFW_AppContext()
    {
        glfwTerminate();
    }

    void GLFW_AppContext::Run()
    {
        glfwSetErrorCallback(OnGlfwError);

#ifdef __APPLE__
        glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif

        if (!glfwInit()) {
            ALIMER_LOGERROR("Failed to initialize GLFW");
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        uint32_t width, height;
        _app->GetDefaultWindowSize(&width, &height);
        _mainWindow.reset(new GLFW_Window("Alimer", width, height, WindowStyle::Default));

        Initialize();

        // Main message loop
        while (!_mainWindow->ShouldClose() && !exitRequested)
        {
            // Check for window messages to process.
            glfwPollEvents();
            _app->Tick();
        }
    }
}
