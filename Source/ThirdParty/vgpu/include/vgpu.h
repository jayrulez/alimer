//
// Copyright (c) 2019-2020 Amer Koleci.
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

#ifndef VGPU_H
#define VGPU_H

#if defined(VGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_API __declspec(dllexport)
#       else
#           define VGPU_API __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       define VGPU_API __attribute__((visibility("default")))
#   endif  // defined(_WIN32)
#else       // defined(VGPU_SHARED_LIBRARY)
#   define VGPU_API
#endif  // defined(VGPU_SHARED_LIBRARY)

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct vgpu_device_t* vgpu_device;
    typedef struct vgpu_context_t* vgpu_context;

    typedef enum {
        VGPU_BACKEND_TYPE_NULL,
        VGPU_BACKEND_TYPE_D3D11,
        VGPU_BACKEND_TYPE_D3D12,
        VGPU_BACKEND_TYPE_METAL,
        VGPU_BACKEND_TYPE_VULKAN,
        VGPU_BACKEND_TYPE_OPENGL,
        VGPU_BACKEND_TYPE_COUNT,
        _VGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_backend_type;

    /* Device */
    VGPU_API vgpu_device vgpu_create_device(vgpu_backend_type backend_type, bool debug);
    VGPU_API void vgpu_destroy_device(vgpu_device device);

    /* Context */
    typedef struct {
        uintptr_t handle;
    } vgpu_swapchain_info;

    typedef struct {
        uint32_t max_inflight_frames;
        uint32_t width;
        uint32_t height;

        vgpu_swapchain_info swapchain_info;
    } vgpu_context_info;

    VGPU_API vgpu_context vgpu_create_context(vgpu_device device, const vgpu_context_info* info);
    VGPU_API void vgpu_destroy_context(vgpu_device device, vgpu_context context);
    VGPU_API void vgpu_begin_frame(vgpu_device device, vgpu_context context);
    VGPU_API void vgpu_end_frame(vgpu_device device, vgpu_context context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* VGPU_H */
