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

#include "Core/Object.h"
#include "Platform/Input.h"
#include "Graphics/GraphicsDevice.h"
#include <unordered_map>
#include <memory>

namespace Alimer
{
    namespace details
    {
        struct Context
        {
            /// Object factories.
            std::unordered_map<StringId32, RefPtr<Object>> subsystems;
            std::unordered_map<StringId32, std::unique_ptr<ObjectFactory>> factories;

            WeakPtr<Input> input;
            WeakPtr<GraphicsDevice> graphics;

            void RegisterSubsystem(Object* subsystem)
            {
                subsystems[subsystem->GetType()] = subsystem;
            }

            void RemoveSubsystem(StringId32 subsystemType)
            {
                subsystems.erase(subsystemType);
            }

            Object* GetSubsystem(StringId32 type)
            {
                auto it = subsystems.find(type);
                return it != subsystems.end() ? it->second.Get() : nullptr;
            }

            void RegisterFactory(ObjectFactory* factory)
            {
                factories[factory->GetType()].reset(factory);
            }

            RefPtr<Object> CreateObject(StringId32 type)
            {
                auto it = factories.find(type);
                return it != factories.end() ? it->second->Create() : nullptr;
            }
        };

        Context& context() {
            static Context s_context;
            return s_context;
        }
    }

    TypeInfo::TypeInfo(const char* typeName_, const TypeInfo* baseTypeInfo_)
        : type(typeName_)
        , typeName(typeName_)
        , baseTypeInfo(baseTypeInfo_)
    {

    }

    bool TypeInfo::IsTypeOf(StringId32 type) const
    {
        const TypeInfo* current = this;
        while (current)
        {
            if (current->GetType() == type)
                return true;

            current = current->GetBaseTypeInfo();
        }

        return false;
    }

    bool TypeInfo::IsTypeOf(const TypeInfo* typeInfo) const
    {
        if (typeInfo == nullptr)
            return false;

        const TypeInfo* current = this;
        while (current)
        {
            if (current == typeInfo || current->GetType() == typeInfo->GetType())
                return true;

            current = current->GetBaseTypeInfo();
        }

        return false;
    }

    /* Object */
    bool Object::IsInstanceOf(StringId32 type) const
    {
        return GetTypeInfo()->IsTypeOf(type);
    }

    bool Object::IsInstanceOf(const TypeInfo* typeInfo) const
    {
        return GetTypeInfo()->IsTypeOf(typeInfo);
    }

    void Object::RegisterSubsystem(Object* subsystem)
    {
        if (!subsystem)
            return;

        details::context().RegisterSubsystem(subsystem);
    }

    void Object::RegisterSubsystem(Input* subsystem)
    {
        details::context().input = subsystem;
        details::context().RegisterSubsystem(subsystem);
    }

    void Object::RegisterSubsystem(GraphicsDevice* subsystem)
    {
        details::context().graphics = subsystem;
        details::context().RegisterSubsystem(subsystem);
    }

    void Object::RemoveSubsystem(Object* subsystem)
    {
        if (!subsystem)
            return;

        details::context().RemoveSubsystem(subsystem->GetType());
    }

    void Object::RemoveSubsystem(StringId32 type)
    {
        details::context().RemoveSubsystem(type);
    }

    Object* Object::GetSubsystem(StringId32 type)
    {
        return details::context().GetSubsystem(type);
    }

    Input* Object::GetInput()
    {
        return details::context().input;
    }

    GraphicsDevice* Object::GetGraphics()
    {
        return details::context().graphics;
    }

    void Object::RegisterFactory(ObjectFactory* factory)
    {
        if (!factory)
            return;

        details::context().RegisterFactory(factory);
    }

    RefPtr<Object> Object::CreateObject(StringId32 objectType)
    {
        return details::context().CreateObject(objectType);
    }
}
