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
#include "core/Log.h"
#include "core/Assert.h"
#include "os/window.h"
#include "graphics/GPUProvider.h"

#if defined(ALIMER_GRAPHICS_VULKAN)
//#include "graphics/vulkan/VulkanGraphicsDevice.h"
#endif

#if defined(ALIMER_GRAPHICS_D3D12)
#include "graphics/d3d12/D3D12GPUProvider.h"
#endif

#if defined(ALIMER_GRAPHICS_D3D11)
#include "graphics/d3d11/D3D11GPUProvider.h"
#endif

#if defined(ALIMER_GRAPHICS_OPENGL)
//#include "graphics/opengl/GLGPUDevice.h"
#endif

using namespace std;

namespace alimer
{
    std::set<BackendType> GPUProvider::GetAvailableBackends()
    {
        static std::set<BackendType> availableProviders;

        if (availableProviders.empty())
        {
            availableProviders.insert(BackendType::Null);

#if defined(ALIMER_VULKAN)
            if (VulkanGraphicsDevice::IsAvailable())
            {
                availableProviders.insert(BackendType::Vulkan);
            }
#endif

#if defined(ALIMER_GRAPHICS_D3D12)
            if (D3D12GPUProvider::IsAvailable())
            {
                availableProviders.insert(BackendType::Direct3D12);
            }
#endif

#if defined(ALIMER_GRAPHICS_D3D11)
            if (D3D11GPUProvider::IsAvailable())
            {
                availableProviders.insert(BackendType::Direct3D11);
            }
#endif

#if defined(ALIMER_OPENGL)
            availableProviders.insert(BackendType::OpenGL);
#endif
        }

        return availableProviders;
    }

    GPUProvider::GPUProvider()
    {
    }

    unique_ptr<GPUProvider> GPUProvider::Create(BackendType preferredBackend, bool validation)
    {
        if (preferredBackend == BackendType::Count)
        {
            auto availableBackends = GetAvailableBackends();

            if (availableBackends.find(BackendType::Metal) != availableBackends.end())
                preferredBackend = BackendType::Metal;
            else if (availableBackends.find(BackendType::Direct3D12) != availableBackends.end())
                preferredBackend = BackendType::Direct3D12;
            else if (availableBackends.find(BackendType::Vulkan) != availableBackends.end())
                preferredBackend = BackendType::Vulkan;
            else if (availableBackends.find(BackendType::Direct3D11) != availableBackends.end())
                preferredBackend = BackendType::Direct3D11;
            else if (availableBackends.find(BackendType::OpenGL) != availableBackends.end())
                preferredBackend = BackendType::OpenGL;
            else
                preferredBackend = BackendType::Null;
        }

        std::unique_ptr<GPUProvider> provider = nullptr;
        switch (preferredBackend)
        {
#if defined(ALIMER_VULKAN)
        case BackendType::Vulkan:
            ALIMER_LOGINFO("Using Vulkan render driver");
            device = std::make_unique<VulkanGraphicsDevice>(desc);
            break;
#endif
#if defined(ALIMER_GRAPHICS_D3D12)
        case BackendType::Direct3D12:
            ALIMER_LOGINFO("Creating Direct3D12 GPU provider");
            provider = std::make_unique<D3D12GPUProvider>(validation);
            break;
#endif

#if defined(ALIMER_GRAPHICS_D3D11)
        case BackendType::Direct3D11:
            ALIMER_LOGINFO("Creating Direct3D11 GPU provider");
            provider = std::make_unique<D3D11GPUProvider>(validation);
            break;
#endif

#if defined(ALIMER_OPENGL)
        case BackendType::OpenGL:
            ALIMER_LOGINFO("Using OpenGL render driver");
            provider = make_shared<GLGPUDevice>(window, desc);
            break;
#endif

        case BackendType::Metal:
            break;
        default:
            /* TODO: create null backend. */
            break;
        }

        return provider;
    }
}
