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
#include "Core/Containers.h"

#ifdef _WIN32
#   include "PlatformIncl.h"
#else
#   include <unistd.h>
#   ifdef __linux__
#       include <linux/limits.h>
#   endif
#endif

namespace Alimer
{
    const String EMPTY_STRING{};

#ifdef _WIN32
    String ToUtf8(const wchar_t* wstr, size_t len)
    {
        Vector<char> char_buffer;
        auto ret = WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
        if (ret < 0)
            return "";
        char_buffer.resize(ret);
        WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(len), char_buffer.data(), static_cast<int>(char_buffer.size()), nullptr, nullptr);
        return String(char_buffer.data(), char_buffer.size());
    }

    String ToUtf8(const WString& wstr)
    {
        return ToUtf8(wstr.data(), wstr.size());
    }

    WString ToUtf16(const char* str, size_t len)
    {
        Vector<wchar_t> wchar_buffer;
        auto ret = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(len), nullptr, 0);
        if (ret < 0)
            return L"";
        wchar_buffer.resize(ret);
        MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(len), wchar_buffer.data(), static_cast<int>(wchar_buffer.size()));
        return WString(wchar_buffer.data(), wchar_buffer.size());
    }

    WString ToUtf16(const String& str)
    {
        return ToUtf16(str.data(), str.size());
    }
#endif
}
