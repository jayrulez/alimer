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
        /// Construct.
        RefCount() : refs(0), weakRefs(0)
        {
        }

        /// Destruct.
        ~RefCount()
        {
            // Set reference counts below zero to fire asserts if this object is still accessed
            refs = static_cast<uint32_t>(-1);
            weakRefs = static_cast<uint32_t>(-1);
        }

        /// Number of strong references. These keep the object alive.
        uint32_t refs{ 0 };
        /// Number of weak references.
        uint32_t weakRefs{ 0 };
    };

    /// Base class for intrusively reference counted objects that can be pointed to with SharedPtr and WeakPtr. These are noncopyable and non-assignable.
    class ALIMER_API RefCounted
    {
    public:
        /// Construct. Allocate the reference count structure and set an initial self weak reference.
        RefCounted();
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
        RefCount* RefCountPtr() { return refCount; }

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
        T& operator [](size_t index)
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
        uint32_t Reset()
        {
            return Release();
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
        bool IsNotNull() const { return ptr_ != nullptr; }

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

    /// Weak pointer template class with intrusive reference counting. Does not keep the object pointed to alive.
    template <class T> class WeakPtr
    {
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
            AddRef();
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
            AddRef();
        }

        /// Construct from a shared pointer.
        WeakPtr(const SharedPtr<T>& rhs) noexcept : ptr_(rhs.Get()), refCount_(rhs.RefCountPtr())
        {
            AddRef();
        }

        /// Construct from a raw pointer.
        explicit WeakPtr(T* ptr) noexcept : ptr_(ptr), refCount_(ptr ? ptr->RefCountPtr() : nullptr)
        {
            AddRef();
        }

        /// Destruct. Release the weak reference to the object.
        ~WeakPtr() noexcept
        {
            Release();
        }

        /// Assign from a shared pointer.
        WeakPtr<T>& operator =(const SharedPtr<T>& rhs)
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

            Release();
            ptr_ = rhs.ptr_;
            refCount_ = rhs.refCount_;
            AddRef();

            return *this;
        }

        /// Assign from a raw pointer.
        WeakPtr<T>& operator =(T* ptr)
        {
            RefCount* refCount = ptr ? ptr->RefCountPtr() : nullptr;

            if (ptr_ == ptr && refCount_ == refCount)
                return *this;

            Release();
            ptr_ = ptr;
            refCount_ = refCount;
            AddRef();

            return *this;
        }

        /// Convert to a shared pointer. If expired, return a null shared pointer.
        SharedPtr<T> Lock() const
        {
            if (!IsExpired())
                return SharedPtr<T>(ptr_);

            return SharedPtr<T>();
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
            Urho3D::Swap(ptr_, rhs.ptr_);
            Urho3D::Swap(refCount_, rhs.refCount_);
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
            Release();
            ptr_ = static_cast<T*>(rhs.Get());
            refCount_ = rhs.refCount_;
            AddRef();
        }

        /// Perform a dynamic cast from a weak pointer of another type.
        template <class U> void DynamicCast(const WeakPtr<U>& rhs)
        {
            Release();
            ptr_ = dynamic_cast<T*>(rhs.Get());

            if (ptr_)
            {
                refCount_ = rhs.refCount_;
                AddRef();
            }
            else
            {
                refCount_ = 0;
            }
        }

        /// Check if the pointer is null.
        bool IsNull() const { return refCount_ == nullptr; }

        /// Check if the pointer is not null.
        bool IsNotNull() const { return refCount_ != nullptr; }

        /// Return the object's reference count, or 0 if null pointer or if object has expired.
        uint32_t Refs() const { return (refCount_ && refCount_->refs >= 0) ? refCount_->refs : 0; }

        /// Return the object's weak reference count.
        uint32_t WeakRefs() const
        {
            if (!IsExpired())
                return ptr_->WeakRefs();

            return refCount_ ? refCount_->weakRefs : 0;
        }

        /// Return whether the object has expired. If null pointer, always return true.
        bool IsExpired() const { return refCount_ ? refCount_->refs < 0 : true; }

        /// Return pointer to the RefCount structure.
        RefCount* RefCountPtr() const { return refCount_; }

        /// Return hash value for HashSet & HashMap.
        size_t ToHash() const { return (size_t)((size_t)ptr_ / sizeof(T)); }

    private:
        template <class U> friend class WeakPtr;

        /// Add a weak reference to the object pointed to.
        void AddRef()
        {
            if (refCount_)
            {
                ALIMER_ASSERT(refCount_->weakRefs >= 0);
                ++(refCount_->weakRefs);
            }
        }

        /// Release the weak reference. Delete the Refcount structure if necessary.
        void Release()
        {
            if (refCount_)
            {
                ALIMER_ASSERT(refCount_->weakRefs > 0);
                --(refCount_->weakRefs);

                if (IsExpired() && !refCount_->weakRefs)
                    delete refCount_;
            }

            ptr_ = nullptr;
            refCount_ = nullptr;

            ptr_ = nullptr;
            refCount_ = nullptr;
        }

        /// Pointer to the object.
        T* ptr_;
        /// Pointer to the RefCount structure.
        RefCount* refCount_;
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

    /// Perform a static cast from one weak pointer type to another.
    template <class T, class U> WeakPtr<T> StaticCast(const WeakPtr<U>& ptr)
    {
        WeakPtr<T> ret;
        ret.StaticCast(ptr);
        return ret;
    }

    /// Perform a dynamic cast from one weak pointer type to another.
    template <class T, class U> WeakPtr<T> DynamicCast(const WeakPtr<U>& ptr)
    {
        WeakPtr<T> ret;
        ret.DynamicCast(ptr);
        return ret;
    }

    /// Construct SharedPtr.
    template <class T, class ... Args> SharedPtr<T> MakeShared(Args&& ... args)
    {
        return SharedPtr<T>(new T(std::forward<Args>(args)...));
    }
}
