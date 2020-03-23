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

#include "core/StringId.h"
#include "core/String.h"
#include "core/Hash.h"
#include <inttypes.h> // PRIx64

namespace alimer
{
    const StringId32 StringId32::Zero;
    const StringId64 StringId64::Zero;

    /* StringId32 */
    StringId32::StringId32(const char* str) noexcept
    {
        value = murmur32(str, (uint32_t)strlen(str), 0);
    }

    StringId32::StringId32(const std::string& str) noexcept
    {
        value = murmur32(str.c_str(), (uint32_t)str.length(), 0);
    }

    std::string StringId32::ToString() const
    {
        char tempBuffer[CONVERSION_BUFFER_LENGTH];
        sprintf(tempBuffer, "%08X", value);
        return std::string(tempBuffer);
    }

    /* StringId64 */
    StringId64::StringId64(const char* str) noexcept
    {
        value = murmur64(str, (uint64_t)strlen(str), 0);
    }

    StringId64::StringId64(const std::string& str) noexcept
    {
        value = murmur64(str.c_str(), (uint64_t)str.length(), 0);
    }

    std::string StringId64::ToString() const
    {
        char tempBuffer[CONVERSION_BUFFER_LENGTH];
        sprintf(tempBuffer, "%16" PRIx64, value);
        return std::string(tempBuffer);
    }
}
