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

#include "Core/String.h"
#include "Core/Hash.h"
#include "Core/Array.h"

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

namespace Alimer
{
    const std::string EMPTY_STRING{};

#ifdef _WIN32
    std::string ToUtf8(const wchar_t* wstr, size_t len)
    {
        Array<char> char_buffer;
        auto ret = WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
        if (ret < 0)
            return EMPTY_STRING;
        char_buffer.Resize(ret);
        WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(len), char_buffer.Data(), static_cast<int>(char_buffer.Size()), nullptr, nullptr);
        return std::string(char_buffer.Data(), char_buffer.Size());
    }

    std::string ToUtf8(const std::wstring& wstr)
    {
        return ToUtf8(wstr.data(), wstr.size());
    }

    std::wstring ToUtf16(const char* str, size_t len)
    {
        Array<wchar_t> wchar_buffer;
        auto ret = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(len), nullptr, 0);
        if (ret < 0)
            return L"";
        wchar_buffer.Resize(ret);
        MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(len), wchar_buffer.Data(), static_cast<int>(wchar_buffer.Size()));
        return std::wstring(wchar_buffer.Data(), wchar_buffer.Size());
    }

    std::wstring ToUtf16(const std::string& str)
    {
        return ToUtf16(str.data(), str.size());
    }
#endif
}
