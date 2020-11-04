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

#include "Core/Memory.h"
#include <spdlog/fmt/fmt.h>

namespace alimer
{
    using String = std::string;

    static constexpr uint32_t CONVERSION_BUFFER_LENGTH = 128u;

    extern const std::string kEmptyString;
    extern const char kWhitespaceASCII[];

    enum class WhitespaceHandling
    {
        KeepWhitespace,
        TrimWhitespace
    };

    enum class SplitResult
    {
        All,
        NonEmpty,
    };

    ALIMER_API std::string TrimString(const std::string& input, const std::string& trimChars);

    ALIMER_API std::vector<std::string> SplitString(const std::string& input, const std::string& delimiters, WhitespaceHandling whitespace,
                                                    SplitResult resultType);

#ifdef _WIN32
    ALIMER_API std::string ToUtf8(const wchar_t* wstr, size_t len);
    ALIMER_API std::string ToUtf8(const std::wstring& wstr);

    ALIMER_API std::wstring ToUtf16(const char* str, size_t len);
    ALIMER_API std::wstring ToUtf16(const std::string& str);
#endif
}
