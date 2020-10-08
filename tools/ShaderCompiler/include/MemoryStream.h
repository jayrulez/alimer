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

#pragma once

#include <string>
#include <fstream>
#include <iostream>

namespace ShaderCompiler
{
    class FileStream final
    {
    public:
        FileStream(const std::string& filePath);
        ~FileStream();

        template<class T>
        void Write(const T& value)
        {
            Write(&value, sizeof(T));
        }

        /// Write an 8-bit integer.
        void WriteByte(int8_t value);
        void WriteFileID(const std::string& value);
        size_t Write(const void* buffer, size_t length) { return WriteFrom(buffer, length); }

        int64_t Length() const { return length; }
        //int64_t position() const { return static_cast<int64_t>(stream.tellp()); }
        //int64_t seek(int64_t seekTo) { return m_position = (seekTo < 0 ? 0 : (seekTo > m_length ? m_length : seekTo)); }
        bool is_open() const { return stream.is_open(); }
        bool is_readable() const { return true; }
        bool is_writable() const { return true; }
        void close() { stream.close(); }

    protected:
        virtual int64_t read_into(void* ptr, int64_t length);
        virtual size_t WriteFrom(const void* ptr, size_t length);

    private:
        std::ofstream stream;
        int64_t length;
    };

}
