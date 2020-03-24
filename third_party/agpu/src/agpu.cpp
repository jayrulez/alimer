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

#include "agpu_internal.h"

static agpu_renderer* s_renderer = nullptr;

bool agpu_is_backend_supported(agpu_backend backend) {
    if (backend == AGPU_BACKEND_DEFAULT)
        backend = agpu_get_default_platform_backend();

    switch (backend)
    {
    case AGPU_BACKEND_NULL:
        return true;
    case AGPU_BACKEND_VULKAN:
        return agpu_vk_supported();
        //case AGPU_BACKEND_DIRECT3D11:
        //    return vgpu_d3d11_supported();
    case AGPU_BACKEND_DIRECT3D12:
        return agpu_d3d12_supported();
    case AGPU_BACKEND_OPENGL:
        return agpu_gl_supported();
    default:
        return false;
    }
}

agpu_backend agpu_get_default_platform_backend() {
#if defined(_WIN32)
    if (agpu_is_backend_supported(AGPU_BACKEND_DIRECT3D12)) {
        return AGPU_BACKEND_DIRECT3D12;
    }

    if (agpu_is_backend_supported(AGPU_BACKEND_VULKAN)) {
        return AGPU_BACKEND_VULKAN;
    }

    if (agpu_is_backend_supported(AGPU_BACKEND_DIRECT3D11)) {
        return AGPU_BACKEND_DIRECT3D11;
    }

    if (agpu_is_backend_supported(AGPU_BACKEND_OPENGL)) {
        return AGPU_BACKEND_OPENGL;
    }

    return AGPU_BACKEND_NULL;
#elif defined(__linux__) || defined(__ANDROID__)
    return AGPU_BACKEND_OPENGL;
#elif defined(__APPLE__)
    return AGPU_BACKEND_VULKAN;
#else
    return AGPU_BACKEND_OPENGL;
#endif
}

bool agpu_init(const agpu_config* config) {
    if (s_renderer != nullptr) {
        return true;
    }

    assert(config);
    agpu_backend backend = config->preferred_backend;
    if (backend == AGPU_BACKEND_DEFAULT) {
        backend = agpu_get_default_platform_backend();
    }

    agpu_renderer* renderer = nullptr;
    switch (backend)
    {
    case AGPU_BACKEND_NULL:
        break;
    case AGPU_BACKEND_VULKAN:
        renderer = agpu_create_vk_backend();
        if (!renderer) {
            //vgpuLogError("OpenGL backend is not supported");
        }
        break;

    case AGPU_BACKEND_DIRECT3D12:
        renderer = agpu_create_d3d12_backend();
        if (!renderer) {
            //vgpuLogError("OpenGL backend is not supported");
        }
        break;

    case AGPU_BACKEND_OPENGL:
        renderer = agpu_create_gl_backend();
        if (!renderer) {
            //vgpuLogError("OpenGL backend is not supported");
        }
        break;
    default:
        break;
    }

    if (renderer != nullptr) {
        s_renderer = renderer;
        return renderer->initialize(config);
    }

    //s_logCallback = descriptor->logCallback;
    return false;

    return true;
}

void agpu_shutdown(void) {
    if (s_renderer != nullptr) {
        s_renderer->shutdown();
        s_renderer = nullptr;
    }
}

void agpu_wait_idle(void) {
    s_renderer->wait_idle();
}

void agpu_begin_frame(void) {
    s_renderer->begin_frame();
}

void agpu_end_frame(void) {
    s_renderer->end_frame();
}
