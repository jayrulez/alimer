//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#include "Scene/EntityManager.h"
#include "Core/Log.h"

namespace alimer
{
    void EntityManager::AddRoot(Entity* entity)
    {
        ALIMER_ASSERT(entity);

        if (entity->parent != nullptr)
        {
            LOGE("Entity has already a parent");
        }

        Add(entity);
    }

    void EntityManager::RemoveRoot(Entity* entity)
    {
        Remove(entity);
    }

    void EntityManager::Add(Entity* entity)
    {
        if (entities.find(entity) != entities.end()) {
            return;
        }

        if (entity->manager != nullptr)
        {
            LOGE("This entity is already used by another entity manager.");
        }

        entity->SetEntityManager(this);
        entities.insert(entity);
    }

    void EntityManager::Remove(Entity* entity)
    {
        if (!entities.erase(entity)) return;
    }
}
