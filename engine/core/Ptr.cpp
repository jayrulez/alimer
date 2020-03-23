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

#include "core/Ptr.h"

namespace alimer
{
    RefCounted::RefCounted()
        : refCount(new RefCount())
    {
        // Hold a weak ref to self to avoid possible double delete of the refcount
        (refCount->weakRefs)++;
    }

    RefCounted::~RefCounted()
    {
        ALIMER_ASSERT(refCount);
        ALIMER_ASSERT(refCount->refs == 0);
        ALIMER_ASSERT(refCount->weakRefs > 0);

        // Mark object as expired, release the self weak ref and delete the refcount if no other weak refs exist
        refCount->refs = static_cast<uint32_t>(-1);
        (refCount->weakRefs)--;
        if (!refCount->weakRefs)
        {
            delete refCount;
        }

        refCount = nullptr;
    }

    uint32_t RefCounted::AddRef()
    {
        // TODO: Atomics
        ALIMER_ASSERT(refCount->refs >= 0);
        (refCount->refs)++;
        return refCount->refs;
    }

    uint32_t RefCounted::Release()
    {
        ALIMER_ASSERT(refCount->refs > 0);
        (refCount->refs)--;
        if (!refCount->refs)
        {
            delete this;
            return 0;
        }

        return refCount->refs;
    }

    uint32_t RefCounted::Refs() const
    {
        return refCount->refs;
    }

    uint32_t RefCounted::WeakRefs() const
    {
        // Subtract one to not return the internally held reference
        return refCount->weakRefs - 1;
    }
}
