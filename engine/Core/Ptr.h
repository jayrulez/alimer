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
    struct ALIMER_API RefCount
    {
    public:
        /// Constructor.
        RefCount() = default;

        /// Destructor.
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
        /// Constructor
        RefCounted()
            : refCount(new RefCount())
        {
            // Hold a weak ref to self to avoid possible double delete of the refcount
            refCount->weakRefs++;
        }

        /// Destructor, asserting that the reference count is 1.
        virtual ~RefCounted()
        {
#if ALIMER_ENABLE_ASSERT
            ALIMER_ASSERT(refCount);
            ALIMER_ASSERT(refCount->refs == 0);
            ALIMER_ASSERT(refCount->weakRefs > 0);
#endif

            // Mark object as expired, release the self weak ref and delete the refcount if no other weak refs exist
            refCount->refs = -1;

            if (AtomicDecrement(&refCount->weakRefs) == 0) {
                delete refCount;
            }

            refCount = nullptr;
        }

        /// Add a strong reference.
        int32_t AddRef()
        {
            int32_t refs = AtomicIncrement(&refCount->refs);
            ALIMER_ASSERT(refs > 0);
            return refs;
        }

        /// Release a strong reference.
        int32_t Release()
        {
            int32_t refs = AtomicDecrement(&refCount->refs);
            ALIMER_ASSERT(refs >= 0);

            if (refs == 0)
            {
                DeleteThis();
            }

            return refs;
        }

        /// Return the reference count.
        int32_t Refs() const
        {
            return refCount->refs;
        }

        /// Return the weak reference count.
        int32_t WeakRefs() const
        {
            // Subtract one to not return the internally held reference
            return refCount->weakRefs - 1;
        }

    protected:
        // A Derived class may override this if they require a custom deleter.
        virtual void DeleteThis()
        {
            delete this;
        }

    private:
        /// Pointer to the reference count structure.
        RefCount* refCount = nullptr;

        RefCounted(RefCounted&&) = delete;
        RefCounted(const RefCounted&) = delete;
        RefCounted& operator=(RefCounted&&) = delete;
        RefCounted& operator=(const RefCounted&) = delete;
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

        template<class U>
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
        template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
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

        /// Perform a dynamic cast from a shared pointer of another type.
        template <class U> void DynamicCast(const RefPtr<U>& rhs)
        {
            RefPtr<T> copy(dynamic_cast<T*>(rhs.Get()));
            Swap(copy);
        }
    };

    template <typename T> inline void Swap(RefPtr<T>& a, RefPtr<T>& b)
    {
        a.Swap(b);
    }

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

    /// Delete object of type T. T must be complete. See boost::checked_delete.
    template<class T> inline void CheckedDelete(T* x)
    {
        // intentionally complex - simplification causes regressions
        using type_must_be_complete = char[sizeof(T) ? 1 : -1];
        (void)sizeof(type_must_be_complete);
        delete x;
    }

    /// Unique pointer template class.
    template <class T> class UniquePtr
    {
    public:
        /// Construct empty.
        UniquePtr() : ptr_(nullptr) { }

        /// Construct from pointer.
        explicit UniquePtr(T* ptr) : ptr_(ptr) { }

        /// Prevent copy construction.
        UniquePtr(const UniquePtr&) = delete;
        /// Prevent assignment.
        UniquePtr& operator=(const UniquePtr&) = delete;

        /// Assign from pointer.
        UniquePtr& operator = (T* ptr)
        {
            Reset(ptr);
            return *this;
        }

        /// Construct empty.
        UniquePtr(std::nullptr_t) : ptr_(nullptr) { }   // NOLINT(google-explicit-constructor)

        /// Move-construct from UniquePtr.
        UniquePtr(UniquePtr&& up) noexcept : ptr_(up.Detach()) {}

        /// Move-assign from UniquePtr.
        UniquePtr& operator =(UniquePtr&& up) noexcept
        {
            Reset(up.Detach());
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

        /// Test for less than with another unique pointer.
        template <class U>
        bool operator <(const UniquePtr<U>& rhs) const { return ptr_ < rhs.ptr_; }

        /// Test for equality with another unique pointer.
        template <class U>
        bool operator ==(const UniquePtr<U>& rhs) const { return ptr_ == rhs.ptr_; }

        /// Test for inequality with another unique pointer.
        template <class U>
        bool operator !=(const UniquePtr<U>& rhs) const { return ptr_ != rhs.ptr_; }

        /// Cast pointer to bool.
        operator bool() const { return !!ptr_; }    // NOLINT(google-explicit-constructor)

        /// Swap with another UniquePtr.
        void Swap(UniquePtr& up) { std::swap(ptr_, up.ptr_); }

        /// Detach pointer from UniquePtr without destroying.
        T* Detach()
        {
            T* ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }

        /// Check if the pointer is null.
        bool IsNull() const { return ptr_ == nullptr; }

        /// Check if the pointer is not null.
        bool IsNotNull() const { return ptr_ != nullptr; }

        /// Return the raw pointer.
        T* Get() const { return ptr_; }

        /// Reset.
        void Reset(T* ptr = nullptr)
        {
            CheckedDelete(ptr_);
            ptr_ = ptr;
        }

        /// Destruct.
        ~UniquePtr()
        {
            Reset();
        }

    private:
        T* ptr_;
    };

    /// Pointer which takes ownership of an array allocated with new[] and deletes it when the pointer goes out of scope.
    template <class T> class UniqueArrayPtr
    {
    public:
        /// Construct a null pointer.
        UniqueArrayPtr() : ptr_(nullptr) { }

        /// Construct and take ownership of the array.
        explicit UniqueArrayPtr(T* array_) : ptr_(array_)
        {
        }

        /// Construct empty.
        UniqueArrayPtr(std::nullptr_t) : ptr_(nullptr) { }   // NOLINT(google-explicit-constructor)

        /// Prevent copy construction.
        UniqueArrayPtr(const UniqueArrayPtr&) = delete;
        /// Prevent assignment.
        UniqueArrayPtr& operator=(const UniqueArrayPtr&) = delete;

        /// Move-construct from UniquePtr.
        UniqueArrayPtr(UniqueArrayPtr&& up) noexcept : ptr_(up.Detach()) {}

        /// Move-assign from UniquePtr.
        UniqueArrayPtr& operator =(UniqueArrayPtr&& up) noexcept
        {
            Reset(up.Detach());
            return *this;
        }

        /// Destruct. Delete the array pointed to.
        ~UniqueArrayPtr()
        {
            Reset();
        }

        /// Reset.
        void Reset(T* ptr = nullptr)
        {
            delete[] ptr_;
            ptr_ = ptr;
        }

        /// Assign a new array. Existing array is deleted.
        UniqueArrayPtr<T>& operator= (T* ptr)
        {
            Reset(ptr);
            return *this;
        }

        /// Point to the object.
        T* operator -> () const { ALIMER_ASSERT(ptr_); return ptr_; }
        /// Dereference the array.
        T& operator * () const { ALIMER_ASSERT(ptr_); return *ptr_; }
        /// Index the array.
        T& operator [] (size_t index) { ALIMER_ASSERT(ptr_); return ptr_[index]; }
        /// Const-index the array.
        const T& operator [] (size_t index) const { ALIMER_ASSERT(ptr_); return ptr_[index]; }
        /// Cast pointer to bool.
        operator bool() const { return !!ptr_; }    // NOLINT(google-explicit-constructor)

        /// Swap with another UniqueArrayPtr.
        void Swap(UniqueArrayPtr& rhs) { std::swap(ptr_, rhs.ptr_); }

        /// Detach the array from the pointer without destroying it and return it. The pointer becomes null.
        T* Detach()
        {
            T* ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }

        /// Check if the pointer is null.
        bool IsNull() const { return ptr_ == nullptr; }

        /// Check if the pointer is not null.
        bool IsNotNull() const { return ptr_ != nullptr; }

        /// Return the raw pointer.
        T* Get() const { return ptr_; }

    private:
        /// Array pointer.
        T* ptr_;
    };

    /// Swap two UniquePtr-s.
    template <class T> void Swap(UniquePtr<T>& first, UniquePtr<T>& second)
    {
        first.Swap(second);
    }

    /// Swap two UniquePtr-s.
    template <class T> void Swap(UniqueArrayPtr<T>& first, UniqueArrayPtr<T>& second)
    {
        first.Swap(second);
    }

    /// Construct UniquePtr.
    template <class T, class ... Args> UniquePtr<T> MakeUnique(Args&& ... args)
    {
        return UniquePtr<T>(new T(std::forward<Args>(args)...));
    }

    /// Construct SharedPtr.
    template <class T, class ... Args> RefPtr<T> MakeShared(Args&& ... args)
    {
        return RefPtr<T>(new T(std::forward<Args>(args)...));
    }
}
