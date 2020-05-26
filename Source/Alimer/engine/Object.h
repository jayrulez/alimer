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

#include "engine/Ptr.h"
#include "engine/StringId.h"
#include <memory>

namespace Alimer
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
        const std::string& GetTypeName() const { return typeName; }
        /// Return base type info.
        const TypeInfo* GetBaseTypeInfo() const { return baseTypeInfo; }

    private:
        /// Type.
        StringId32 type;
        /// Type name.
        std::string typeName;
        /// Base class type info.
        const TypeInfo* baseTypeInfo;
    };

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
        virtual const std::string& GetTypeName() const = 0;
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
    };
}

#define ALIMER_OBJECT(typeName, baseTypeName) \
    public: \
        using ClassName = typeName; \
        using BaseClassName = baseTypeName; \
        virtual Alimer::StringId32 GetType() const override { return GetTypeInfoStatic()->GetType(); } \
        virtual const std::string& GetTypeName() const override { return GetTypeInfoStatic()->GetTypeName(); } \
        virtual const Alimer::TypeInfo* GetTypeInfo() const override { return GetTypeInfoStatic(); } \
        static Alimer::StringId32 GetTypeStatic() { return GetTypeInfoStatic()->GetType(); } \
        static const std::string& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); } \
        static const Alimer::TypeInfo* GetTypeInfoStatic() { static const Alimer::TypeInfo typeInfoStatic(#typeName, BaseClassName::GetTypeInfoStatic()); return &typeInfoStatic; } \
