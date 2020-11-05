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
/// MurmurHash2, by Austin Appleby
#include "Core/Hash.h"

namespace alimer
{
    uint32 Murmur32(const void* key, uint32 len, uint32 seed)
    {
        // 'm' and 'r' are mixing constants generated offline.
        // They're not really 'magic', they just happen to work well.
        const unsigned int m = 0x5bd1e995;
        const int r = 24;

        // Initialize the hash to a 'random' value
        unsigned int h = seed ^ len;

        // Mix 4 bytes at a time into the hash
        const uint8* data = (const uint8*)key;

        while (len >= 4)
        {
            unsigned int k = *(unsigned int*)data;

            k *= m;
            k ^= k >> r;
            k *= m;

            h *= m;
            h ^= k;

            data += 4;
            len -= 4;
        }

        // Handle the last few bytes of the input array
        switch (len)
        {
        case 3:
            h ^= data[2] << 16; // Fallthrough
        case 2:
            h ^= data[1] << 8; // Fallthrough
        case 1:
            h ^= data[0]; // Fallthrough
            h *= m;
        };

        // Do a few final mixes of the hash to ensure the last few
        // bytes are well-incorporated.
        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
    }

    uint64 Murmur64(const void* key, uint64 len, uint64 seed)
    {
        const uint64 m = 0xc6a4a7935bd1e995ull;
        const int r = 47;

        uint64 h = seed ^ (len * m);

        const uint64* data = (const uint64*)key;
        const uint64* end = data + (len / 8);

        while (data != end)
        {
            uint64 k = *data++;

            k *= m;
            k ^= k >> r;
            k *= m;

            h ^= k;
            h *= m;
        }

        const uint8* data2 = (const uint8*)data;

        switch (len & 7)
        {
        case 7:
            h ^= uint64(data2[6]) << 48; // Fallthrough
        case 6:
            h ^= uint64(data2[5]) << 40; // Fallthrough
        case 5:
            h ^= uint64(data2[4]) << 32; // Fallthrough
        case 4:
            h ^= uint64(data2[3]) << 24; // Fallthrough
        case 3:
            h ^= uint64(data2[2]) << 16; // Fallthrough
        case 2:
            h ^= uint64(data2[1]) << 8; // Fallthrough
        case 1:
            h ^= uint64(data2[0]); // Fallthrough
            h *= m;
        };

        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
    }
}
