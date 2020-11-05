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
#include <type_traits>
#include <utility>

namespace alimer
{
    /**
     * Hash for enum types, to be used instead of std::hash<T> when T is an enum.
     *
     * Until C++14, std::hash<T> is not defined if T is a enum (see
     * http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#2148).  But
     * even with C++14, as of october 2016, std::hash for enums is not widely
     * implemented by compilers, so here when T is a enum, we use EnumClassHash
     * instead of std::hash. (For instance, in bs::hash_combine(), or
     * bs::UnorderedMap.)
     */
    struct EnumClassHash
    {
        template <typename T> constexpr std::size_t operator()(T t) const { return static_cast<std::size_t>(t); }
    };

    /** Hasher that handles custom enums automatically. */
    template <typename Key> using HashType = typename std::conditional<std::is_enum<Key>::value, EnumClassHash, std::hash<Key>>::type;

    // Source: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
    template <class T> constexpr void CombineHash(std::size_t& seed, const T& v)
    {
        using HashType = typename std::conditional<std::is_enum<T>::value, EnumClassHash, std::hash<T>>::type;

        HashType hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    /** Generates a hash for the provided type. Type must have a std::hash specialization. */
    template <class T> constexpr size_t Hash(const T& v)
    {
        using HashType = typename std::conditional<std::is_enum<T>::value, EnumClassHash, std::hash<T>>::type;

        HashType hasher;
        return hasher(v);
    }

    constexpr size_t StringHash(const char* input)
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
