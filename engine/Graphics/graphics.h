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

#pragma once

#include "graphics/PixelFormat.h"

namespace alimer
{
    namespace graphics
    {
#ifdef _DEBUG
#define DEFAULT_ENABLE_VALIDATION true
#else
#define DEFAULT_ENABLE_VALIDATION false
#endif

        typedef struct DeviceImpl* Device;

        static constexpr uint32_t kInvalidHandle = 0u;
        struct BufferHandle { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };
        const BufferHandle kInvalidBuffer = { kInvalidHandle };

        enum class BackendType : uint32_t {
            Null,
            D3D11,
            OpenGL
        };

        // Parameters structure for CreateDevice
        struct DeviceParams
        {
            bool validation = DEFAULT_ENABLE_VALIDATION;
            uint32_t width;
            uint32_t height;
            bool fullscreen;
            bool verticalSync;
            PixelFormat colorFormat = PixelFormat::BGRA8UNorm;
            PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
            void* windowHandle = nullptr;
        };

        Device CreateDevice(const DeviceParams& params);
        void DestroyDevice(Device device);
        void BeginFrame(Device device);
        void PresentFrame(Device device);
    }
}
