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

#if defined(VGPU_DRIVER_VULKAN)

#include "vgpu_driver.h"

static bool vk_init(const vgpu_config* config) {
    return true;
}

static void vk_shutdown(void) {
}

static bool vk_frame_begin(void) {
    return true;
}

static void vk_frame_end(void) {

}

/* Driver functions */
static bool vulkan_is_supported(void) {
    return true;
}

static vgpu_context* vulkan_create_context(void) {
    static vgpu_context context = { 0 };
    ASSIGN_DRIVER(vk);
    return &context;
}

vgpu_driver vulkan_driver = {
    VGPU_BACKEND_TYPE_VULKAN,
    vulkan_is_supported,
    vulkan_create_context
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
