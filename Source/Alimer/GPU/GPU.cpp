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

#include "config.h"
#include "GPU/GPU.h"

#if defined(ALIMER_GPU_D3D12)
#include "GPU/D3D12/D3D12GPU.h"
#endif

namespace alimer
{
    static bool s_EnableBackendValidation = false;
    static bool s_EnableGPUBasedBackendValidation = false;

    /* GPU */
    GPUDevice* GPU::Instance = nullptr;

    void GPU::EnableBackendValidation(bool enableBackendValidation)
    {
        s_EnableBackendValidation = enableBackendValidation;
    }

    bool GPU::IsBackendValidationEnabled()
    {
        return s_EnableBackendValidation;
    }

    void GPU::EnableGPUBasedBackendValidation(bool enableGPUBasedBackendValidation)
    {
        s_EnableGPUBasedBackendValidation = enableGPUBasedBackendValidation;
    }

    bool GPU::IsGPUBasedBackendValidationEnabled()
    {
        return s_EnableGPUBasedBackendValidation;
    }

    GPUDevice* GPU::CreateDevice(WindowHandle windowHandle, const GPUDevice::Desc& desc)
    {
        if (Instance != nullptr)
            return Instance;

#if defined(ALIMER_GPU_D3D12)
        Instance = D3D12GPU::Get()->CreateDevice(windowHandle, desc);
#endif

        return Instance;
    }
}
