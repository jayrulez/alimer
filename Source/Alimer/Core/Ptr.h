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
#include "Core/Concurrency.h"
#include <EASTL/utility.h>

namespace alimer
{
    /// Base class for intrusively reference counted objects that can be pointed to with RefPtr. These are noncopyable and non-assignable.
    class ALIMER_API RefCounted
    {
    public:
        /// Constructor
        RefCounted()
            : refs(0)
        {
        }

        /// Add a strong reference.
        int32_t AddRef()
        {
            int32_t newRefs = AtomicIncrement(&refs);
            ALIMER_ASSERT(newRefs > 0);
            return newRefs;
        }

        /// Release a strong reference.
        int32_t Release()
        {
            int32_t newRefs = AtomicDecrement(&refs);
            ALIMER_ASSERT(newRefs >= 0);

            if (newRefs == 0)
            {
                delete this;
            }

            return newRefs;
        }

        /// Return the reference count.
        int32_t Refs() const
        {
            return refs;
        }

    protected:
        virtual ~RefCounted() = default;

    private:
        int32_t refs;
    };

    /// Pointer which holds a strong reference to a RefCounted subclass and allows shared ownership.
    template <class T> class RefPtr
    {
    protected:
        T* ptr_;
        template<class U> friend class RefPtr;

        void InternalAddRef() const noexcept
        {
            if (ptr_ != nullptr) {
                ptr_->AddRef();
            }
        }

        int32_t InternalRelease() noexcept
        {
            int32_t ref = 0;
            T* temp = ptr_;

            if (temp != nullptr)
            {
                ptr_ = nullptr;
                ref = temp->Release();
            }

            return ref;
        }

    public:
        /// Construct a null pointer.
        RefPtr() noexcept : ptr_(nullptr) {}

        /// Construct a null shared pointer.
        RefPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

        template <typename U, typename = typename eastl::enable_if<eastl::is_convertible<U*, T*>::value>::type>
        RefPtr(_In_opt_ U* other) noexcept : ptr_(other)
        {
            InternalAddRef();
        }

        /// Copy-construct from another shared pointer.
        RefPtr(const RefPtr& other) : ptr_(other.ptr_)
        {
            InternalAddRef();
        }

        /// Copy-construct from another shared pointer allowing implicit upcasting.
        template <typename U, typename = typename eastl::enable_if<eastl::is_convertible<U*, T*>::value>::type>
        RefPtr(const RefPtr<U>& other) noexcept : ptr_(other.ptr_)
        {
            InternalAddRef();
        }

        /// Move-construct from another shared pointer.
        RefPtr(_Inout_ RefPtr<T>&& rhs) noexcept : ptr_(rhs.ptr_)
        {
            rhs.ptr_ = nullptr;
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

        RefPtr<T>& operator=(_In_opt_ T* other) noexcept
        {
            if (ptr_ != other)
            {
                RefPtr(other).Swap(*this);
            }

            return *this;
        }

        template <typename U>
        RefPtr& operator=(_In_opt_ U* other) noexcept
        {
            RefPtr(other).Swap(*this);
            return *this;
        }

        RefPtr& operator =(const RefPtr<T>& other) noexcept
        {
            if (ptr_ != other.ptr_)
            {
                RefPtr(other).Swap(*this);
            }

            return *this;
        }

        template<class U>
        RefPtr& operator=(const RefPtr<U>& other) noexcept
        {
            RefPtr(other).Swap(*this);
            return *this;
        }

        RefPtr& operator=(_Inout_ RefPtr&& other) noexcept
        {
            RefPtr(static_cast<RefPtr&&>(other)).Swap(*this);
            return *this;
        }

        template<class U>
        RefPtr& operator=(_Inout_ RefPtr<U>&& other) noexcept
        {
            RefPtr(static_cast<RefPtr<U>&&>(other)).Swap(*this);
            return *this;
        }

        /// Point to the object.
        T* operator ->() const
        {
            ALIMER_ASSERT(ptr_);
            return ptr_;
        }

        /// Dereference the object.
        T& operator *() const
        {
            ALIMER_ASSERT(this->Get() != nullptr);
            return *this->Get();
        }

        /// Convert to a raw pointer.
        operator T* () const { return ptr_; }

        operator bool() const noexcept { return ptr_ != nullptr; }

        /// Test for less than with another shared pointer.
        template <class U> bool operator <(const RefPtr<U>& rhs) const { return ptr_ < rhs.ptr_; }

        /// Test for equality with another shared pointer.
        template <class U> bool operator ==(const RefPtr<U>& rhs) const { return ptr_ == rhs.ptr_; }

        /// Test for inequality with another shared pointer.
        template <class U> bool operator !=(const RefPtr<U>& rhs) const { return ptr_ != rhs.ptr_; }

        /// Check if the pointer is null.
        bool IsNull() const { return ptr_ == nullptr; }

        /// Check if the pointer is not null.
        bool IsNotNull() const { return ptr_ != nullptr; }

        /// Return the raw pointer.
        T* Get() const noexcept { return ptr_; }

        void Swap(_Inout_ RefPtr&& r) noexcept
        {
            T* tmp = ptr_;
            ptr_ = r.ptr_;
            r.ptr_ = tmp;
        }

        void Swap(_Inout_ RefPtr& rhs) noexcept
        {
            T* tmp = ptr_;
            ptr_ = rhs.ptr_;
            rhs.ptr_ = tmp;
        }

        T* const* GetAddressOf() const noexcept
        {
            return &ptr_;
        }

        T** GetAddressOf() noexcept
        {
            return &ptr_;
        }

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

        int32_t Reset()
        {
            return InternalRelease();
        }

        /// Perform a static cast from a shared pointer of another type.
        template <class U> void StaticCast(const RefPtr<U>& rhs)
        {
            RefPtr<T> copy(static_cast<T*>(rhs.Get()));
            Swap(copy);
        }

        /// Perform a dynamic cast from a shared RefPtr of another type.
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

    template <typename T> inline void Swap(RefPtr<T>& a, RefPtr<T>& b)
    {
        a.Swap(b);
    }

    /// Construct RefPtr.
    template <class T, class ... Args> RefPtr<T> MakeRefPtr(Args&& ... args)
    {
        return RefPtr<T>(new T(eastl::forward<Args>(args)...));
    }
}
