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

#include "core/Assert.h"
#include <atomic>
#include <cstdint>

namespace alimer
{
    /// Base class for intrusively reference counted objects that can be pointed to with RefPtr. These are noncopyable and non-assignable.
    class ALIMER_CLASS_API RefCounted
    {
    public:
        /// Constructor
        RefCounted()
        {
            count.store(1, std::memory_order_relaxed);
        }

        /// Destructor, asserting that the reference count is 1.
        virtual ~RefCounted()
        {
#if ALIMER_ENABLE_ASSERT
            auto refs = GetRefCount();
            ALIMER_ASSERT_MSG(refs == 1, "RefCounted was %d", refs);
            count.store(1, std::memory_order_relaxed);
#endif
        }

        bool IsUnique() const
        {
            if (count.load(std::memory_order_acquire) == 1)
            {
                return true;
            }

            return false;
        }

        /// Add a strong reference.
        void AddRef()
        {
#if ALIMER_ENABLE_ASSERT
            ALIMER_ASSERT(GetRefCount() > 0);
#endif
            // No barrier required.
            count.fetch_add(1, std::memory_order_relaxed);
        }

        /// Release a strong reference.
        void Release()
        {
#if ALIMER_ENABLE_ASSERT
            ALIMER_ASSERT(GetRefCount() > 0);
#endif
            auto result = count.fetch_sub(1, std::memory_order_acq_rel);
            if (1 == result)
            {
                DeleteThis();
            }
        }

        /// Return the number of strong references.
        uint32_t GetRefCount() const
        {
            return count.load(std::memory_order_relaxed);
        }

    protected:
        // A Derived class may override this if they require a custom deleter.
        virtual void DeleteThis()
        {
#ifdef ALIMER_ENABLE_ASSERT
            ALIMER_ASSERT(0 == GetRefCount());
            count.store(1, std::memory_order_relaxed);
#endif
            delete this;
        }

    private:
        std::atomic_uint32_t count;

        RefCounted(RefCounted&&) = delete;
        RefCounted(const RefCounted&) = delete;
        RefCounted& operator=(RefCounted&&) = delete;
        RefCounted& operator=(const RefCounted&) = delete;
    };

    template <typename T> static inline T* AddReference(T* obj)
    {
        ALIMER_ASSERT(obj);
        obj->AddRef();
        return obj;
    }

    template <typename T> static inline T* SafeAddReference(T* obj)
    {
        if (obj)
        {
            obj->AddRef();
        }

        return obj;
    }

    template <typename T> static inline void SafeRelease(T* obj)
    {
        if (obj) {
            obj->Release();
        }
    }


    /// Pointer which holds a strong reference to a RefCounted subclass and allows shared ownership.
    template <class T> class RefPtr
    {
    public:
        /// Construct a null pointer.
        constexpr RefPtr() : ptr_(nullptr) {}

        /// Construct a null shared pointer.
        constexpr RefPtr(std::nullptr_t) : ptr_(nullptr) {}

        RefPtr(const RefPtr<T>& rhs) : ptr_(SafeAddReference(rhs.Get()))
        {
        }

        template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
        RefPtr(const RefPtr<U>& rhs) : ptr_(SafeAddReference(rhs.Get()))
        {
        }

        RefPtr(RefPtr<T>&& rhs) : ptr_(rhs.Release())
        {
        }

        template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
        RefPtr(RefPtr<U>&& rhs) : ptr_(rhs.Release())
        {
        }

        explicit RefPtr(T* obj) : ptr_(obj) {}

        /// Destruct. Release the object reference.
        ~RefPtr() noexcept
        {
            SafeRelease(ptr_);
        }

        RefPtr<T>& operator=(std::nullptr_t) { this->Reset(); return *this; }

        /// Assign from a raw pointer.
        RefPtr<T>& operator=(const RefPtr<T>& rhs)
        {
            if (this != &rhs) {
                this->Reset(SafeAddReference(rhs.Get()));
            }
            return *this;
        }

        template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
        RefPtr<T>& operator=(const RefPtr<U>& rhs)
        {
            this->Reset(SafeAddReference(rhs.Get()));
            return *this;
        }

        RefPtr<T>& operator=(RefPtr<T>&& rhs)
        {
            this->Reset(rhs.Release());
            return *this;
        }

        template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
        RefPtr<T>& operator=(RefPtr<U>&& rhs)
        {
            this->Reset(rhs.release());
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

        T* Release() {
            T* ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }

        void Swap(RefPtr<T>& rhs)
        {
            std::swap(ptr_, rhs.ptr_);
        }

        /// Reset with another pointer.
        void Reset(T* ptr = nullptr)
        {
            T* oldPtr = ptr_;
            ptr_ = ptr;
            SafeRelease(oldPtr);
        }

        /// Check if the pointer is null.
        bool IsNull() const { return ptr_ == nullptr; }

        /// Check if the pointer is not null.
        bool IsNotNull() const { return ptr_ != nullptr; }

        operator bool() const noexcept { return ptr_ != nullptr; }

        /// Return the raw pointer.
        T* Get() const noexcept { return ptr_; }

        /// Return hash value for HashSet & HashMap.
        size_t ToHash() const { return ((size_t)ptr_ / sizeof(T)); }

    private:
        /// Object pointer.
        T* ptr_;
    };

    template <typename T> inline void Swap(RefPtr<T>& a, RefPtr<T>& b)
    {
        a.Swap(b);
    }

    template <typename T, typename U> inline bool operator==(const RefPtr<T>& a, const RefPtr<U>& b)
    {
        return a.get() == b.get();
    }

    template <typename T> inline bool operator==(const RefPtr<T>& a, std::nullptr_t)
    {
        return !a;
    }

    template <typename T> inline bool operator==(std::nullptr_t, const RefPtr<T>& b)
    {
        return !b;
    }

    template <typename T, typename U> inline bool operator!=(const RefPtr<T>& a, const RefPtr<U>& b)
    {
        return a.get() != b.get();
    }

    template <typename T> inline bool operator!=(const RefPtr<T>& a, std::nullptr_t)
    {
        return static_cast<bool>(a);
    }

    template <typename T> inline bool operator!=(std::nullptr_t, const RefPtr<T>& b)
    {
        return static_cast<bool>(b);
    }

    template <typename T, typename... Args>
    RefPtr<T> MakeRefPtr(Args&&... args) {
        return RefPtr<T>(new T(std::forward<Args>(args)...));
    }

    template <typename T> RefPtr<T> ConstructRefPtr(T* obj)
    {
        return RefPtr<T>(SafeAddReference(obj));
    }

    template <typename T> RefPtr<T> ConstructRefPtr(const T* obj)
    {
        return RefPtr<T>(const_cast<T*>(SafeAddReference(obj)));
    }
}
