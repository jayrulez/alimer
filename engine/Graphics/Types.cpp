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

#include "Graphics/Types.h"

namespace alimer
{
    const char* ToString(GraphicsBackendType value)
    {
        switch (value)
        {
        case GraphicsBackendType::Direct3D12:
            return "Direct3D12";
        case GraphicsBackendType::Vulkan:
            return "Vulkan";
        }

        return nullptr;
    }

    const char* ToString(TextureType value)
    {
#define CASE_STRING(ENUM_VALUE)                                                                                        \
    case TextureType::##ENUM_VALUE:                                                                                    \
        return #ENUM_VALUE;

        switch (value)
        {
            CASE_STRING(Type1D)
            CASE_STRING(Type2D)
            CASE_STRING(Type3D)
            CASE_STRING(TypeCube)
        }

#undef CASE_STRING
        return nullptr;
    }
}
