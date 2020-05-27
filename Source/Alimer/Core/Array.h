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

#include "Core/Platform.h"
#include "Core/ArrayView.h"
#include "Core/Assert.h"

namespace alimer {
    /**
    * Fixed size array.
    */
    template<typename T, uint32_t N = 1>
    class Array
    {
    public:
        using IndexType = decltype(N);
        using ValueType = T;
        using Iterator = ValueType*;
        using ConstIterator = const ValueType*;

        ValueType data_[N ? N : 1];

        operator ArrayView<T>() { return ArrayView<T>(data_, N); }
        operator ArrayView<const T>() const { return ArrayView<const T>(data_, N); }

        constexpr ValueType& operator[](IndexType index)
        {
            ALIMER_ASSERT(index >= 0 && index < N);
            return data_[index];
        }

        constexpr const ValueType& operator[](IndexType index) const
        {
            ALIMER_ASSERT(index >= 0 && index < N);
            return data_[index];
        }

        void Fill(const ValueType& value)
        {
            for (IndexType index = 0; index < N; ++index) {
                data_[index] = value;
            }
        }

        constexpr T* Data() { return &data_[0]; }
        constexpr const T* Data() const { return &data_[0]; }
        IndexType Size() const { return N; }

        constexpr ValueType& Front() { return data_[0]; }
        constexpr const ValueType& Front() const { return data_[0]; }
        constexpr ValueType& Back() { return data_[N - 1]; }
        constexpr const ValueType& Back() const { return data_[N - 1]; }

        constexpr Iterator begin() noexcept { return data_; }
        constexpr ConstIterator begin() const noexcept { return data_; }
        constexpr Iterator  end() { return data_ + N; }
        constexpr ConstIterator end() const { return data_ + N; }
    };
} // namespace alimer
