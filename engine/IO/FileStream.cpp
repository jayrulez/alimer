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

#include "IO/FileStream.h"
#include "PlatformIncl.h"

namespace Alimer
{
#ifdef _WIN32
    static const wchar_t* openModes[] =
    {
        L"rb",
        L"wb",
        L"rb+"
    };
#else
    static const char* openModes[] =
    {
        "rb",
        "wb",
        "rb+"
    };
#endif

    FileStream::FileStream()
        : handle(nullptr)
        , mode(FileMode::Read)
        , length(0)
    {
    }

    FileStream::FileStream(const std::string& path, FileMode mode)
        : mode{ mode }
    {
#ifdef _WIN32
        handle = _wfopen(ToUtf16(path).c_str(), openModes[static_cast<uint32>(mode)]);
#else
        handle = fopen(path.c_str(), openModes[static_cast<uint32>(mode)]);
#endif

        fseek((FILE*)handle, 0, SEEK_END);
        length = ftell((FILE*)handle);
        fseek((FILE*)handle, 0, SEEK_SET);
    }

    FileStream::FileStream(FileStream&& src) noexcept
    {
        handle = src.handle;
        mode = src.mode;
        length = src.length;
        src.handle = nullptr;
    }

    FileStream& FileStream::operator=(FileStream&& src) noexcept
    {
        handle = src.handle;
        mode = src.mode;
        length = src.length;
        src.handle = nullptr;
        return *this;
    }

    FileStream::~FileStream()
    {
        Close();
    }

    void FileStream::Close()
    {
        if (handle != nullptr)
        {
            fclose((FILE*)handle);
        }

        handle = nullptr;
        mode = FileMode::Read;
    }

    int64_t FileStream::Length() const
    {
        return length;
    }

    int64_t FileStream::Position() const
    {
        return ftell((FILE*)handle);
    }

    bool FileStream::CanSeek() const
    {
        return handle != nullptr;
    }

    bool FileStream::CanRead() const
    {
        return handle != nullptr && (mode == FileMode::ReadWrite || mode == FileMode::Read);
    }

    bool FileStream::CanWrite() const
    {
        return handle != nullptr && (mode == FileMode::ReadWrite || mode == FileMode::Write);
    }

    int64_t FileStream::Seek(int64_t position)
    {
        fseek((FILE*)handle, (long)position, SEEK_SET);
        return position;
    }

    int64_t FileStream::Read(void* buffer, int64_t length)
    {
        size_t ret = fread(buffer, length, 1, (FILE*)handle);
        return static_cast<int64_t>(ret);
    }

    uint64_t FileStream::Write(const void* buffer, uint64_t length)
    {
        size_t ret = fwrite(buffer, length, 1, (FILE*)handle);
        return static_cast<int64_t>(ret);
    }
}
