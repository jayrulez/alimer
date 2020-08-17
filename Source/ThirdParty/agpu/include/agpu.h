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

#ifndef AGPU_H
#define AGPU_H

// On Windows, use the stdcall convention, pn other platforms, use the default calling convention
#if defined(_WIN32)
#   define AGPU_API_CALL __stdcall
#else
#   define AGPU_API_CALL
#endif

#if defined(AGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(AGPU_IMPLEMENTATION)
#           define AGPU_API __declspec(dllexport)
#       else
#           define AGPU_API __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       define AGPU_API __attribute__((visibility("default")))
#   endif  // defined(_WIN32)
#else       // defined(AGPU_SHARED_LIBRARY)
#   define AGPU_API
#endif  // defined(AGPU_SHARED_LIBRARY)

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct AGPUDevice AGPUDevice;
    typedef struct AGPUTexture AGPUTexture;

    typedef enum AGPUBackendType {
        AGPUBackendType_Null = 0x00000000,
        AGPUBackendType_D3D11 = 0x00000001,
        AGPUBackendType_D3D12 = 0x00000002,
        AGPUBackendType_Metal = 0x00000003,
        AGPUBackendType_Vulkan = 0x00000004,
        AGPUBackendType_OpenGL = 0x00000005,
        AGPUBackendType_OpenGLES = 0x00000006,
        AGPUBackendType_Force32 = 0x7FFFFFFF
    } AGPUBackendType;

    /// Defines pixel format.
    typedef enum AGPUPixelFormat {
        AGPUPixelFormat_Undefined = 0,
        // 8-bit pixel formats
        AGPUPixelFormat_R8UNorm,
        AGPUPixelFormat_R8SNorm,
        AGPUPixelFormat_R8UInt,
        AGPUPixelFormat_R8SInt,

        // 16-bit pixel formats
        AGPUPixelFormat_R16UNorm,
        AGPUPixelFormat_R16SNorm,
        AGPUPixelFormat_R16UInt,
        AGPUPixelFormat_R16SInt,
        AGPUPixelFormat_R16Float,
        AGPUPixelFormat_RG8UNorm,
        AGPUPixelFormat_RG8SNorm,
        AGPUPixelFormat_RG8UInt,
        AGPUPixelFormat_RG8SInt,

        // 32-bit pixel formats
        AGPUPixelFormat_R32UInt,
        AGPUPixelFormat_R32SInt,
        AGPUPixelFormat_R32Float,
        AGPUPixelFormat_RG16UInt,
        AGPUPixelFormat_RG16SInt,
        AGPUPixelFormat_RG16Float,

        AGPUPixelFormat_RGBA8Unorm,
        AGPUPixelFormat_RGBA8UnormSrgb,
        AGPUPixelFormat_RGBA8SNorm,
        AGPUPixelFormat_RGBA8UInt,
        AGPUPixelFormat_RGBA8SInt,
        AGPUPixelFormat_BGRA8Unorm,
        AGPUPixelFormat_BGRA8UnormSrgb,

        // Packed 32-Bit Pixel formats
        AGPUPixelFormat_RGB10A2Unorm,
        AGPUPixelFormat_RG11B10Float,

        // 64-Bit Pixel Formats
        AGPUPixelFormat_RG32UInt,
        AGPUPixelFormat_RG32SInt,
        AGPUPixelFormat_RG32Float,
        AGPUPixelFormat_RGBA16UInt,
        AGPUPixelFormat_RGBA16SInt,
        AGPUPixelFormat_RGBA16Float,

        // 128-Bit Pixel Formats
        AGPUPixelFormat_RGBA32UInt,
        AGPUPixelFormat_RGBA32SInt,
        AGPUPixelFormat_RGBA32Float,

        // Depth-stencil
        AGPUPixelFormat_Depth16Unorm,
        AGPUPixelFormat_Depth32Float,
        AGPUPixelFormat_Depth24Unorm_Stencil8,
        AGPUPixelFormat_Depth32Float_Stencil8,

        // Compressed BC formats
        AGPUPixelFormat_BC1RGBAUnorm,
        AGPUPixelFormat_BC1RGBAUnormSrgb,
        AGPUPixelFormat_BC2RGBAUnorm,
        AGPUPixelFormat_BC2RGBAUnormSrgb,
        AGPUPixelFormat_BC3RGBAUnorm,
        AGPUPixelFormat_BC3RGBAUnormSrgb,
        AGPUPixelFormat_BC4RUnorm,
        AGPUPixelFormat_BC4RSnorm,
        AGPUPixelFormat_BC5RGUnorm,
        AGPUPixelFormat_BC5RGSnorm,
        AGPUPixelFormat_BC6HRGBUfloat,
        AGPUPixelFormat_BC6HRGBSfloat,
        AGPUPixelFormat_BC7RGBAUnorm,
        AGPUPixelFormat_BC7RGBAUnormSrgb,

        AGPUPixelFormat_Force32 = 0x7FFFFFFF
    } AGPUPixelFormat;

    AGPU_API AGPUDevice* agpuCreateDevice(AGPUBackendType backendType);
    AGPU_API void agpuDestroyDevice(AGPUDevice* device);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AGPU_H */
