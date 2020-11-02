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

#include "Core/Ptr.h"

namespace alimer
{
    RefCounted::RefCounted() : refCount(new RefCount())
    {
        // Hold a weak ref to self to avoid possible double delete of the refcount
        refCount->weakRefs++;
    }

    RefCounted::~RefCounted()
    {
        ALIMER_ASSERT(refCount);
        ALIMER_ASSERT(refCount->refs == 0);
        ALIMER_ASSERT(refCount->weakRefs > 0);

        // Mark object as expired, release the self weak ref and delete the refcount if no other weak refs exist
        refCount->refs = -1;
        if (AtomicDecrement(&refCount->weakRefs) == 0)
        {
            delete refCount;
        }

        refCount = nullptr;
    }

    int32_t RefCounted::AddRef()
    {
        int32_t refs = AtomicIncrement(&refCount->refs);
        ALIMER_ASSERT(refs > 0);
        return refs;
    }

    int32_t RefCounted::Release()
    {
        int32_t refs = AtomicDecrement(&refCount->refs);
        ALIMER_ASSERT(refs >= 0);

        if (refs == 0)
        {
            delete this;
        }

        return refs;
    }

    int32_t RefCounted::Refs() const { return refCount->refs; }

    int32_t RefCounted::WeakRefs() const
    {
        // Subtract one to not return the internally held reference
        return refCount->weakRefs - 1;
    }
}
