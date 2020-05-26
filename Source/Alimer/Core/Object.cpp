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

namespace Alimer
{
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
}
