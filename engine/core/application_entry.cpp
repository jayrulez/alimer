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

#include "core/preprocessor.h"
#include "core/application.h"
#include "platform/platform.h"
#include "io/path.h"

#if ALIMER_PLATFORM_WINDOWS
#include "platform/windows/windows_private.h"
#include <shellapi.h>
#endif

namespace Alimer
{
    // Make sure this is linked in.
    void application_dummy()
    {
    }
}

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
#ifdef _WIN32
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int argc;
    wchar_t** wide_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<char*> argv_buffer(argc + 1);
    char** argv = nullptr;
    std::vector<std::string> argv_strings(argc);

    if (wide_argv)
    {
        argv = argv_buffer.data();
        for (int i = 0; i < argc; i++)
        {
            argv_strings[i] = Alimer::Path::to_utf8(wide_argv[i]);
            argv_buffer[i] = const_cast<char*>(argv_strings[i].c_str());
        }
    }
#endif

    Alimer::Platform::set_arguments({ argv + 1, argv + argc });

    int ret = Alimer::application_main(Alimer::application_create, argc, argv);
    return ret;
}
