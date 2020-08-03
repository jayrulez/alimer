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

#include "Application/Application.h"
#include "Core/Window.h"
#include "Core/Log.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <shellapi.h>

namespace alimer
{
    namespace
    {
        std::string WStringToString(const std::wstring& wstr)
        {
            if (wstr.empty())
            {
                return {};
            }

            auto wstr_len = static_cast<int>(wstr.size());
            auto str_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len, NULL, 0, NULL, NULL);

            std::string str(str_len, 0);
            WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len, &str[0], str_len, NULL, NULL);

            return str;
        }

        static void OnGLFWError(int code, const char* description) {
            LOGE("GLFW  Error (code %d): %s", code, description);
        }
    }

    void Application::PlatformConstruct()
    {
        LPWSTR* argv;
        int     argc;
        argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        // Ignore the first argument containing the application full path.
        for (int i = 0; i < argc; i++)
        {
            args.push_back(WStringToString(argv[i]));
        }

        if (argv) {
            LocalFree(argv);
        }

        if (AllocConsole()) {
            FILE* fp;
            freopen_s(&fp, "conin$", "r", stdin);
            freopen_s(&fp, "conout$", "w", stdout);
            freopen_s(&fp, "conout$", "w", stderr);
        }

        glfwSetErrorCallback(OnGLFWError);
        if (!glfwInit())
        {
            LOGE("GLFW couldn't be initialized.");
        }
    }

    void Application::PlatformDestroy()
    {
        glfwTerminate();
    }

    int Application::PlatformRun()
    {
#if !defined(__GNUC__) && _HAS_EXCEPTIONS
        try
#endif
        {
            running = true;

            InitBeforeRun();

            // Main message loop
            while (running)
            {
                glfwPollEvents();
                if (window->ShouldClose())
                {
                    running = false;
                    break;
                }

                Tick();
            }

            return EXIT_SUCCESS;
        }
#if !defined(__GNUC__) && _HAS_EXCEPTIONS
        catch (std::bad_alloc&)
        {
            //ErrorDialog(GetTypeName(), "An out-of-memory error occurred. The application will now exit.");
            return EXIT_FAILURE;
        }
#endif
    }
}
