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

// Implementation based on Urho3D RefCounted: https://github.com/urho3d/Urho3D/blob/master/LICENSE
#pragma once

#include "core/Assert.h"
#include <cstddef>
#include <utility>

namespace alimer
{
    class RefCounted;
    template <class T> class WeakPtr;

    /// Reference count structure. Used in both intrusive and non-intrusive reference counting.
    struct ALIMER_API RefCount
    {
        /// Construct with zero refcounts.
        RefCount() = default;

        /// Number of strong references. These keep the object alive.
        uint32_t refs{0};
        /// Number of weak references.
        uint32_t weakRefs{0};
        /// Expired flag. The object is no longer safe to access after this is set true.
        bool expired{ false };
    };

    /// Base class for intrusively reference counted objects that can be pointed to with SharedPtr and WeakPtr. These are noncopyable and non-assignable.
    class ALIMER_API RefCounted
    {
    public:
        /// Construct. Allocate the reference count structure and set an initial self weak reference.
        RefCounted() = default;
        /// Destruct. Mark as expired and also delete the reference count structure if no outside weak references exist.
        virtual ~RefCounted();

        /// Add a strong reference. Allocate the reference count structure first if necessary.
        uint32_t AddRef();
        /// Release a strong reference. Destroy the object when the last strong reference is gone.
        uint32_t Release();

        /// Return the number of strong references.
        uint32_t Refs() const;
        /// Return the number of weak references.
        uint32_t WeakRefs() const;
        /// Return pointer to the reference count structure. Allocate if not allocated yet.
        RefCount* RefCountPtr();

        RefCounted(const RefCounted&) = delete;
        RefCounted& operator=(const RefCounted&) = delete;
        RefCounted(RefCounted&&) = delete;
        RefCounted& operator=(RefCounted&&) = delete;

    private:
        /// Reference count structure, allocated on demand.
        RefCount* refCount = nullptr;
    };

    /// Pointer which holds a strong reference to a RefCounted subclass and allows shared ownership.
    template <class T> class SharedPtr
    {
    private:
        template <class U> friend class SharedPtr;

        /// Add a reference to the object pointed to.
        void AddRef()
        {
            if (ptr_)
                ptr_->AddRef();
        }

        /// Release the object reference and delete it if necessary.
        uint32_t Release()
        {
            uint32_t ref = 0;
            T* temp = ptr_;

            if (temp != nullptr)
            {
                ptr_ = nullptr;
                ref = temp->Release();
            }

            return ref;
        }

        /// Object pointer.
        T* ptr_;

    public:
        /// Construct a null pointer.
        SharedPtr() noexcept : ptr_(nullptr)
        {
        }

        /// Construct a null shared pointer.
        SharedPtr(std::nullptr_t) noexcept : ptr_(nullptr)
        {
        }

        /// Copy-construct from another pointer allowing implicit upcasting.
        template<class U> SharedPtr(U* other) noexcept : ptr_(other)
        {
            AddRef();
        }

        /// Copy-construct from another shared pointer.
        SharedPtr(const SharedPtr<T>& rhs) noexcept : ptr_(rhs.ptr_)
        {
            AddRef();
        }

        /// Move-construct from another shared pointer.
        SharedPtr(SharedPtr<T>&& rhs) noexcept : ptr_(rhs.ptr_)
        {
            rhs.ptr_ = nullptr;
        }


        /// Copy-construct from another shared pointer allowing implicit upcasting.
        template <class U> SharedPtr(const SharedPtr<U>& rhs) noexcept : ptr_(rhs.ptr_)
        {
            AddRef();
        }

        /// Destruct. Release the object reference.
        ~SharedPtr() noexcept
        {
            Release();
        }

        SharedPtr<T>& operator=(std::nullptr_t) noexcept
        {
            Release();
            return *this;
        }

        /// Assign from a raw pointer.
        SharedPtr<T>& operator =(T* other) noexcept
        {
            if (ptr_ != other)
            {
                SharedPtr<T>(other).Swap(*this);
            }

            return *this;
        }

        template <typename U>
        SharedPtr& operator=(U* other) noexcept
        {
            SharedPtr(other).Swap(*this);
            return *this;
        }

        /// Assign from another shared pointer.
        SharedPtr<T>& operator =(const SharedPtr<T>& rhs)
        {
            if (ptr_ != rhs.ptr_)
            {
                SharedPtr(other).Swap(*this);
            }

            return *this;
        }

        /// Move-assign from another shared pointer.
        SharedPtr<T>& operator =(SharedPtr<T>&& rhs)
        {
            SharedPtr<T> copy(std::move(rhs));
            Swap(copy);

            return *this;
        }

        /// Assign from another shared pointer allowing implicit upcasting.
        template <class U> SharedPtr<T>& operator =(const SharedPtr<U>& rhs)
        {
            SharedPtr(other).Swap(*this);
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
            ALIMER_ASSERT(ptr_);
            return *ptr_;
        }

        /// Subscript the object if applicable.
        T& operator [](int index)
        {
            ALIMER_ASSERT(ptr_);
            return ptr_[index];
        }

        /// Test for less than with another shared pointer.
        template <class U> bool operator <(const SharedPtr<U>& rhs) const { return ptr_ < rhs.ptr_; }

        /// Test for equality with another shared pointer.
        template <class U> bool operator ==(const SharedPtr<U>& rhs) const { return ptr_ == rhs.ptr_; }

        /// Test for inequality with another shared pointer.
        template <class U> bool operator !=(const SharedPtr<U>& rhs) const { return ptr_ != rhs.ptr_; }

        /// Convert to a raw pointer.
        operator T* () const { return ptr_; }

        void Swap(SharedPtr&& rhs) throw()
        {
            T* tmp = ptr_;
            ptr_ = rhs.ptr_;
            rhs.ptr_ = tmp;
        }

        void Swap(SharedPtr& rhs)
        {
            T* tmp = ptr_;
            ptr_ = rhs.ptr_;
            rhs.ptr_ = tmp;
        }

        /// Reset with another pointer.
        void Reset(T* ptr = nullptr)
        {
            SharedPtr<T> copy(ptr);
            Swap(copy);
        }

        T* const* GetAddressOf() const noexcept { return &ptr_; }
        T** GetAddressOf() noexcept { return &ptr_; }

        T** ReleaseAndGetAddressOf() noexcept
        {
            Release();
            return &ptr_;
        }

        /// Detach without destroying the object even if the refcount goes zero. To be used for scripting language interoperation.
        T* Detach()
        {
            T* ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }

        /// Perform a static cast from a shared pointer of another type.
        template <class U> void StaticCast(const SharedPtr<U>& rhs)
        {
            SharedPtr<T> copy(static_cast<T*>(rhs.Get()));
            Swap(copy);
        }

        /// Perform a dynamic cast from a shared pointer of another type.
        template <class U> void DynamicCast(const SharedPtr<U>& rhs)
        {
            SharedPtr<T> copy(dynamic_cast<T*>(rhs.Get()));
            Swap(copy);
        }

        /// Check if the pointer is null.
        bool IsNull() const { return ptr_ == nullptr; }

        /// Check if the pointer is not null.
        bool NotNull() const { return ptr_ != nullptr; }

        operator bool() const noexcept { return ptr_ != nullptr; }

        /// Return the raw pointer.
        T* Get() const noexcept { return ptr_; }

        /// Return the object's reference count, or 0 if the pointer is null.
        uint32_t Refs() const { return ptr_ ? ptr_->Refs() : 0; }

        /// Return the object's weak reference count, or 0 if the pointer is null.
        uint32_t WeakRefs() const { return ptr_ ? ptr_->WeakRefs() : 0; }

        /// Return pointer to the RefCount structure.
        RefCount* RefCountPtr() const { return ptr_ ? ptr_->RefCountPtr() : nullptr; }

        /// Return hash value for HashSet & HashMap.
        size_t ToHash() const { return ((size_t)ptr_ / sizeof(T)); }
    };

    /// Perform a static cast between shared pointers of two types.
    template <class T, class U> SharedPtr<T> StaticCast(const SharedPtr<U>& rhs)
    {
        SharedPtr<T> ret;
        ret.StaticCast(rhs);
        return ret;
    }

    /// Perform a dynamic cast between shared pointers of two types.
    template <class T, class U> SharedPtr<T> DynamicCast(const SharedPtr<U>& rhs)
    {
        SharedPtr<T> ret;
        ret.DynamicCast(rhs);
        return ret;
    }
}
