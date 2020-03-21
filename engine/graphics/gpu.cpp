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

#include "gpu_backend.h"

namespace gpu
{
#if defined(GPU_D3D12_BACKEND)
    extern bool d3d12_supported();
    extern Device* create_d3d12_backend();
#endif
}

static gpu::Device* s_device = nullptr;

bool gpu_init(const gpu_config* config)
{
    assert(config);

    if (s_device != nullptr) {
        return true;
    }

    gpu::Device* new_device = nullptr;
    new_device = gpu::create_d3d12_backend();
    if (new_device != nullptr
        && new_device->init(config)) {
        s_device = new_device;
        return true;
    }

    return false;
}

void gpu_shutdown()
{
    if (s_device != nullptr) {
        s_device->shutdown();
        delete s_device;
        s_device = nullptr;
    }
}

