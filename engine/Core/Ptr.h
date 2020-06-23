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
    };

    template <typename T> inline void Swap(RefPtr<T>& a, RefPtr<T>& b)
    {
        a.Swap(b);
    }
}
