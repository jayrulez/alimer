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


#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <EASTL/string.h>

namespace Alimer
{
    static inline eastl::string ToUtf8(const eastl::wstring& wstr)
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

    static inline eastl::wstring ToUtf16(const eastl::string& str)
    {
        if (str.empty())
        {
            return {};
        }

        auto input_str_length = static_cast<int>(str.size());
        auto wstr_len = MultiByteToWideChar(CP_UTF8, 0, &str[0], input_str_length, nullptr, 0);

        eastl::wstring wstr(wstr_len, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], input_str_length, &wstr[0], wstr_len);
        return wstr;
    }
}
