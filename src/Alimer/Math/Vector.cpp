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

#include "Math/Vector.h"
#include "Core/String.h"
#include "Diagnostics/Assert.h"

namespace Alimer
{
    /* Vector2 */
    const Vector2 Vector2::Zero = { 0.0f, 0.0f };
    const Vector2 Vector2::One = { 1.0f, 1.0f };
    const Vector2 Vector2::UnitX = { 1.0f, 0.0f };
    const Vector2 Vector2::UnitY = { 0.0f, 1.0f };

    float Vector2::Cross(const Vector2& lhs, const Vector2& rhs)
    {
        return lhs.x * rhs.y - lhs.y * rhs.x;
    }

    void Vector2::Normalize(Vector2* result) const noexcept
    {
        ALIMER_ASSERT(result);

        if (result != this)
        {
            result->x = x;
            result->y = y;
        }

        float lenSquared = LengthSquared();
        if (!Alimer::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
        {
            float invLen = 1.0f / sqrtf(lenSquared);
            result->x *= invLen;
            result->y *= invLen;
        }
    }

    eastl::string Vector2::ToString() const
    {
        char tempBuffer[CONVERSION_BUFFER_LENGTH];
        sprintf(tempBuffer, "%g %g", x, y);
        return eastl::string(tempBuffer);
    }

    /* Vector4 */
    eastl::string Vector4::ToString() const
    {
        char tempBuffer[CONVERSION_BUFFER_LENGTH];
        sprintf(tempBuffer, "%g %g %g %g", x, y, z, w);
        return eastl::string(tempBuffer);
    }
}
