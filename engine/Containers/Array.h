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
#include "Containers/ArrayView.h"
#include <cassert>
#include <cstring>
#include <algorithm>
#include <initializer_list>
#include <new>
#include <utility>

namespace alimer
{
    template<typename T>
    class Vector
    {
    public:
        using IndexType = uint32_t;
        using Iterator = T*;
        using ConstIterator = const T*;

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
            for (IndexType i = 0; i < size; ++i)
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
            DoInsertElements(0, vector.begin(), vector.end(), CopyTag{});
        }

        /// Copy-construct from another vector (iterator version).
        Vector(ConstIterator start, ConstIterator end)
        {
            DoInsertElements(0, start, end, CopyTag{});
        }

        /// Move-construct from another vector.
        Vector(Vector<T>&& vector)
        {
            swap(vector);
        }

        /// Aggregate initialization constructor.
        Vector(const std::initializer_list<T>& list) : Vector()
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

        Vector& operator=(const Vector& other)
        {
            // In case of self-assignment do nothing
            if (&rhs != this)
            {
                Vector<T> copy(rhs);
                swap(copy);
            }
            return *this;
        }

        /// Move-assign from another vector.
        Vector<T>& operator =(Vector<T>&& rhs)
        {
            assert(&rhs != this);
            swap(rhs);
            return *this;
        }

        void swap(Vector& other)
        {
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
        }

        operator ArrayView<T>() { return ArrayView<T>(data_, size_); }
        operator ArrayView<const T>() const { return ArrayView<const T>(data_, size_); }

        T& operator[](IndexType index)
        {
            assert(index >= 0 && index < size_);
            return data_[index];
        }

        const T& operator[](IndexType index) const
        {
            assert(index >= 0 && index < size_);
            return data_[index];
        }

        /// Return element at index.
        T& At(IndexType index)
        {
            assert(index >= 0 && index < size_);
            return data_[index];
        }

        /// Return const element at index.
        const T& At(IndexType index) const
        {
            assert(index >= 0 && index < size_);
            return data_[index];
        }

        /// Create an element at the end.
        template <class... Args> T& EmplaceBack(Args&&... args)
        {
            if (size_ < capacity_)
            {
                // Optimize common case
                ++size_;
                new (&Back()) T(std::forward<Args>(args)...);
            }
            else
            {
                T value(std::forward<Args>(args)...);
                Push(std::move(value));
            }
            return Back();
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
                DoInsertElements(size_, &value, &value + 1, CopyTag{});
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
                DoInsertElements(size_, &value, &value + 1, MoveTag{});
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
        void Insert(unsigned pos, const T& value)
        {
            DoInsertElements(pos, &value, &value + 1, CopyTag{});
        }

        /// Insert an element at position.
        void Insert(unsigned pos, T&& value)
        {
            DoInsertElements(pos, &value, &value + 1, MoveTag{});
        }

        /// Insert another vector at position.
        void Insert(unsigned pos, const Vector<T>& vector)
        {
            DoInsertElements(pos, vector.Begin(), vector.End(), CopyTag{});
        }

        /// Insert an element by iterator.
        Iterator Insert(const Iterator& dest, const T& value)
        {
            auto pos = (unsigned)(dest - Begin());
            return DoInsertElements(pos, &value, &value + 1, CopyTag{});
        }

        /// Move-insert an element by iterator.
        Iterator Insert(const Iterator& dest, T&& value)
        {
            auto pos = (unsigned)(dest - Begin());
            return DoInsertElements(pos, &value, &value + 1, MoveTag{});
        }

        /// Insert a vector by iterator.
        Iterator Insert(const Iterator& dest, const Vector<T>& vector)
        {
            auto pos = (unsigned)(dest - Begin());
            return DoInsertElements(pos, vector.Begin(), vector.End(), CopyTag{});
        }

        /// Insert a vector partially by iterators.
        Iterator Insert(const Iterator& dest, const ConstIterator& start, const ConstIterator& end)
        {
            auto pos = (unsigned)(dest - Begin());
            return DoInsertElements(pos, start, end, CopyTag{});
        }

        /// Insert elements.
        Iterator Insert(const Iterator& dest, const T* start, const T* end)
        {
            auto pos = (unsigned)(dest - Begin());
            return DoInsertElements(pos, start, end, CopyTag{});
        }

        /// Erase a range of elements.
        void Erase(unsigned pos, unsigned length = 1)
        {
            // Return if the range is illegal
            if (pos + length > size_ || !length)
                return;

            DoEraseElements(pos, length);
        }

        /// Erase a range of elements by swapping elements from the end of the array.
        void EraseSwap(unsigned pos, unsigned length = 1)
        {
            unsigned shiftStartIndex = pos + length;
            // Return if the range is illegal
            if (shiftStartIndex > size_ || !length)
                return;

            unsigned newSize = size_ - length;
            unsigned trailingCount = size_ - shiftStartIndex;
            if (trailingCount <= length)
            {
                // We're removing more elements from the array than exist past the end of the range being removed, so perform a normal shift and destroy
                DoEraseElements(pos, length);
            }
            else
            {
                // Swap elements from the end of the array into the empty space
                T* buffer = Buffer();
                std::move(buffer + newSize, buffer + size_, buffer + pos);
                Resize(newSize);
            }
        }

        /// Erase an element by iterator. Return iterator to the next element.
        Iterator Erase(const Iterator& it)
        {
            auto pos = (unsigned)(it - Begin());
            if (pos >= size_)
                return End();
            Erase(pos);

            return Begin() + pos;
        }

        /// Erase a range by iterators. Return iterator to the next element.
        Iterator Erase(const Iterator& start, const Iterator& end)
        {
            auto pos = (unsigned)(start - Begin());
            if (pos >= size_)
                return End();
            auto length = (unsigned)(end - start);
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
        void Clear()
        {
            Resize(0);
        }

        /// Resize the vector.
        void Resize(IndexType newSize)
        {
            InternalResize(newSize);
        }

        /// Resize the vector and fill new elements with default value.
        void Resize(IndexType newSize, const T& value)
        {
            IndexType oldSize = size();
            InternalResize(newSize);
            for (IndexType i = oldSize; i < newSize; ++i)
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
                    newBuffer = reinterpret_cast<T*>(AllocateBuffer(capacity_ * sizeof(T), alignof(T)));
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
                ++it;
            return it;
        }

        /// Return const iterator to value, or to the end if not found.
        ConstIterator Find(const T& value) const
        {
            ConstIterator it = Begin();
            while (it != End() && *it != value)
                ++it;
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
        Iterator Begin() { return data_; }

        /// Return const iterator to the beginning.
        ConstIterator Begin() const { return data_; }

        /// Return iterator to the end.
        Iterator End() { return data_ + size_; }

        /// Return const iterator to the end.
        ConstIterator End() const { return data_ + size_; }

        T& Front()
        {
            assert(size_ > 0);
            return data_[0];
        }

        const T& Front() const
        {
            assert(size_ > 0);
            return data_[0];
        }

        T& Back()
        {
            assert(size_ > 0);
            return data_[size_ - 1];
        }

        const T& Back() const
        {
            assert(size_ > 0);
            return data_[size_ - 1];
        }

        /* std compatible methods */
        Iterator begin() noexcept { return data_; }
        ConstIterator begin() const noexcept { return data_; }
        Iterator end() noexcept { return data_ + size_; }
        ConstIterator end() const noexcept { return data_ + size_; }

        T* data() noexcept { return &data_[0]; }
        const T* data() const noexcept { return &data_[0]; }
        IndexType size() const noexcept { return size_; }
        IndexType capacity() const noexcept { return capacity_; }
        bool empty() const noexcept { return size_ == 0; }

    private:
        struct CopyTag {};
        struct MoveTag {};
        static constexpr IndexType MIN_SIZE = 16;

        /// Calculate new vector capacity.
        static IndexType CalculateCapacity(IndexType size, IndexType capacity)
        {
            if (!capacity)
                return size;

            while (capacity < size)
                capacity += (capacity + 1) >> 1;
            return capacity;
        }

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
            const IndexType count = (IndexType)(end - start);
            for (IndexType i = 0; i < count; ++i)
                new(dest + i) T(*(start + i));
        }

        /// Move-construct elements.
        template <class TIterator>
        static void ConstructElements(T* dest, TIterator start, TIterator end, MoveTag)
        {
            const IndexType count = (IndexType)(end - start);
            for (IndexType i = 0; i < count; ++i)
                new(dest + i) T(std::move(*(start + i)));
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
                T* src = data_;

                // Reallocate vector
                Vector<T> newVector;
                newVector.Reserve(CalculateCapacity(size_ + numElements, capacity_));
                newVector.size_ = size_ + numElements;
                T* dest = newVector.data_;

                // Copy or move new elements
                ConstructElements(dest + pos, start, end, Tag{});

                // Move old elements
                if (pos > 0)
                    ConstructElements(dest, src, src + pos, MoveTag{});
                if (pos < size_)
                    ConstructElements(dest + pos + numElements, src + pos, src + size_, MoveTag{});

                swap(newVector);
            }
            else if (numElements > 0)
            {
                T* buffer = data_;

                // Copy or move new elements
                ConstructElements(buffer + size_, start, end, Tag{});

                // Rotate buffer
                if (pos < size_)
                {
                    std::rotate(buffer + pos, buffer + size_, buffer + size_ + numElements);
                }

                // Update size
                size_ += numElements;
            }

            return begin() + pos;
        }


        static IndexType GetGrowCapacity(IndexType currCapacity)
        {
            return currCapacity >= MIN_SIZE ? (currCapacity + (currCapacity / 2)) : MIN_SIZE;
        }

        static void Destruct(Iterator first) { first->~T(); }
        static void Destruct(Iterator first, Iterator last)
        {
            for (; first != last; ++first) {
                Destruct(first);
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

        static uint8_t* AllocateBuffer(size_t size, size_t alignment)
        {
            return new uint8_t[size];
        }

        void InternalResize(IndexType newSize)
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
                    T* src = data_;

                    // Reallocate vector
                    Vector<T> newVector;
                    newVector.Reserve(CalculateCapacity(newSize, capacity_));
                    newVector.size_ = size_;
                    T* dest = newVector.data();

                    // Move old elements
                    ConstructElements(dest, src, src + size_, MoveTag{});

                    swap(newVector);
                }

                // Initialize the new elements
                ConstructElements(data_ + size_, newSize - size_);
            }

            size_ = newSize;
        }

        T* data_ = nullptr;
        IndexType size_ = 0;
        IndexType capacity_ = 0;
    };
}
