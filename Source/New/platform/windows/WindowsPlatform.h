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

#pragma once

#include "platform/Platform.h"

struct HINSTANCE__;
struct HWND__;

namespace Alimer
{
    using HANDLE = void*;
    using HINSTANCE = HINSTANCE__*;
    using HWND = HWND__*;

    class ALIMER_API WindowsPlatform final : public PlatformBase
    {
    private:
        static HINSTANCE hInstance;

    public:
        static const wchar_t AppWindowClass[];

        static bool Init(HINSTANCE hInstance);
        static void Shutdown();

        /// Return the current platform name.
        static const char* GetName();

        /// Return the current platform ID.
        static PlatformId GetId();

        /// Return the current platform family.
        static PlatformFamily GetFamily();

        static HINSTANCE GetHInstance();
    };

    std::string ToUtf8(const std::wstring& wstr);
    std::wstring ToUtf16(const std::string& str);
}
