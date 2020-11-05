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
#include "PlatformIncl.h"
#else
#include <unistd.h>
#ifdef __linux__
#include <linux/limits.h>
#endif
#endif

namespace alimer
{
    const std::string kEmptyString{};
    const char kWhitespaceASCII[] = " \f\n\r\t\v";

    std::string TrimString(const std::string& input, const std::string& trimChars)
    {
        auto begin = input.find_first_not_of(trimChars);
        if (begin == std::string::npos)
        {
            return "";
        }

        std::string::size_type end = input.find_last_not_of(trimChars);
        if (end == std::string::npos)
        {
            return input.substr(begin);
        }

        return input.substr(begin, end - begin + 1);
    }

    std::vector<std::string> SplitString(const std::string& input, const std::string& delimiters, WhitespaceHandling whitespace,
                                         SplitResult resultType)
    {
        std::vector<std::string> result;
        if (input.empty())
        {
            return result;
        }

        std::string::size_type start = 0;
        while (start != std::string::npos)
        {
            auto end = input.find_first_of(delimiters, start);

            std::string piece;
            if (end == std::string::npos)
            {
                piece = input.substr(start);
                start = std::string::npos;
            }
            else
            {
                piece = input.substr(start, end - start);
                start = end + 1;
            }

            if (whitespace == WhitespaceHandling::TrimWhitespace)
            {
                piece = TrimString(piece, kWhitespaceASCII);
            }

            if (resultType == SplitResult::All || !piece.empty())
            {
                result.push_back(std::move(piece));
            }
        }

        return result;
    }

#ifdef _WIN32
    std::string ToUtf8(const wchar_t* wstr, size_t len)
    {
        std::vector<char> char_buffer;
        auto ret = WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
        if (ret < 0)
            return kEmptyString;
        char_buffer.resize(ret);
        WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(len), char_buffer.data(), static_cast<int>(char_buffer.size()), nullptr,
                            nullptr);
        return std::string(char_buffer.data(), char_buffer.size());
    }

    std::string ToUtf8(const std::wstring& wstr) { return ToUtf8(wstr.data(), wstr.size()); }

    std::wstring ToUtf16(const char* str, size_t len)
    {
        std::vector<wchar_t> wchar_buffer;
        auto ret = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(len), nullptr, 0);
        if (ret < 0)
            return L"";
        wchar_buffer.resize(ret);
        MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(len), wchar_buffer.data(), static_cast<int>(wchar_buffer.size()));
        return std::wstring(wchar_buffer.data(), wchar_buffer.size());
    }

    std::wstring ToUtf16(const std::string& str) { return ToUtf16(str.data(), str.size()); }
#endif
}
