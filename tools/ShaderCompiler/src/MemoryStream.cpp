//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#include "MemoryStream.h"
#include <assert.h>
#include <algorithm>

namespace ShaderCompiler
{
    FileStream::FileStream(const std::string& filePath)
        : stream(filePath,std::ios_base::binary)
    {
        length = stream.tellp();
    }

    FileStream::~FileStream()
    {
        close();
    }

    void FileStream::WriteByte(int8_t value)
    {
        Write(&value, sizeof(int8_t));
    }

    void FileStream::WriteFileID(const std::string& value)
    {
        size_t length = std::min<size_t>(value.length(), 4U);

        Write(value.c_str(), length);
        for (size_t i = value.length(); i < 4; ++i)
        {
            WriteByte(' ');
        }

    }


    int64_t FileStream::read_into(void* ptr, int64_t len)
    {
        /*if (len < 0 || ptr == nullptr)
            return 0;

        if (len > m_length - m_position)
            len = m_length - m_position;

        memcpy(ptr, m_data + m_position, (size_t)len);
        m_position += len;*/
        return len;
    }

    size_t FileStream::WriteFrom(const void* ptr, size_t length)
    {
        stream.write(reinterpret_cast<const char*>(ptr), length);
        return length;
    }
}
