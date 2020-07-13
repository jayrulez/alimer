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

    /// Base class for intrusively reference counted objects that can be pointed to with SharedPtr. These are noncopyable and non-assignable.
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

        /// Return pointer to the reference count structure.
        RefCount* RefCountPtr() { return refCount; }

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
    template <class T> class SharedPtr
    {
    protected:
        T* ptr_;
        template<class U> friend class SharedPtr;

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
        SharedPtr() noexcept : ptr_(nullptr) {}

        /// Construct a null shared pointer.
        SharedPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

        template<class U>
        SharedPtr(_In_opt_ U* other) noexcept : ptr_(other)
        {
            InternalAddRef();
        }

        /// Copy-construct from another shared pointer.
        SharedPtr(const SharedPtr& other) : ptr_(other.ptr_)
        {
            InternalAddRef();
        }

        /// Copy-construct from another shared pointer allowing implicit upcasting.
        template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
        SharedPtr(const SharedPtr<U>& other) noexcept : ptr_(other.ptr_)
        {
            InternalAddRef();
        }

        /// Move-construct from another shared pointer.
        SharedPtr(_Inout_ SharedPtr<T>&& rhs) noexcept : ptr_(rhs.ptr_)
        {
            rhs.ptr_ = nullptr;
        }

        /// Destruct. Release the object reference.
        ~SharedPtr() noexcept
        {
            InternalRelease();
        }

        SharedPtr<T>& operator=(std::nullptr_t) noexcept
        {
            InternalRelease();
            return *this;
        }

        SharedPtr<T>& operator=(_In_opt_ T* other) noexcept
        {
            if (ptr_ != other)
            {
                SharedPtr(other).Swap(*this);
            }

            return *this;
        }

        template <typename U>
        SharedPtr& operator=(_In_opt_ U* other) noexcept
        {
            SharedPtr(other).Swap(*this);
            return *this;
        }

        SharedPtr& operator =(const SharedPtr<T>& other) noexcept
        {
            if (ptr_ != other.ptr_)
            {
                SharedPtr(other).Swap(*this);
            }

            return *this;
        }

        template<class U>
        SharedPtr& operator=(const SharedPtr<U>& other) noexcept
        {
            SharedPtr(other).Swap(*this);
            return *this;
        }

        SharedPtr& operator=(_Inout_ SharedPtr&& other) noexcept
        {
            SharedPtr(static_cast<SharedPtr&&>(other)).Swap(*this);
            return *this;
        }

        template<class U>
        SharedPtr& operator=(_Inout_ SharedPtr<U>&& other) noexcept
        {
            SharedPtr(static_cast<SharedPtr<U>&&>(other)).Swap(*this);
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
        template <class U> bool operator <(const SharedPtr<U>& rhs) const { return ptr_ < rhs.ptr_; }

        /// Test for equality with another shared pointer.
        template <class U> bool operator ==(const SharedPtr<U>& rhs) const { return ptr_ == rhs.ptr_; }

        /// Test for inequality with another shared pointer.
        template <class U> bool operator !=(const SharedPtr<U>& rhs) const { return ptr_ != rhs.ptr_; }

        /// Check if the pointer is null.
        bool IsNull() const { return ptr_ == nullptr; }

        /// Check if the pointer is not null.
        bool IsNotNull() const { return ptr_ != nullptr; }

        /// Return the raw pointer.
        T* Get() const noexcept { return ptr_; }

        void Swap(_Inout_ SharedPtr&& r) noexcept
        {
            T* tmp = ptr_;
            ptr_ = r.ptr_;
            r.ptr_ = tmp;
        }

        void Swap(_Inout_ SharedPtr& rhs) noexcept
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
        template <class U> void StaticCast(const SharedPtr<U>& rhs)
        {
            SharedPtr<T> copy(static_cast<T*>(rhs.Get()));
            Swap(copy);
        }

        /// Perform a dynamic cast from a shared SharedPtr of another type.
        template <class U> void DynamicCast(const SharedPtr<U>& rhs)
        {
            SharedPtr<T> copy(dynamic_cast<T*>(rhs.Get()));
            Swap(copy);
        }
    };

    /// Weak pointer template class with intrusive reference counting. Does not keep the object pointed to alive.
    template <class T> class WeakPtr
    {
    public:
        /// Construct a null weak pointer.
        WeakPtr() noexcept
            : ptr_(nullptr)
            , refCount_(nullptr)
        {
        }

        /// Construct a null weak pointer.
        WeakPtr(std::nullptr_t) noexcept
            : ptr_(nullptr)
            , refCount_(nullptr)
        {
        }

        /// Copy-construct from another weak pointer.
        WeakPtr(const WeakPtr<T>& rhs) noexcept
            : ptr_(rhs.ptr_)
            , refCount_(rhs.refCount_)
        {
            AddRef();
        }

        /// Move-construct from another weak pointer.
        WeakPtr(WeakPtr<T>&& rhs) noexcept
            : ptr_(rhs.ptr_)
            , refCount_(rhs.refCount_)
        {
            rhs.ptr_ = nullptr;
            rhs.refCount_ = nullptr;
        }

        /// Copy-construct from another weak pointer allowing implicit upcasting.
        template <class U> WeakPtr(const WeakPtr<U>& rhs) noexcept
            : ptr_(rhs.ptr_)
            , refCount_(rhs.refCount_)
        {
            AddRef();
        }

        /// Construct from a shared pointer.
        WeakPtr(const SharedPtr<T>& rhs) noexcept
            : ptr_(rhs.Get())
            , refCount_(rhs.RefCountPtr())
        {
            AddRef();
        }

        /// Construct from a raw pointer.
        explicit WeakPtr(T* ptr) noexcept
            : ptr_(ptr)
            , refCount_(ptr ? ptr->RefCountPtr() : nullptr)
        {
            AddRef();
        }

        /// Destruct. Release the weak reference to the object.
        ~WeakPtr() noexcept
        {
            ReleaseRef();
        }

        /// Assign from a shared pointer.
        WeakPtr<T>& operator =(const SharedPtr<T>& rhs)
        {
            if (ptr_ == rhs.Get() && refCount_ == rhs.RefCountPtr()) {
                return *this;
            }

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

            ReleaseRef();
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

            ReleaseRef();
            ptr_ = ptr;
            refCount_ = refCount;
            AddRef();

            return *this;
        }

        /// Convert to a shared pointer. If expired, return a null shared pointer.
        SharedPtr<T> Lock() const
        {
            if (IsExpired())
                return SharedPtr<T>();

            return SharedPtr<T>(ptr_);
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
            ReleaseRef();
            ptr_ = static_cast<T*>(rhs.Get());
            refCount_ = rhs.refCount_;
            AddRef();
        }

        /// Perform a dynamic cast from a weak pointer of another type.
        template <class U> void DynamicCast(const WeakPtr<U>& rhs)
        {
            ReleaseRef();
            ptr_ = dynamic_cast<T*>(rhs.Get());

            if (ptr_)
            {
                refCount_ = rhs.refCount_;
                AddRef();
            }
            else
                refCount_ = 0;
        }

        /// Check if the pointer is null.
        bool IsNull() const { return refCount_ == nullptr; }

        /// Check if the pointer is not null.
        bool IsNotNull() const { return refCount_ != nullptr; }

        /// Return the object's reference count, or 0 if null pointer or if object has expired.
        int32_t Refs() const { return (refCount_ && refCount_->refs >= 0) ? refCount_->refs : 0; }

        /// Return the object's weak reference count.
        int WeakRefs() const
        {
            if (!IsExpired())
                return ptr_->WeakRefs();

            return refCount_ ? refCount_->weakRefs : 0;
        }

        /// Return whether the object has expired. If null pointer, always return true.
        bool IsExpired() const { return refCount_ ? refCount_->refs < 0 : true; }

        /// Return pointer to the RefCount structure.
        RefCount* RefCountPtr() const { return refCount_; }

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
        void ReleaseRef()
        {
            if (refCount_)
            {
                ALIMER_ASSERT(refCount_->weakRefs > 0);
                --(refCount_->weakRefs);

                if (IsExpired() && !refCount_->weakRefs) {
                    delete refCount_;
                }
            }

            ptr_ = nullptr;
            refCount_ = nullptr;
        }

        /// Pointer to the object.
        T* ptr_;
        /// Pointer to the RefCount structure.
        RefCount* refCount_;
    };

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

    /// Perform a static cast from one shared pointer type to another.
    template <class T, class U> SharedPtr<T> StaticCast(const SharedPtr<U>& ptr)
    {
        SharedPtr<T> ret;
        ret.StaticCast(ptr);
        return ret;
    }

    /// Perform a dynamic cast from one weak pointer type to another.
    template <class T, class U> SharedPtr<T> DynamicCast(const SharedPtr<U>& ptr)
    {
        SharedPtr<T> ret;
        ret.DynamicCast(ptr);
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

    template <typename T> inline void Swap(SharedPtr<T>& a, SharedPtr<T>& b)
    {
        a.Swap(b);
    }

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
    template <class T, class ... Args> SharedPtr<T> MakeShared(Args&& ... args)
    {
        return SharedPtr<T>(new T(std::forward<Args>(args)...));
    }
}
