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
#include "Core/Platform.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <shellapi.h>
#endif

using namespace eastl;

namespace alimer
{
    // Make sure this is linked in.
    void application_dummy()
    {
    }

#ifdef _WIN32
    static inline string ToUtf8(const wstring& wstr)
    {
        if (wstr.empty())
        {
            return {};
        }

        auto input_str_length = static_cast<int>(wstr.size());
        auto str_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], input_str_length, nullptr, 0, nullptr, nullptr);

        eastl::string str(str_len, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], input_str_length, &str[0], str_len, nullptr, nullptr);

        return str;
    }
#endif
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

    alimer::Platform::SetArguments(args);
    alimer::Platform::OpenConsole();
#endif

    auto app = unique_ptr<alimer::Game>(alimer::application_create(args));
    app->Run();
    return EXIT_SUCCESS;
}

#endif /* !defined(ALIMER_EXPORTS) */
