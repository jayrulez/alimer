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

#include <foundation/platform.h>
#include <type_traits>

namespace alimer
{
    template <typename T>
    void SafeDelete(T*& resource)
    {
        delete resource;
        resource = nullptr;
    }
}

#define ALIMER_DISABLE_COPY(ClassType) \
    ClassType(const ClassType&) = delete; ClassType& operator=(const ClassType&) = delete; \
    ClassType(ClassType&&) = default; ClassType& operator=(ClassType&&) = default;

#define ALIMER_DISABLE_COPY_MOVE(ClassType) \
    ClassType(const ClassType&) = delete; ClassType& operator=(const ClassType&) = delete; \
    ClassType(ClassType&&) = delete; ClassType& operator=(ClassType&&) = delete;

#define ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(EnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) \
    { return EnumType(((std::underlying_type<EnumType>::type)a) | ((std::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator |= (EnumType &a, EnumType b) \
    { return a = a | b; } \
inline constexpr EnumType operator & (EnumType a, EnumType b) \
    { return EnumType(((std::underlying_type<EnumType>::type)a) & ((std::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator &= (EnumType &a, EnumType b) \
    { return a = a & b; } \
inline constexpr EnumType operator ~ (EnumType a) \
    { return EnumType(~((std::underlying_type<EnumType>::type)a)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) \
    { return EnumType(((std::underlying_type<EnumType>::type)a) ^ ((std::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator ^= (EnumType &a, EnumType b) \
    { return a = a ^ b; } \
inline constexpr bool any(EnumType a) \
    { return ((std::underlying_type<EnumType>::type)a) != 0; } 
