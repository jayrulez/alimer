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

#include "engine/Assert.h"

namespace Alimer
{
    template<typename T>
    class ArrayView
    {
    public:
        ArrayView() = default;

        ArrayView(const ArrayView& other)
            : begin_(other.begin_)
            , end_(other.end_)
        {
        }

        template<typename U>
        ArrayView(const ArrayView<U>& other)
            : begin_(other.begin())
            , end_(other.end())
        {
        }

        ArrayView(T& value)
            : begin_(&value)
            , end_(begin_ + 1)
        {
            ALIMER_ASSERT(begin_ == nullptr && end_ == nullptr || (begin_ != nullptr && end_ != nullptr));
        }

        ArrayView(T* begin, T* end)
            : begin_(begin)
            , end_(end)
        {
            ALIMER_ASSERT(begin_ == nullptr && end_ == nullptr || (begin_ != nullptr && end_ != nullptr));
        }

        ArrayView(T* data, uint32_t size)
            : begin_(data)
            , end_(data + size)
        {
        }

        template<uint32_t SIZE>
        ArrayView(T(&arr)[SIZE])
            : begin_(&arr[0])
            , end_(&arr[0] + SIZE)

        {
        }

        ~ArrayView() = default;

        T* begin() { return begin_; }
        const T* begin() const { return begin_; }
        T* end() { return end_; }
        const T* end() const { return end_; }

        uint32_t size() const { return (uint32_t)(end_ - begin_); }
        T* data() const { return begin_; }

        explicit operator bool() const { return !!begin_; }

        T& operator[](uint32_t index)
        {
            ALIMER_ASSERT(index >= 0 && index < size());
            return begin_[index];
        }

        const T& operator[](uint32_t index) const
        {
            ALIMER_ASSERT(index >= 0 && index < size());
            return begin_[index];
        }

    private:
        T* begin_ = nullptr;
        T* end_ = nullptr;
    };
}
