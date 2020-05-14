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

#include "core/Assert.h"

namespace alimer
{
    template<typename T>
    class ArrayView
    {
    public:
        using ElementType = T;

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

        ArrayView(ElementType& value)
            : begin_(&value)
            , end_(begin_ + 1)
        {
            ALIMER_ASSERT(begin_ == nullptr && end_ == nullptr || (begin_ != nullptr && end_ != nullptr));
        }

        ArrayView(ElementType* begin, ElementType* end)
            : begin_(begin)
            , end_(end)
        {
            ALIMER_ASSERT(begin_ == nullptr && end_ == nullptr || (begin_ != nullptr && end_ != nullptr));
        }

        ArrayView(ElementType* data, uint32_t size)
            : begin_(data)
            , end_(data + size)
        {
        }

        template<uint32_t SIZE>
        ArrayView(ElementType(&arr)[SIZE])
            : begin_(&arr[0])
            , end_(&arr[0] + SIZE)

        {
        }

        ~ArrayView() = default;

        ALIMER_FORCEINLINE ElementType* begin() { return begin_; }
        ALIMER_FORCEINLINE ElementType* end() { return end_; }
        ALIMER_FORCEINLINE const ElementType* begin() const { return begin_; }
        ALIMER_FORCEINLINE const ElementType* end() const { return end_; }

        uint32_t size() const { return (uint32_t)(end_ - begin_); }
        ElementType* data() const { return begin_; }

        explicit operator bool() const { return !!begin_; }

        ElementType& operator[](uint32_t index)
        {
            ALIMER_ASSERT(index >= 0 && index < size());
            return begin_[index];
        }

        const ElementType& operator[](uint32_t index) const
        {
            ALIMER_ASSERT(index >= 0 && index < size());
            return begin_[idx];
        }

    private:
        ArrayView* begin_ = nullptr;
        ArrayView* end_ = nullptr;
    };
}
