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

#include "Core/Ptr.h"
#include "Core/StringId.h"

namespace alimer
{
    /// Type info.
    class ALIMER_API TypeInfo final
    {
    public:
        /// Construct.
        TypeInfo(const char* typeName_, const TypeInfo* baseTypeInfo_);
        /// Destruct.
        ~TypeInfo() = default;

        /// Check current type is type of specified type.
        bool IsTypeOf(StringId32 type) const;
        /// Check current type is type of specified type.
        bool IsTypeOf(const TypeInfo* typeInfo) const;
        /// Check current type is type of specified class type.
        template<typename T> bool IsTypeOf() const { return IsTypeOf(T::GetTypeInfoStatic()); }

        /// Return type.
        StringId32 GetType() const { return type; }
        /// Return type name.
        const eastl::string& GetTypeName() const { return typeName; }
        /// Return base type info.
        const TypeInfo* GetBaseTypeInfo() const { return baseTypeInfo; }

    private:
        /// Type.
        StringId32 type;
        /// Type name.
        eastl::string typeName;
        /// Base class type info.
        const TypeInfo* baseTypeInfo;
    };

    class ObjectFactory;
    template <class T> class ObjectFactoryImpl;

    class Input;
    class GraphicsDevice;

    /// Base class for objects with type identification, subsystem access
    class ALIMER_API Object : public RefCounted
    {
    public:
        /// Constructor.
        Object() = default;
        /// Destructor.
        virtual ~Object() = default;

        /// Return type hash.
        virtual StringId32 GetType() const = 0;
        /// Return type name.
        virtual const eastl::string& GetTypeName() const = 0;
        /// Return type info.
        virtual const TypeInfo* GetTypeInfo() const = 0;

        /// Return type info static.
        static const TypeInfo* GetTypeInfoStatic() { return nullptr; }
        /// Check current instance is type of specified type.
        bool IsInstanceOf(StringId32 type) const;
        /// Check current instance is type of specified type.
        bool IsInstanceOf(const TypeInfo* typeInfo) const;
        /// Check current instance is type of specified class.
        template<typename T> bool IsInstanceOf() const { return IsInstanceOf(T::GetTypeInfoStatic()); }
        /// Cast the object to specified most derived class.
        template<typename T> T* Cast() { return IsInstanceOf<T>() ? static_cast<T*>(this) : nullptr; }
        /// Cast the object to specified most derived class.
        template<typename T> const T* Cast() const { return IsInstanceOf<T>() ? static_cast<const T*>(this) : nullptr; }

        /// Register an object as a subsystem that can be accessed globally. Note that the subsystems container does not own the objects.
        static void RegisterSubsystem(Object* subsystem);
        static void RegisterSubsystem(Input* subsystem);
        static void RegisterSubsystem(GraphicsDevice* subsystem);

        /// Remove a subsystem by object pointer.
        static void RemoveSubsystem(Object* subsystem);
        /// Remove a subsystem by type.
        static void RemoveSubsystem(StringId32 type);
        /// Template version of removing a subsystem.
        template <class T> static void RemoveSubsystem() { RemoveSubsystem(T::GetTypeStatic()); }
        /// Return a subsystem by type, or null if not registered.
        static Object* GetSubsystem(StringId32 type);
        /// Register an object factory.
        static void RegisterFactory(ObjectFactory* factory);
        /// Create an object by type hash. Return pointer to it or null if no factory found.
        static SharedPtr<Object> CreateObject(StringId32 objectType);

        /// Return a subsystem, template version.
        template <class T> static T* GetSubsystem() { return static_cast<T*>(GetSubsystem(T::GetTypeStatic())); }
        template <> static Input* GetSubsystem<Input>() { return GetInput(); }
        template <> static GraphicsDevice* GetSubsystem<GraphicsDevice>() { return GetGraphics(); }

        /// Register an object factory, template version.
        template <class T> static void RegisterFactory() { RegisterFactory(new ObjectFactoryImpl<T>()); }
        /// Create and return an object through a factory, template version.
        template <class T> static inline SharedPtr<T> CreateObject() { return StaticCast<T>(CreateObject(T::GetTypeStatic())); }

        /// Return input subsystem.
        static Input* GetInput();

        /// Return graphics subsystem.
        static GraphicsDevice* GetGraphics();
    };

    /// Base class for object factories.
    class ALIMER_API ObjectFactory
    {
    public:
        /// Destruct.
        virtual ~ObjectFactory() = default;

        /// /// Create an object.
        virtual SharedPtr<Object> Create() = 0;

        /// Return type info of objects created by this factory.
        const TypeInfo* GetTypeInfo() const { return typeInfo; }

        /// Return type hash of objects created by this factory.
        StringId32 GetType() const { return typeInfo->GetType(); }

        /// Return type name of objects created by this factory.
        const eastl::string& GetTypeName() const { return typeInfo->GetTypeName(); }

    protected:
        /// Type info.
        const TypeInfo* typeInfo = nullptr;
    };

    /// Template implementation of the object factory.
    template <class T> class ObjectFactoryImpl : public ObjectFactory
    {
    public:
        /// Construct.
        ObjectFactoryImpl()
        {
            typeInfo = T::GetTypeInfoStatic();
        }

        /// Create an object of the specific type.
        SharedPtr<Object> Create() override { return SharedPtr<Object>(new T()); }
    };
}

#define ALIMER_OBJECT(typeName, baseTypeName) \
    public: \
        using ClassName = typeName; \
        using Parent = baseTypeName; \
        virtual alimer::StringId32 GetType() const override { return GetTypeInfoStatic()->GetType(); } \
        virtual const eastl::string& GetTypeName() const override { return GetTypeInfoStatic()->GetTypeName(); } \
        virtual const alimer::TypeInfo* GetTypeInfo() const override { return GetTypeInfoStatic(); } \
        static alimer::StringId32 GetTypeStatic() { return GetTypeInfoStatic()->GetType(); } \
        static const eastl::string& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); } \
        static const alimer::TypeInfo* GetTypeInfoStatic() { static const alimer::TypeInfo typeInfoStatic(#typeName, Parent::GetTypeInfoStatic()); return &typeInfoStatic; } \
