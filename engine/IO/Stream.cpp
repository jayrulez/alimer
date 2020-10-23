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

#include "IO/Stream.h"

namespace Alimer
{
    Stream::Stream()
    {

    }

    String Stream::ReadString(int length)
    {
        if (length >= 0)
        {
            String str;
            str.resize(length);
            Read(str.data(), length);
            str[length] = '\0';
            return str;
        }
        else
        {
            String str;
            char next;
            while (Read(&next, 1) && next != '\0')
            {
                str += next;
            }

            return str;
        }
    }

    String Stream::ReadLine()
    {
        String string;
        ReadLine(string);
        return string;

    }

    int64_t Stream::ReadLine(String& writeTo)
    {
        const int bufferSize = 512;
        char buffer[bufferSize];

        int64_t pos = Position();
        int64_t length = 0;
        int64_t count = 0;
        bool hit = false;

        // read chunk-by-chunk
        do
        {
            count = (int)Read(buffer, bufferSize);
            pos += count;

            // check for a newline
            int64_t end = count;
            for (int n = 0; n < count; n++)
                if (buffer[n] == '\n' || buffer[n] == '\r')
                {
                    hit = true;
                    end = n;

                    // skip to the end of the line for future reading
                    int64_t lineEnd = pos - count + end + 1;

                    // there might be a trailing '\n'
                    if (buffer[n] == '\r')
                    {
                        if (end < count && buffer[n + 1] == '\n')
                        {
                            lineEnd++;
                        }
                        // our buffer aligned perfectly ..... :/
                        else if (count == bufferSize && end == count)
                        {
                            char ch;
                            if (Read(&ch, 1) != 0 && ch == '\n')
                                lineEnd++;
                        }
                    }

                    Seek(lineEnd);
                    break;
                }

            // copy to string
            writeTo.resize((int)(length + end));
            memcpy(writeTo.data() + length, buffer, (size_t)end);
            *(writeTo.data() + length + end) = '\0';

            // increment length
            length += end;

        } while (!hit && count >= bufferSize);

        return length;
    }


    uint32_t Stream::ReadVLE()
    {
        uint32_t ret;
        unsigned char byte;

        byte = Read<unsigned char>();
        ret = byte & 0x7f;
        if (byte < 0x80)
            return ret;

        byte = Read<unsigned char>();
        ret |= ((unsigned)(byte & 0x7f)) << 7;
        if (byte < 0x80)
            return ret;

        byte = Read<unsigned char>();
        ret |= ((unsigned)(byte & 0x7f)) << 14;
        if (byte < 0x80)
            return ret;

        byte = Read<unsigned char>();
        ret |= ((unsigned)byte) << 21;
        return ret;
    }

    String Stream::ReadFileID()
    {
        return ReadString(4);
    }

    Vector<uint8_t> Stream::ReadBytes(uint32_t count)
    {
        Vector<uint8_t> result(count > 0 ? count : Length());
        Read(result.data(), static_cast<int64_t>(result.size()));
        return result;
    }

    Vector<uint8_t> Stream::ReadBuffer()
    {
        Vector<uint8_t> ret(ReadVLE());
        if (ret.size())
        {
            Read(ret.data(), static_cast<int64_t>(ret.size()));
        }

        return ret;
    }

    template<> bool Stream::Read<bool>()
    {
        return Read<unsigned char>() != 0;
    }

    template<> String Stream::Read<String>()
    {
        return ReadString();
    }

    template<> StringId32 Stream::Read<StringId32>()
    {
        return StringId32(Read<uint32_t>());
    }

    void Stream::WriteFileID(const String& value)
    {
        Write(value.c_str(), Min<uint64_t>(value.length(), 4));
        for (size_t i = value.length(); i < 4; ++i)
        {
            Write(' ');
        }
    }

    void Stream::WriteBuffer(const Vector<uint8_t>& value)
    {
        uint32_t numBytes = static_cast<uint32_t>(value.size());

        WriteVLE(numBytes);
        if (numBytes)
        {
            Write(&value[0], numBytes);
        }
    }

    void Stream::WriteVLE(uint32_t value)
    {
        uint8_t data[4];

        if (value < 0x80)
            Write((uint8_t)value);
        else if (value < 0x4000)
        {
            data[0] = (uint8_t)value | 0x80;
            data[1] = (uint8_t)(value >> 7);
            Write(data, 2);
        }
        else if (value < 0x200000)
        {
            data[0] = (uint8_t)value | 0x80;
            data[1] = (uint8_t)((value >> 7) | 0x80);
            data[2] = (uint8_t)(value >> 14);
            Write(data, 3);
        }
        else
        {
            data[0] = (uint8_t)value | 0x80;
            data[1] = (uint8_t)((value >> 7) | 0x80);
            data[2] = (uint8_t)((value >> 14) | 0x80);
            data[3] = (uint8_t)(value >> 21);
            Write(data, 4);
        }
    }

    void Stream::WriteLine(const String& value)
    {
        Write(value.c_str(), value.length());
        Write('\r');
        Write('\n');
    }

    template<> void Stream::Write<bool>(const bool& value)
    {
        Write<uint8_t>(value ? 1 : 0);
    }

    template<> void Stream::Write<String>(const String& value)
    {
        // Write content and null terminator
        Write(value.c_str(), value.length() + 1);
    }

    template<> void Stream::Write<StringId32>(const StringId32& value)
    {
        Write(value.Value());
    }
}
