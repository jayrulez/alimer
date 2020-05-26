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
#include "Core/Allocator.h"
#include "Core/ArrayView.h"
#include "Core/Assert.h"
#include <cstdlib>
#include <initializer_list>
#include <algorithm>

namespace Alimer
{
    template<typename T>
    class Vector
    {
        struct CopyTag {};
        struct MoveTag {};

    public:
        using IndexType = uint32_t;
        using ValueType = T;
        using Iterator = ValueType*;
        using ConstIterator = const ValueType*;

        /// Construct empty.
        Vector() noexcept = default;

        /// Construct with initial size.
        explicit Vector(IndexType size)
        {
            Resize(size);
        }

        /// Construct with initial size and default value.
        Vector(IndexType size, const T& value)
        {
            Resize(size);
            for (unsigned i = 0; i < size; ++i)
                At(i) = value;
        }

        /// Construct with initial data.
        Vector(const T* data, IndexType size)
        {
            DoInsertElements(0, data, data + size, CopyTag{});
        }

        /// Copy-construct from another vector.
        Vector(const Vector<T>& vector)
        {
            DoInsertElements(0, vector.Begin(), vector.End(), CopyTag{});
        }

        /// Copy-construct from another vector (iterator version).
        Vector(ConstIterator start, ConstIterator end)
        {
            DoInsertElements(0, start, end, CopyTag{});
        }

        /// Move-construct from another vector.
        Vector(Vector<T>&& vector)
        {
            Swap(vector);
        }

        /// Aggregate initialization constructor.
        Vector(const std::initializer_list<T>& list)
        {
            for (auto it = list.begin(); it != list.end(); it++)
            {
                Push(*it);
            }
        }

        ~Vector()
        {
            DestructElements(data_, size_);
            delete[] data_;
        }

        /// Assign from another vector.
        Vector<T>& operator =(const Vector<T>& rhs)
        {
            // In case of self-assignment do nothing
            if (&rhs != this)
            {
                Vector<T> copy(rhs);
                Swap(copy);
            }
            return *this;
        }

        /// Move-assign from another vector.
        Vector<T>& operator =(Vector<T>&& rhs)
        {
            ALIMER_ASSERT(&rhs != this);
            Swap(rhs);
            return *this;
        }

        /// Add-assign an element.
        Vector<T>& operator +=(const T& rhs)
        {
            Push(rhs);
            return *this;
        }

        /// Add-assign another vector.
        Vector<T>& operator +=(const Vector<T>& rhs)
        {
            Push(rhs);
            return *this;
        }

        /// Add an element.
        Vector<T> operator +(const T& rhs) const
        {
            Vector<T> ret(*this);
            ret.Push(rhs);
            return ret;
        }

        /// Add another vector.
        Vector<T> operator +(const Vector<T>& rhs) const
        {
            Vector<T> ret(*this);
            ret.Push(rhs);
            return ret;
        }

        /// Test for equality with another vector.
        bool operator ==(const Vector<T>& rhs) const
        {
            if (rhs.size_ != size_)
                return false;

            for (IndexType i = 0; i < size_; ++i)
            {
                if (data_[i] != rhs.data_[i])
                    return false;
            }

            return true;
        }

        /// Test for inequality with another vector.
        bool operator !=(const Vector<T>& rhs) const
        {
            if (rhs.size_ != size_)
                return true;

            for (IndexType i = 0; i < size_; ++i)
            {
                if (data_[i] != rhs.data_[i])
                    return true;
            }

            return false;
        }

        operator ArrayView<T>() { return ArrayView<T>(data_, size_); }
        operator ArrayView<const T>() const { return ArrayView<const T>(data_, size_); }

        T& operator[](IndexType index)
        {
            ALIMER_ASSERT(index < size_);
            return data_[index];
        }

        const T& operator[](IndexType index) const
        {
            ALIMER_ASSERT(index < size_);
            return data_[index];
        }

        /// Return element at index.
        T& At(IndexType index)
        {
            ALIMER_ASSERT(index < size_);
            return Buffer()[index];
        }

        /// Return const element at index.
        const T& At(IndexType index) const
        {
            assert(index < size_);
            return Buffer()[index];
        }


        void Swap(Vector& other)
        {
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
        }

        /// Add an element at the end.
        void Push(const T& value)
        {
            if (size_ < capacity_)
            {
                // Optimize common case
                ++size_;
                new (&Back()) T(value);
            }
            else
            {
                DoInsertElements(size_, &value, &value + 1, CopyTag{});
            }
        }

        /// Move-add an element at the end.
        void Push(T&& value)
        {
            if (size_ < capacity_)
            {
                // Optimize common case
                ++size_;
                new (&Back()) T(std::move(value));
            }
            else
            {
                DoInsertElements(size_, &value, &value + 1, MoveTag{});
            }
        }

        /// Add another vector at the end.
        void Push(const Vector<T>& vector) { DoInsertElements(size_, vector.Begin(), vector.End(), CopyTag{}); }

        /// Remove the last element.
        void Pop()
        {
            if (size_)
                Resize(size_ - 1);
        }

        /// Insert an element at position.
        void Insert(IndexType pos, const T& value)
        {
            DoInsertElements(pos, &value, &value + 1, CopyTag{});
        }

        /// Insert an element at position.
        void Insert(IndexType pos, T&& value)
        {
            DoInsertElements(pos, &value, &value + 1, MoveTag{});
        }

        /// Insert another vector at position.
        void Insert(IndexType pos, const Vector<T>& vector)
        {
            DoInsertElements(pos, vector.Begin(), vector.End(), CopyTag{});
        }

        /// Insert an element by iterator.
        Iterator Insert(const Iterator& dest, const T& value)
        {
            auto pos = (IndexType)(dest - Begin());
            return DoInsertElements(pos, &value, &value + 1, CopyTag{});
        }

        /// Move-insert an element by iterator.
        Iterator Insert(const Iterator& dest, T&& value)
        {
            auto pos = (IndexType)(dest - Begin());
            return DoInsertElements(pos, &value, &value + 1, MoveTag{});
        }

        /// Insert a vector by iterator.
        Iterator Insert(const Iterator& dest, const Vector<T>& vector)
        {
            auto pos = (IndexType)(dest - Begin());
            return DoInsertElements(pos, vector.Begin(), vector.End(), CopyTag{});
        }

        /// Insert a vector partially by iterators.
        Iterator Insert(const Iterator& dest, const ConstIterator& start, const ConstIterator& end)
        {
            auto pos = (IndexType)(dest - Begin());
            return DoInsertElements(pos, start, end, CopyTag{});
        }

        /// Insert elements.
        Iterator Insert(const Iterator& dest, const T* start, const T* end)
        {
            auto pos = (IndexType)(dest - Begin());
            return DoInsertElements(pos, start, end, CopyTag{});
        }

        /// Erase a range of elements.
        void Erase(IndexType pos, IndexType length = 1)
        {
            // Return if the range is illegal
            if (pos + length > size_ || !length)
                return;

            DoEraseElements(pos, length);
        }

        /// Erase a range of elements by swapping elements from the end of the array.
        void EraseSwap(IndexType pos, IndexType length = 1)
        {
            IndexType shiftStartIndex = pos + length;
            // Return if the range is illegal
            if (shiftStartIndex > size_ || !length)
                return;

            IndexType newSize = size_ - length;
            IndexType trailingCount = size_ - shiftStartIndex;
            if (trailingCount <= length)
            {
                // We're removing more elements from the array than exist past the end of the range being removed, so perform a normal shift and destroy
                DoEraseElements(pos, length);
            }
            else
            {
                // Swap elements from the end of the array into the empty space
                std::move(data_ + newSize, data_ + size_, data_ + pos);
                Resize(newSize);
            }
        }

        /// Erase an element by iterator. Return iterator to the next element.
        Iterator Erase(const Iterator& it)
        {
            auto pos = (IndexType)(it - Begin());
            if (pos >= size_)
                return End();
            Erase(pos);

            return Begin() + pos;
        }

        /// Erase a range by iterators. Return iterator to the next element.
        Iterator Erase(const Iterator& start, const Iterator& end)
        {
            auto pos = (IndexType)(start - Begin());
            if (pos >= size_)
                return End();
            auto length = (IndexType)(end - start);
            Erase(pos, length);

            return Begin() + pos;
        }

        /// Erase an element by value. Return true if was found and erased.
        bool Remove(const T& value)
        {
            Iterator i = Find(value);
            if (i != End())
            {
                Erase(i);
                return true;
            }
            else
                return false;
        }

        /// Erase an element by value by swapping with the last element. Return true if was found and erased.
        bool RemoveSwap(const T& value)
        {
            Iterator i = Find(value);
            if (i != End())
            {
                EraseSwap(i - Begin());
                return true;
            }
            else
                return false;
        }

        /// Clear the vector.
        void Clear() { Resize(0); }

        void Resize(IndexType size)
        {
            DoResize(size);
        }

        /// Resize the vector and fill new elements with default value.
        void Resize(IndexType newSize, const T& value)
        {
            IndexType oldSize = size();
            DoResize(newSize);
            for (unsigned i = oldSize; i < newSize; ++i)
                At(i) = value;
        }

        /// Set new capacity.
        void Reserve(IndexType newCapacity)
        {
            if (newCapacity < size_)
                newCapacity = size_;

            if (newCapacity != capacity_)
            {
                T* newBuffer = nullptr;
                capacity_ = newCapacity;

                if (capacity_)
                {
                    newBuffer = new T[capacity_];
                    // Move the data into the new buffer
                    ConstructElements(newBuffer, begin(), end(), MoveTag{});
                }

                // Delete the old buffer
                DestructElements(data_, size_);
                delete[] data_;
                data_ = newBuffer;
            }
        }

        /// Reallocate so that no extra memory is used.
        void Compact() { Reserve(size_); }

        /// Return iterator to value, or to the end if not found.
        Iterator Find(const T& value)
        {
            Iterator it = Begin();
            while (it != End() && *it != value)
            {
                ++it;
            }

            return it;
        }

        /// Return const iterator to value, or to the end if not found.
        ConstIterator Find(const T& value) const
        {
            ConstIterator it = Begin();
            while (it != End() && *it != value)
            {
                ++it;
            }

            return it;
        }

        /// Return index of value in vector, or size if not found.
        IndexType IndexOf(const T& value) const
        {
            return Find(value) - Begin();
        }

        /// Return whether contains a specific value.
        bool Contains(const T& value) const { return Find(value) != End(); }

        /// Return iterator to the beginning.
        Iterator Begin() noexcept { return data_; }

        /// Return const iterator to the beginning.
        ConstIterator Begin() const noexcept { return data_; }

        /// Return iterator to the end.
        Iterator End() noexcept { return data_ + size_; }

        /// Return const iterator to the end.
        ConstIterator End() const noexcept { return data_ + size_; }

        ValueType& Front()
        {
            ALIMER_ASSERT(size_ > 0);
            return data_[0];
        }

        const ValueType& Front() const
        {
            ALIMER_ASSERT(size_ > 0);
            return data_[0];
        }

        ValueType& Back()
        {
            ALIMER_ASSERT(size_ > 0);
            return data_[size_ - 1];
        }

        const ValueType& Back() const
        {
            ALIMER_ASSERT(size_ > 0);
            return data_[size_ - 1];
        }

        T* Data() noexcept { return &data_[0]; }
        const T* Data() const noexcept { return &data_[0]; }
        IndexType Size() const noexcept { return size_; }
        IndexType Capacity() const noexcept { return capacity_; }
        bool Empty() const noexcept { return size_ == 0; }

        Iterator begin() noexcept { return data_; }
        ConstIterator begin() const noexcept { return data_; }
        Iterator end() noexcept { return data_ + size_; }
        ConstIterator end() const noexcept { return data_ + size_; }

    private:
        /// Construct elements using default ctor.
        static void ConstructElements(T* dest, IndexType count)
        {
            for (IndexType i = 0; i < count; ++i)
                new(dest + i) T();
        }

        /// Copy-construct elements.
        template <class TIterator>
        static void ConstructElements(T* dest, TIterator start, TIterator end, CopyTag)
        {
            const ptrdiff_t count = end - start;
            for (ptrdiff_t i = 0; i < count; ++i)
                new(dest + i) T(*(start + i));
        }

        /// Move-construct elements.
        template <class TIterator>
        static void ConstructElements(T* dest, TIterator start, TIterator end, MoveTag)
        {
            const ptrdiff_t count = end - start;
            for (ptrdiff_t i = 0; i < count; ++i)
            {
                new(dest + i) T(std::move(*(start + i)));
            }
        }

        /// Insert elements into the vector using copy or move constructor.
        template <class Tag, class TIterator>
        Iterator DoInsertElements(IndexType pos, TIterator start, TIterator end, Tag)
        {
            if (pos > size_)
                pos = size_;

            const IndexType numElements = (IndexType)(end - start);
            if (size_ + numElements > capacity_)
            {
                // Reallocate vector
                Vector<T> newVector;
                newVector.Reserve(CalculateCapacity(size_ + numElements, capacity_));
                newVector.size_ = size_ + numElements;
                T* dest = newVector.data_;

                // Copy or move new elements
                ConstructElements(dest + pos, start, end, Tag{});

                // Move old elements
                if (pos > 0)
                    ConstructElements(dest, data_, data_ + pos, MoveTag{});
                if (pos < size_)
                    ConstructElements(dest + pos + numElements, data_ + pos, data_ + size_, MoveTag{});

                Swap(newVector);
            }
            else if (numElements > 0)
            {
                // Copy or move new elements
                ConstructElements(data_ + size_, start, end, Tag{});

                // Rotate buffer
                if (pos < size_)
                {
                    std::rotate(data_ + pos, data_ + size_, data_ + size_ + numElements);
                }

                // Update size
                size_ += numElements;
            }

            return begin() + pos;
        }

        /// Erase elements from the vector.
        Iterator DoEraseElements(IndexType pos, IndexType count)
        {
            ALIMER_ASSERT(count > 0);
            ALIMER_ASSERT(pos + count <= size_);
            std::move(data_ + pos + count, data_ + size_, data_ + pos);
            Resize(size_ - count);
            return Begin() + pos;
        }

        /// Calculate new vector capacity.
        static IndexType CalculateCapacity(IndexType size, IndexType capacity)
        {
            if (!capacity)
            {
                return size;
            }
            else
            {
                while (capacity < size)
                    capacity += (capacity + 1) >> 1;
                return capacity;
            }
        }

        /// Call the elements' destructors.
        static void DestructElements(T* dest, IndexType count)
        {
            while (count--)
            {
                dest->~T();
                ++dest;
            }
        }

        void DoResize(uint32_t newSize)
        {
            // If size shrinks, destruct the removed elements
            if (newSize < size_)
            {
                DestructElements(data_ + newSize, size_ - newSize);
            }
            else
            {
                // Allocate new buffer if necessary and copy the current elements
                if (newSize > capacity_)
                {
                    // Reallocate vector
                    Vector<T> newVector;
                    newVector.Reserve(CalculateCapacity(newSize, capacity_));
                    newVector.size_ = size_;

                    // Move old elements
                    ConstructElements(newVector.data_, data_, data_ + size_, MoveTag{});

                    Swap(newVector);
                }

                // Initialize the new elements
                ConstructElements(data_ + size_, newSize - size_);
            }

            size_ = newSize;
        }

        T* data_ = nullptr;
        uint32_t size_ = 0;
        uint32_t capacity_ = 0;
    };
}
