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

#include "Core/Memory.h"
#include "Core/Hash.h"

#include <vector>
#include <unordered_map>

namespace Alimer
{
    template <typename T>
    constexpr typename std::underlying_type<T>::type ecast(T x)
    {
        return static_cast<typename std::underlying_type<T>::type>(x);
    }

    /**
     * Smart pointer that retains shared ownership of an project through a pointer. Reference to the object must be unique.
     * The object is destroyed automatically when the pointer to the object is destroyed.
     */
    template <typename T, typename Alloc = GenAlloc, typename Delete = Deleter<T, Alloc>>
    using UniquePtr = std::unique_ptr<T, Delete>;

    /** Dynamically sized array that stores element contigously. */
    template <typename T, typename A = StdAlloc<T>>
    using Vector = std::vector<T, A>;

    /** An associative container containing an ordered set of key-value pairs. Usually faster than Map for larger data sets. */
    template <typename K, typename V, typename H = HashType<K>, typename C = std::equal_to<K>, typename A = StdAlloc<std::pair<const K, V>>>
    using UnorderedMap = std::unordered_map<K, V, H, C, A>;

    template<typename Type, typename Alloc = GenAlloc, typename Delete = Deleter<Type, Alloc>>
    UniquePtr<Type, Alloc, Delete> MakeUniquePtr(Type* data, Delete deleter = Delete())
    {
        return UniquePtr<Type, Alloc, Delete>(data, std::move(deleter));
    }

    /** Create a new unique pointer using a custom allocator category. */
    template<typename Type, typename Alloc = GenAlloc, typename Delete = Deleter<Type, Alloc>, typename... Args>
    UniquePtr<Type, Alloc, Delete> MakeUnique(Args &&... args)
    {
        Type* ptr = alimer_new<Type, Alloc>(std::forward<Args>(args)...);

        return MakeUniquePtr<Type, Alloc, Delete>(ptr);
    }
}
