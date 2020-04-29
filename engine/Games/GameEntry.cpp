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

#if !defined(ALIMER_EXPORTS)

#include "Games/Game.h"
#include "core/Platform.h"
#include "core/String.h"

#ifdef _WIN32
    #include <foundation/windows.h>
    #include <shellapi.h>
#endif

using namespace std;

namespace alimer
{
    // Make sure this is linked in.
    void ApplicationDummy()
    {
    }
}

#ifdef _WIN32
int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
#ifdef _WIN32
    ALIMER_UNUSED(hInstance);
    ALIMER_UNUSED(hPrevInstance);
    ALIMER_UNUSED(lpCmdLine);
    ALIMER_UNUSED(nCmdShow);

    LPWSTR* argv;
    int     argc;
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    // Ignore the first argument containing the application full path
    vector<wstring> arg_strings(argv + 1, argv + argc);
    vector<string>  args;

    for (auto& arg : arg_strings)
    {
        args.push_back(alimer::ToUtf8(arg));
    }

    alimer::Platform::OpenConsole();
#endif

    auto app = unique_ptr<alimer::Game>(alimer::ApplicationCreate(args));
    app->Run();
    return EXIT_SUCCESS;
}

#endif /* !defined(ALIMER_EXPORTS) */
