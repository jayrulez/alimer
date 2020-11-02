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

#include "IO/FileSystem.h"
#include "IO/FileStream.h"
#include "PlatformIncl.h"

namespace alimer::File
{
    bool Exists(const FilePath& path)
    {
#ifdef _WIN32
        DWORD attributes = GetFileAttributesW(ToUtf16(path).c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
            return false;
#else
        struct stat st;
        if (stat(path.c_str(), &st) || st.st_mode & S_IFDIR)
            return false;
#endif

        return true;
    }

    std::string ReadAllText(const FilePath& path)
    {
        if (!Exists(path))
            return kEmptyString;

        FileStream  stream(path, FileMode::Read);
        std::string result = stream.ReadString();
        return result;
    }

    std::vector<uint8_t> ReadAllBytes(const FilePath& path)
    {
        if (!Exists(path))
            return {};

        FileStream           stream(path, FileMode::Read);
        std::vector<uint8_t> result = stream.ReadBytes();
        return result;
    }
}

namespace alimer::Directory
{
    bool Exists(const FilePath& path)
    {
#ifdef _WIN32
        DWORD attributes = GetFileAttributesW(ToUtf16(path).c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat st;
        if (stat(path.c_str(), &st) || !(st.st_mode & S_IFDIR))
            return false;

        return true;
#endif
    }
}
