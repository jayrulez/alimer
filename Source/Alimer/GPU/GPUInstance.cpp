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
#include "GPU/GPUInstance.h"
#include "Core/Log.h"

#if defined(ALIMER_D3D12)
#   include "GPU/D3D12/D3D12GPUInstance.h"
#endif

#if defined(ALIMER_VULKAN)
#   include "GPU/Vulkan/VulkanGPUInstance.h"
#endif

namespace alimer
{
    RefPtr<GPUInstance> GPUInstance::Create(GPUBackendType preferredBackend)
    {
        if (preferredBackend == GPUBackendType::Count)
        {
#if defined(ALIMER_D3D12)
            if (D3D12GPUInstance::IsAvailable())
            {
                preferredBackend = GPUBackendType::Direct3D12;
            }
#endif

#if defined(ALIMER_VULKAN)
            if (preferredBackend == GPUBackendType::Count
                && VulkanGPUInstance::IsAvailable())
            {
                preferredBackend = GPUBackendType::Vulkan;
            }
#endif
        }

        switch (preferredBackend)
        {
        case GPUBackendType::Null:
            break;
#if defined(ALIMER_VULKAN)
        case GPUBackendType::Vulkan:
            if (VulkanGPUInstance::IsAvailable())
            {
                return MakeRefPtr<VulkanGPUInstance>("Alimer");
            }
            break;
#endif

#if defined(ALIMER_METAL)
        case GPUBackendType::Metal:
            break;
#endif

#if defined(ALIMER_D3D12)
        case GPUBackendType::Direct3D12:
            if (D3D12GPUInstance::IsAvailable())
            {
                return MakeRefPtr<D3D12GPUInstance>();
            }
            break;
#endif

        default:
            break;
        }

        return nullptr;
    }
}
