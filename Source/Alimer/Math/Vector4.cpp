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

#include "Math/Vector4.h"

namespace alimer
{
    const Vector4 Vector4::Zero = { 0.f, 0.f, 0.f, 0.f };
    const Vector4 Vector4::One = { 1.f, 1.f, 1.f, 1.f };
    const Vector4 Vector4::UnitX = { 1.f, 0.f, 0.f, 0.f };
    const Vector4 Vector4::UnitY = { 0.f, 1.f, 0.f, 0.f };
    const Vector4 Vector4::UnitZ = { 0.f, 0.f, 1.f, 0.f };
    const Vector4 Vector4::UnitW = { 0.f, 0.f, 0.f, 1.f };

    String Vector4::ToString() const
    {
        char tempBuffer[CONVERSION_BUFFER_LENGTH];
        sprintf(tempBuffer, "%g %g %g %g", x, y, z, w);
        return String(tempBuffer);
    }
}
