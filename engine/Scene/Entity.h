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

#pragma once

#include "Core/Object.h"
#include "Core/String.h"
#include "Math/Matrix4x4.h"

namespace alimer
{
    class EntityManager;

    class ALIMER_API Entity final : public Object
    {
        ALIMER_OBJECT(Entity, Object);

        friend class EntityManager;

    public:
        Entity(const std::string& name = "");
        ~Entity();

        /// Return parent entity.
        Entity* GetParent() const { return parent; }

        /// Return the owning entity manager.
        EntityManager* GetEntityManager() const { return manager; }

    private:
        void SetEntityManager(EntityManager* newManager);

        std::string name;
        /// Parent scene node.
        Entity*        parent{nullptr};
        EntityManager* manager{nullptr};
    };
}
