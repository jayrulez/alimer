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

#include "Core/Assert.h"
#include <EASTL/string.h>
#include "DirectXMath.h"
#include "DirectXPackedVector.h"
#include "DirectXCollision.h"

namespace alimer
{
    using namespace DirectX;

    struct Float2;
    struct Float3;
    struct Float4;
    struct Float3x3;
    struct Float4x4;
    struct Quaternion;
    struct Plane;

    struct Float2 : public XMFLOAT2
    {
        Float2() noexcept : XMFLOAT2(0.f, 0.f) {}
        constexpr explicit Float2(float value) noexcept : XMFLOAT2(value, value) {}
        constexpr Float2(float ix, float iy) noexcept : XMFLOAT2(ix, iy) {}
        explicit Float2(_In_reads_(2) const float* pArray) noexcept : XMFLOAT2(pArray) {}
        Float2(FXMVECTOR v) noexcept { XMStoreFloat2(this, v); }
        Float2(const XMFLOAT2& v) noexcept : XMFLOAT2(v.x, v.y) {}
        explicit Float2(const XMVECTORF32& v) noexcept : XMFLOAT2(v.f[0], v.f[1]) {}

        Float2(const Float2&) = default;
        Float2& operator=(const Float2&) = default;

        Float2(Float2&&) = default;
        Float2& operator=(Float2&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat2(this); }

        /// Return as string.
        eastl::string ToString() const;

        // Constants
        static const Float2 Zero;
        static const Float2 One;
        static const Float2 UnitX;
        static const Float2 UnitY;
    };

    struct Float3 : public XMFLOAT3
    {
        Float3() noexcept : XMFLOAT3(0.f, 0.f, 0.f) {}
        constexpr explicit Float3(float value) noexcept : XMFLOAT3(value, value, value) {}
        constexpr Float3(float ix, float iy, float iz) noexcept : XMFLOAT3(ix, iy, iz) {}
        explicit Float3(_In_reads_(3) const float* pArray) noexcept : XMFLOAT3(pArray) {}
        Float3(FXMVECTOR v) noexcept { XMStoreFloat3(this, v); }
        Float3(const XMFLOAT3& v) noexcept : XMFLOAT3(v.x, v.y, v.z) {}
        explicit Float3(const XMVECTORF32& v) noexcept : XMFLOAT3(v.f[0], v.f[1], v.f[2]) {}

        Float3(const Float3&) = default;
        Float3& operator=(const Float3&) = default;

        Float3(Float3&&) = default;
        Float3& operator=(Float3&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat3(this); }

        /// Return as string.
        eastl::string ToString() const;

        // Constants
        static const Float3 Zero;
        static const Float3 One;
        static const Float3 UnitX;
        static const Float3 UnitY;
        static const Float3 UnitZ;
        static const Float3 Up;
        static const Float3 Down;
        static const Float3 Right;
        static const Float3 Left;
        static const Float3 Forward;
        static const Float3 Backward;
    };

    struct Float4 : public XMFLOAT4
    {
        Float4() noexcept : XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) {}
        constexpr explicit Float4(float value) noexcept : XMFLOAT4(value, value, value, value) {}
        constexpr Float4(float ix, float iy, float iz, float iw) noexcept : XMFLOAT4(ix, iy, iz, iw) {}
        explicit Float4(_In_reads_(4) const float* pArray) noexcept : XMFLOAT4(pArray) {}
        Float4(FXMVECTOR v) noexcept { XMStoreFloat4(this, v); }
        Float4(const XMFLOAT4& v) noexcept : XMFLOAT4(v.x, v.y, v.z, v.w) {}
        explicit Float4(const XMVECTORF32& v) noexcept : XMFLOAT4(v.f[0], v.f[1], v.f[2], v.f[3]) {}

        Float4(const Float4&) = default;
        Float4& operator=(const Float4&) = default;

        Float4(Float4&&) = default;
        Float4& operator=(Float4&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }

        /// Return as string.
        eastl::string ToString() const;

        // Constants
        static const Float4 Zero;
        static const Float4 One;
        static const Float4 UnitX;
        static const Float4 UnitY;
        static const Float4 UnitZ;
        static const Float4 UnitW;
    };
}
