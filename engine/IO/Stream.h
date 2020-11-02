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

#include "Core/Containers.h"
#include "Core/String.h"
#include "Core/StringId.h"

namespace alimer
{
    /// Abstract stream for reading and writing.
    class ALIMER_API Stream
    {
    public:
        Stream();
        Stream(const Stream&) = delete;
        Stream& operator=(const Stream&) = delete;

        /// Destructor.
        virtual ~Stream() = default;

        // Closes the stream.
        virtual void Close() = 0;

        /// returns the length of the stream
        virtual int64_t Length() const = 0;

        /// returns the position of the stream
        virtual int64_t Position() const = 0;

        // returns true of the stream is seakable.
        virtual bool CanSeek() const = 0;

        // returns true of the stream is readable.
        virtual bool CanRead() const = 0;

        /// returns true of the stream is writable.
        virtual bool CanWrite() const = 0;

        // Seeks the position of the stream.
        virtual int64_t Seek(int64_t position) = 0;

        /// Read  the amount of bytes into the given buffer, and returns the amount read
        virtual int64_t Read(void* buffer, int64_t length) = 0;

        // writes from the stream from the given buffer, and returns the number of bytes written.
        virtual uint64_t Write(const void* buffer, uint64_t length) = 0;

        // reads a string of a given length, or until a null terminator if -1
        String ReadString(int length = -1);

        /// Reads a single line from this stream (up until \r or \n)
        String ReadLine();

        // Reads a single line from this stream, to the given string (up until \r or \n)
        int64_t ReadLine(String& writeTo);

        /// Read a variable-length encoded unsigned integer, which can use 29 bits maximum.
        uint32_t ReadVLE();

        /// Read a 4-character file ID.
        String ReadFileID();

        /// Read a value, template version.
        template <class T> T Read()
        {
            T value;
            Read(&value, sizeof(T));
            return value;
        }

        /// Read a byte buffer.
        std::vector<uint8_t> ReadBytes(uint32_t count = 0);

        /// Read a byte buffer, with size prepended as a VLE value.
        std::vector<uint8_t> ReadBuffer();
        /// Write a four-letter file ID. If the string is not long enough, spaces will be appended.
        void WriteFileID(const String& value);
        /// Write a byte buffer, with size encoded as VLE.
        void WriteBuffer(const std::vector<uint8_t>& buffer);
        /// Write a variable-length encoded unsigned integer, which can use 29 bits maximum.
        void WriteVLE(uint32_t value);
        /// Write a text line. Char codes 13 & 10 will be automatically appended.
        void WriteLine(const String& value);

        /// Write a value, template version.
        template <class T> void Write(const T& value)
        {
            Write(&value, sizeof(T));
        }
    };

    template <> ALIMER_API bool       Stream::Read();
    template <> ALIMER_API String     Stream::Read();
    template <> ALIMER_API StringId32 Stream::Read();
    template <> ALIMER_API void       Stream::Write(const bool& value);
    template <> ALIMER_API void       Stream::Write(const String& value);
    template <> ALIMER_API void       Stream::Write(const StringId32& value);
}
