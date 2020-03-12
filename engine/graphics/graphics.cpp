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

#include "config.h"
#include "graphics/graphics.h"

#if defined(ALIMER_VULKAN)
#include "graphics/vulkan/vulkan_backend.h"
#endif

#if defined(ALIMER_D3D12)
//#include "graphics/d3d12/vulkan_backend.h"
#endif

namespace alimer
{
    namespace graphics
    {
        Device::Device(Backend::Enum backend)
            : backend{ backend }
        {
        }

        Device* Device::create(Backend::Enum preferredBackend)
        {
            if (preferredBackend == Backend::Count) {


            }

            return nullptr;
        }
    } // namespace graphics

} // namespace alimer
