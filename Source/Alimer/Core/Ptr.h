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
#include <utility>

namespace alimer
{
    /// Reference count structure.
    struct RefCount
    {
        /// Construct.
        RefCount() = default;

        /// Destruct.
        ~RefCount()
        {
            // Set reference counts below zero to fire asserts if this object is still accessed
            refs = -1;
            weakRefs = -1;
        }

        /// Reference count. If below zero, the object has been destroyed.
        int32_t refs = 0;
        /// Weak reference count.
        int32_t weakRefs = 0;
    };

    /// Base class for intrusively reference counted objects that can be pointed to with RefPtr. These are noncopyable and non-assignable.
    class ALIMER_API RefCounted
    {
    public:
        /// Construct. Allocate the reference count structure and set an initial self weak reference.
        RefCounted();
        /// Destruct. Mark as expired and also delete the reference count structure if no outside weak references exist.
        virtual ~RefCounted();

        /// Prevent copy construction.
        RefCounted(const RefCounted& rhs) = delete;
        /// Prevent assignment.
        RefCounted& operator =(const RefCounted& rhs) = delete;

        /// Increment reference count. Can also be called outside of a RefPtr for traditional reference counting. Returns new reference count value. Operation is atomic.
        int32_t AddRef();

        /// Decrement reference count and delete self if no more references. Can also be called outside of a RefPtr for traditional reference counting. Returns new reference count value. Operation is atomic.
        int32_t Release();

        /// Return reference count.
        int32_t Refs() const;
        /// Return weak reference count.
        int32_t WeakRefs() const;

        /// Return pointer to the reference count structure.
        RefCount* RefCountPtr() { return refCount; }

    private:
        /// Pointer to the reference count structure.
        RefCount* refCount;
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
            if (ptr_)
            {
                ptr_->Release();
                ptr_ = nullptr;
            }
        }

    public:
        /// Construct a null shared pointer.
        RefPtr() noexcept : ptr_(nullptr)
        {
        }

        /// Construct a null shared pointer.
        RefPtr(std::nullptr_t) noexcept : ptr_(nullptr)
        {
        }

        /// Copy-construct from another shared pointer.
        RefPtr(const RefPtr<T>& rhs) noexcept : ptr_(rhs.ptr_)
        {
            InternalAddRef();
        }

        /// Move-construct from another shared pointer.
        RefPtr(RefPtr<T>&& rhs) noexcept : ptr_(rhs.ptr_)
        {
            rhs.ptr_ = nullptr;
        }

        /// Copy-construct from another shared pointer allowing implicit upcasting.
        template <class U> RefPtr(const RefPtr<U>& rhs) noexcept : ptr_(rhs.ptr_)
        {
            InternalAddRef();
        }

        /// Construct from a raw pointer.
        explicit RefPtr(T* ptr) noexcept : ptr_(ptr)
        {
            InternalAddRef();
        }

        /// Destruct. Release the object reference.
        ~RefPtr() noexcept
        {
            InternalRelease();
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
            std::swap(ptr_, rhs.ptr_);
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

        /// Subscript the object if applicable.
        T& operator [](size_t index)
        {
            ALIMER_ASSERT(ptr_);
            return ptr_[index];
        }

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

    /// Weak pointer template class with intrusive reference counting. Does not keep the object pointed to alive.
    template <class T> class WeakPtr
    {
    protected:
        /// Pointer to the object.
        T* ptr_;
        /// Pointer to the RefCount structure.
        RefCount* refCount_;

        template <class U> friend class WeakPtr;

        /// Add a weak reference to the object pointed to.
        void InternalAddRef() const noexcept
        {
            if (refCount_)
            {
                ALIMER_ASSERT(refCount_->weakRefs >= 0);
                AtomicIncrement(&refCount_->weakRefs);
            }
        }

        /// Release the weak reference. Delete the Refcount structure if necessary.
        void InternalRelease()
        {
            if (refCount_)
            {
                ALIMER_ASSERT(refCount_->weakRefs > 0);
                int32_t weakRefs = AtomicDecrement(&refCount_->weakRefs);

                if (IsExpired() && weakRefs == 0)
                    delete refCount_;
            }

            ptr_ = nullptr;
            refCount_ = nullptr;
        }

    public:
        /// Construct a null weak pointer.
        WeakPtr() noexcept : ptr_(nullptr), refCount_(nullptr)
        {
        }

        /// Construct a null weak pointer.
        WeakPtr(std::nullptr_t) noexcept : ptr_(nullptr), refCount_(nullptr)
        {
        }

        /// Copy-construct from another weak pointer.
        WeakPtr(const WeakPtr<T>& rhs) noexcept : ptr_(rhs.ptr_), refCount_(rhs.refCount_)
        {
            InternalAddRef();
        }

        /// Move-construct from another weak pointer.
        WeakPtr(WeakPtr<T>&& rhs) noexcept : ptr_(rhs.ptr_), refCount_(rhs.refCount_)
        {
            rhs.ptr_ = nullptr;
            rhs.refCount_ = nullptr;
        }

        /// Copy-construct from another weak pointer allowing implicit upcasting.
        template <class U> WeakPtr(const WeakPtr<U>& rhs) noexcept : ptr_(rhs.ptr_), refCount_(rhs.refCount_)
        {
            InternalAddRef();
        }

        /// Construct from a shared pointer.
        WeakPtr(const RefPtr<T>& rhs) noexcept : ptr_(rhs.Get()), refCount_(rhs.RefCountPtr())
        {
            InternalAddRef();
        }

        /// Construct from a raw pointer.
        explicit WeakPtr(T* ptr) noexcept : ptr_(ptr), refCount_(ptr ? ptr->RefCountPtr() : nullptr)
        {
            InternalAddRef();
        }

        /// Destruct. Release the weak reference to the object.
        ~WeakPtr() noexcept
        {
            InternalRelease();
        }

        /// Assign from a shared pointer.
        WeakPtr<T>& operator =(const RefPtr<T>& rhs)
        {
            if (ptr_ == rhs.Get() && refCount_ == rhs.RefCountPtr())
                return *this;

            WeakPtr<T> copy(rhs);
            Swap(copy);

            return *this;
        }

        /// Assign from a weak pointer.
        WeakPtr<T>& operator =(const WeakPtr<T>& rhs)
        {
            if (ptr_ == rhs.ptr_ && refCount_ == rhs.refCount_)
                return *this;

            WeakPtr<T> copy(rhs);
            Swap(copy);

            return *this;
        }

        /// Move-assign from another weak pointer.
        WeakPtr<T>& operator =(WeakPtr<T>&& rhs)
        {
            WeakPtr<T> copy(std::move(rhs));
            Swap(copy);

            return *this;
        }

        /// Assign from another weak pointer allowing implicit upcasting.
        template <class U> WeakPtr<T>& operator =(const WeakPtr<U>& rhs)
        {
            if (ptr_ == rhs.ptr_ && refCount_ == rhs.refCount_)
                return *this;

            InternalRelease();
            ptr_ = rhs.ptr_;
            refCount_ = rhs.refCount_;
            InternalAddRef();

            return *this;
        }

        /// Assign from a raw pointer.
        WeakPtr<T>& operator =(T* ptr)
        {
            RefCount* refCount = ptr ? ptr->RefCountPtr() : nullptr;

            if (ptr_ == ptr && refCount_ == refCount)
                return *this;

            InternalRelease();
            ptr_ = ptr;
            refCount_ = refCount;
            InternalAddRef();

            return *this;
        }

        /// Convert to a shared pointer. If expired, return a null shared pointer.
        RefPtr<T> Lock() const
        {
            if (IsExpired())
                return RefPtr<T>();

            return RefPtr<T>(ptr_);
        }

        /// Return raw pointer. If expired, return null.
        T* Get() const
        {
            return IsExpired() ? nullptr : ptr_;
        }

        /// Point to the object.
        T* operator ->() const
        {
            T* rawPtr = Get();
            ALIMER_ASSERT(rawPtr);
            return rawPtr;
        }

        /// Dereference the object.
        T& operator *() const
        {
            T* rawPtr = Get();
            ALIMER_ASSERT(rawPtr);
            return *rawPtr;
        }

        /// Subscript the object if applicable.
        T& operator [](size_t index)
        {
            T* rawPtr = Get();
            ALIMER_ASSERT(rawPtr);
            return (*rawPtr)[index];
        }

        /// Test for equality with another weak pointer.
        template <class U> bool operator ==(const WeakPtr<U>& rhs) const { return ptr_ == rhs.ptr_ && refCount_ == rhs.refCount_; }

        /// Test for inequality with another weak pointer.
        template <class U> bool operator !=(const WeakPtr<U>& rhs) const { return ptr_ != rhs.ptr_ || refCount_ != rhs.refCount_; }

        /// Test for less than with another weak pointer.
        template <class U> bool operator <(const WeakPtr<U>& rhs) const { return ptr_ < rhs.ptr_; }

        /// Convert to a raw pointer, null if the object is expired.
        operator T* () const { return Get(); }   // NOLINT(google-explicit-constructor)

        /// Swap with another WeakPtr.
        void Swap(WeakPtr<T>& rhs)
        {
            std::swap(ptr_, rhs.ptr_);
            std::swap(refCount_, rhs.refCount_);
        }

        /// Reset with another pointer.
        void Reset(T* ptr = nullptr)
        {
            WeakPtr<T> copy(ptr);
            Swap(copy);
        }

        /// Perform a static cast from a weak pointer of another type.
        template <class U> void StaticCast(const WeakPtr<U>& rhs)
        {
            InternalRelease();
            ptr_ = static_cast<T*>(rhs.Get());
            refCount_ = rhs.refCount_;
            InternalAddRef();
        }

        /// Perform a dynamic cast from a weak pointer of another type.
        template <class U> void DynamicCast(const WeakPtr<U>& rhs)
        {
            InternalRelease();
            ptr_ = dynamic_cast<T*>(rhs.Get());

            if (ptr_)
            {
                refCount_ = rhs.refCount_;
                InternalAddRef();
            }
            else
            {
                refCount_ = nullptr;
            }
        }

        /// Check if the pointer is null.
        bool IsNull() const { return refCount_ == nullptr; }

        /// Check if the pointer is not null.
        bool IsNotNull() const { return refCount_ != nullptr; }

        /// Return the object's reference count, or 0 if null pointer or if object has expired.
        int32_t Refs() const { return (refCount_ && refCount_->refs >= 0) ? refCount_->refs : 0; }

        /// Return the object's weak reference count.
        int32_t WeakRefs() const
        {
            if (!IsExpired())
                return ptr_->WeakRefs();
            else
                return refCount_ ? refCount_->weakRefs : 0;
        }

        /// Return whether the object has expired. If null pointer, always return true.
        bool IsExpired() const { return refCount_ ? refCount_->refs < 0 : true; }

        /// Return pointer to the RefCount structure.
        RefCount* RefCountPtr() const { return refCount_; }
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

    /// Construct SharedPtr.
    template <class T, class ... Args> RefPtr<T> MakeRef(Args && ... args)
    {
        return RefPtr<T>(new T(std::forward<Args>(args)...));
    }
}
