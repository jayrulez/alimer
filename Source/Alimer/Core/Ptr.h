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

#include "Core/Assert.h"
#include <atomic>

namespace alimer
{
    /// Base class for intrusively reference counted objects that can be pointed to with RefPtr. These are noncopyable and non-assignable.
    class ALIMER_API RefCounted
    {
    public:
        /// Constructor
        RefCounted()
        {
            refs.store(0, std::memory_order_relaxed);
        }

        /// Destructor
        virtual ~RefCounted() = default;

        /// Prevent copy construction.
        RefCounted(const RefCounted& rhs) = delete;
        /// Prevent assignment.
        RefCounted& operator =(const RefCounted& rhs) = delete;

        /// Add a strong reference.
        void AddRef()
        {
            refs.fetch_add(1, std::memory_order_relaxed);
        }

        /// Release a strong reference.
        void Release()
        {
            auto result = refs.fetch_sub(1, std::memory_order_acq_rel);

            if (result == 1)
            {
                delete this;
            }
        }

        /// Return the reference count.
        int32_t Refs() const
        {
            return refs.load(std::memory_order_relaxed);
        }

    private:
        std::atomic_uint32_t refs;
    };

    // Type alias for intrusive_ptr template
    template <typename T> class RefPtr
    {
    protected:
        T* ptr_;
        template<class U> friend class RefPtr;

        void InternalAddRef() const noexcept
        {
            if (ptr_ != nullptr)
            {
                ptr_->AddRef();
            }
        }

        void InternalRelease() noexcept
        {
            if (ptr_ != nullptr)
            {
                ptr_->Release();
                ptr_ = nullptr;
            }
        }

    public:
        /// Construct a null shared pointer.
        RefPtr() noexcept
            : ptr_(nullptr)
        {
        }

        /// Construct a null shared pointer.
        RefPtr(std::nullptr_t) noexcept
            : ptr_(nullptr)
        {
        }

        /// Copy-construct from another shared pointer.
        RefPtr(const RefPtr& rhs) noexcept
            : ptr_(rhs.ptr_)
        {
            InternalAddRef();
        }

        /// Move-construct from another shared pointer.
        RefPtr(RefPtr&& rhs) noexcept
            : ptr_(rhs.ptr_)
        {
            rhs.ptr_ = nullptr;
        }

        /// Copy-construct from another shared pointer allowing implicit upcasting.
        template <class U> RefPtr(const RefPtr<U>& rhs) noexcept
            : ptr_(rhs.ptr_)
        {
            InternalAddRef();
        }

        /// Construct from a raw pointer.
        explicit RefPtr(T* ptr) noexcept
            : ptr_(ptr)
        {
            InternalAddRef();
        }

        /// Destruct. Release the object reference.
        ~RefPtr() noexcept
        {
            InternalRelease();
        }

        RefPtr<T>& operator=(std::nullptr_t) noexcept
        {
            InternalRelease();
            return *this;
        }

        /// Assign from another shared pointer.
        RefPtr<T>& operator =(const RefPtr<T>& rhs)
        {
            if (ptr_ == rhs.ptr_)
                return *this;

            RefPtr<T> copy(rhs);
            Swap(copy);

            return *this;
        }

        /// Move-assign from another shared pointer.
        RefPtr<T>& operator =(RefPtr<T>&& rhs)
        {
            RefPtr<T> copy(std::move(rhs));
            Swap(copy);

            return *this;
        }

        /// Assign from another shared pointer allowing implicit upcasting.
        template <class U> RefPtr<T>& operator =(const RefPtr<U>& rhs)
        {
            if (ptr_ == rhs.ptr_)
                return *this;

            RefPtr<T> copy(rhs);
            Swap(copy);

            return *this;
        }

        /// Assign from a raw pointer.
        RefPtr<T>& operator =(T* ptr)
        {
            if (ptr_ == ptr)
                return *this;

            RefPtr<T> copy(ptr);
            Swap(copy);

            return *this;
        }

        /// Swap with another SharedPtr.
        void Swap(_Inout_ RefPtr& rhs)
        {
            T* tmp = ptr_;
            ptr_ = rhs.ptr_;
            rhs.ptr_ = tmp;
        }

        explicit operator bool() const { return ptr_ != nullptr; }

        /// Return the raw pointer.
        T* Get() const { return ptr_; }

        /// Point to the object.
        T* operator ->() const { return ptr_; }

        /// Dereference the object.
        T& operator *() const { return *ptr_; }

        /// Convert to a raw pointer.
        operator T* () const { return ptr_; }

        /// Test for equality with another shared pointer.
        bool operator ==(const RefPtr& rhs) const { return ptr_ == rhs.ptr_; }

        /// Test for inequality with another shared pointer.
        bool operator !=(const RefPtr& rhs) const { return ptr_ != rhs.ptr_; }

        T* const* GetAddressOf() const noexcept { return &ptr_; }
        T** GetAddressOf() noexcept { return &ptr_; }

        T** ReleaseAndGetAddressOf() noexcept
        {
            InternalRelease();
            return &ptr_;
        }

        T* Detach() noexcept
        {
            T* ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }

        /// Reset with another pointer.
        void Reset(T* ptr = nullptr)
        {
            RefPtr<T> copy(ptr);
            Swap(copy);
        }

        /// Perform a static cast from a shared pointer of another type.
        template <class U> void StaticCast(const RefPtr<U>& rhs)
        {
            RefPtr<T> copy(static_cast<T*>(rhs.Get()));
            Swap(copy);
        }

        /// Perform a dynamic cast from a shared pointer of another type.
        template <class U> void DynamicCast(const RefPtr<U>& rhs)
        {
            RefPtr<T> copy(dynamic_cast<T*>(rhs.Get()));
            Swap(copy);
        }
    };

    /// Perform a static cast from one shared pointer type to another.
    template <class T, class U> RefPtr<T> StaticCast(const RefPtr<U>& ptr)
    {
        RefPtr<T> ret;
        ret.StaticCast(ptr);
        return ret;
    }

    /// Perform a dynamic cast from one weak pointer type to another.
    template <class T, class U> RefPtr<T> DynamicCast(const RefPtr<U>& ptr)
    {
        RefPtr<T> ret;
        ret.DynamicCast(ptr);
        return ret;
    }
}
