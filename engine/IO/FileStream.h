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

#include "IO/Stream.h"

namespace Alimer
{
    enum class FileMode
    {
        Read,
        Write,
        ReadWrite
    };

    /// Stream for reading and writing to file.
    class ALIMER_API FileStream : public Stream
    {
    public:
        FileStream();
        FileStream(const String& path, FileMode mode = FileMode::Read);
        FileStream(FileStream&& src) noexcept;
        FileStream& operator=(FileStream&& src) noexcept;
        virtual ~FileStream();

        virtual void Close() override;
        virtual int64_t Length() const override;
        virtual int64_t Position() const override;
        virtual bool CanSeek() const override;
        virtual bool CanRead() const override;
        virtual bool CanWrite() const  override;
        virtual int64_t Seek(int64_t position)  override;
        virtual int64_t Read(void* buffer, int64_t length)  override;
        virtual uint64_t Write(const void* buffer, uint64_t length) override;

    private:
        FileMode mode;
        void* handle;
        int64_t length;
    };

}
