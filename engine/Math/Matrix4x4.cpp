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

#include "Math/Matrix4x4.h"

namespace Alimer
{
    const Matrix4x4 Matrix4x4::Zero = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };

    const Matrix4x4 Matrix4x4::Identity = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    Matrix4x4::Matrix4x4(const float* data) noexcept
    {
        ALIMER_ASSERT(data != nullptr);

        m[0][0] = data[0];
        m[0][1] = data[1];
        m[0][2] = data[2];
        m[0][3] = data[3];

        m[1][0] = data[4];
        m[1][1] = data[5];
        m[1][2] = data[6];
        m[1][3] = data[7];

        m[2][0] = data[8];
        m[2][1] = data[9];
        m[2][2] = data[10];
        m[2][3] = data[11];

        m[3][0] = data[12];
        m[3][1] = data[13];
        m[3][2] = data[14];
        m[3][3] = data[15];
    }

    void Matrix4x4::CreatePerspectiveFieldOfView(float fieldOfView, float aspectRatio, float zNearPlane, float zFarPlane, Matrix4x4* result)
    {
        ALIMER_ASSERT(result);
        ALIMER_ASSERT(zFarPlane != zNearPlane);

        float yScale = 1.0f / tan(fieldOfView * 0.5f);
        float xScale = yScale / aspectRatio;
        const float negFarRange = Alimer::IsInf(zFarPlane) ? -1.0f : zFarPlane / (zNearPlane - zFarPlane);

        result->m11 = xScale;
        result->m12 = 0.0f;
        result->m13 = 0.0f;
        result->m14 = 0.0f;

        result->m21 = 0.0f;
        result->m22 = yScale;
        result->m23 = 0.0f;
        result->m24 = 0.0f;

        result->m31 = 0.0f;
        result->m32 = 0.0;
        result->m33 = negFarRange;
        result->m34 = -1.0f;

        result->m41 = 0.0f;
        result->m42 = 0.0;
        result->m43 = zNearPlane * negFarRange;
        result->m44 = 0.0f;
    }

    void Matrix4x4::CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane, Matrix4x4* result)
    {
        ALIMER_ASSERT(result);
        result->m11 = 2.0f / width;
        result->m12 = 0.0f;
        result->m13 = 0.0f;
        result->m14 = 0.0f;

        result->m21 = 0.0f;
        result->m22 = 2.0f / height;
        result->m23 = 0.0f;
        result->m24 = 0.0f;

        result->m31 = 0.0f;
        result->m32 = 0.0;
        result->m33 = 1.0f / (zNearPlane - zFarPlane);
        result->m34 = 0.0f;

        result->m41 = 0.0f;
        result->m42 = 0.0;
        result->m43 = zNearPlane / (zNearPlane - zFarPlane);
        result->m44 = 1.0f;
    }

    void Matrix4x4::CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane, Matrix4x4* result)
    {
        ALIMER_ASSERT(result);
        ALIMER_ASSERT(right != left);
        ALIMER_ASSERT(top != bottom);
        ALIMER_ASSERT(zFarPlane != zNearPlane);

        result->m11 = 2.0f / (right - left);
        result->m12 = result->m13 = result->m14 = 0.0f;

        result->m22 = 2.0f / (top - bottom);
        result->m21 = result->m23 = result->m24 = 0.0f;

        result->m33 = 1.0f / (zNearPlane - zFarPlane);
        result->m31 = result->m32 = result->m34 = 0.0f;

        result->m41 = (left + right) / (left - right);
        result->m42 = (top + bottom) / (bottom - top);
        result->m43 = zNearPlane / (zNearPlane - zFarPlane);
        result->m44 = 1.0f;
    }

    std::string Matrix4x4::ToString() const
    {
        return fmt::format("{} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {}",
            m11, m12, m13, m14,
            m21, m22, m23, m24,
            m31, m32, m33, m34,
            m41, m42, m43, m44);
    }
}
