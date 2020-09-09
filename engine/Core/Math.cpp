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

#include "Core/Math.h"

namespace Alimer
{
    /* Float2 */
    const Float2 Float2::Zero = { 0.0f, 0.0f };
    const Float2 Float2::One = { 1.0f, 1.0f };
    const Float2 Float2::UnitX = { 1.0f, 0.0f };
    const Float2 Float2::UnitY = { 0.0f, 1.0f };

    std::string Float2::ToString() const
    {
        char tempBuffer[CONVERSION_BUFFER_LENGTH];
        sprintf(tempBuffer, "%g %g", x, y);
        return std::string(tempBuffer);
    }

    /* Int2 */
    const Int2 Int2::Zero = { 0, 0 };
    const Int2 Int2::One = { 1, 1 };
    const Int2 Int2::UnitX = { 1, 0 };
    const Int2 Int2::UnitY = { 0, 1 };

    /* UInt2 */
    const UInt2 UInt2::Zero = { 0, 0 };
    const UInt2 UInt2::One = { 1, 1 };
    const UInt2 UInt2::UnitX = { 1, 0 };
    const UInt2 UInt2::UnitY = { 0, 1 };

    /* Float3 */
    const Float3 Float3::Zero = { 0.0f, 0.0f, 0.0f };
    const Float3 Float3::One = { 1.0f, 1.0f, 1.0f };
    const Float3 Float3::UnitX = { 1.0f, 0.0f, 0.0f };
    const Float3 Float3::UnitY = { 0.0f, 1.0f, 0.0f };
    const Float3 Float3::UnitZ = { 0.0f, 0.0f, 1.0f };
    const Float3 Float3::Up = { 0.0f, 1.0f, 0.0f };
    const Float3 Float3::Down = { 0.0f, -1.0f, 0.0f };
    const Float3 Float3::Right = { 1.0f, 0.0f, 0.f };
    const Float3 Float3::Left = { -1.0f, 0.0f, 0.0f };
    const Float3 Float3::Forward = { 0.0f, 0.0f, -1.0f };
    const Float3 Float3::Backward = { 0.0f, 0.0f, 1.0f };

    std::string Float3::ToString() const
    {
        char tempBuffer[CONVERSION_BUFFER_LENGTH];
        sprintf(tempBuffer, "%g %g %g", x, y, z);
        return std::string(tempBuffer);
    }

    /* Float4 */
    const Float4 Float4::Zero = { 0.f, 0.f, 0.f, 0.f };
    const Float4 Float4::One = { 1.f, 1.f, 1.f, 1.f };
    const Float4 Float4::UnitX = { 1.f, 0.f, 0.f, 0.f };
    const Float4 Float4::UnitY = { 0.f, 1.f, 0.f, 0.f };
    const Float4 Float4::UnitZ = { 0.f, 0.f, 1.f, 0.f };
    const Float4 Float4::UnitW = { 0.f, 0.f, 0.f, 1.f };

    std::string Float4::ToString() const
    {
        char tempBuffer[CONVERSION_BUFFER_LENGTH];
        sprintf(tempBuffer, "%g %g %g %g", x, y, z, w);
        return std::string(tempBuffer);
    }
}
