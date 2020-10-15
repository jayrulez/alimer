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

#include "PlatformDef.h"
#include <functional>

namespace Alimer
{
    // Source: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
    template <class T>
    constexpr void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    constexpr size_t string_hash(const char* input)
    {
        // https://stackoverflow.com/questions/2111667/compile-time-string-hashing
        size_t hash = sizeof(size_t) == 8 ? 0xcbf29ce484222325 : 0x811c9dc5;
        const size_t prime = sizeof(size_t) == 8 ? 0x00000100000001b3 : 0x01000193;

        while (*input)
        {
            hash ^= static_cast<size_t>(*input);
            hash *= prime;
            ++input;
        }

        return hash;
    }

    ALIMER_API uint32 Murmur32(const void* key, uint32 len, uint32 seed);
    ALIMER_API uint64 Murmur64(const void* key, uint64 len, uint64 seed);
}
