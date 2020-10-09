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

#include "AlimerConfig.h"
#include "Core/Log.h"
#include "Math/MathHelper.h"
#include "Graphics/GraphicsDevice.h"

#if defined(ALIMER_D3D11)
#include "Graphics/D3D11/D3D11GraphicsDevice.h"
#endif
#if defined(ALIMER_D3D12)
#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif
#if defined(ALIMER_VULKAN)
//#include "Graphics/Vulkan/VulkanGraphicsDevice.h"
#endif

namespace Alimer
{
    GraphicsDevice* GraphicsDevice::Instance = nullptr;

    GraphicsDevice::GraphicsDevice(const PresentationParameters& presentationParameters)
        : backbufferWidth(presentationParameters.backBufferWidth)
        , backbufferHeight(presentationParameters.backBufferHeight)
    {

    }

    std::set<GraphicsBackendType> GraphicsDevice::GetAvailableBackends()
    {
        static std::set<GraphicsBackendType> backends;

        if (backends.empty())
        {
            backends.insert(GraphicsBackendType::Null);

#if defined(ALIMER_D3D11)
            if(D3D11GraphicsDevice::IsAvailable())
                backends.insert(GraphicsBackendType::Direct3D11);
#endif

#if defined(ALIMER_D3D12)
            if (D3D12GraphicsDevice::IsAvailable())
                backends.insert(GraphicsBackendType::Direct3D12);
#endif

#if defined(ALIMER_VULKAN)
            //if (VulkanGraphicsDevice::IsAvailable())
            //    backends.insert(GraphicsBackendType::Vulkan);
#endif
        }

        return backends;
    }

    bool GraphicsDevice::Initialize(const PresentationParameters& presentationParameters, GraphicsBackendType preferredBackendType, GraphicsDeviceFlags flags)
    {
        if (Instance != nullptr)
        {
            return true;
        }

        GraphicsBackendType backendType = preferredBackendType;
        if (backendType == GraphicsBackendType::Count)
        {
            const auto availableBackends = GetAvailableBackends();
            if (availableBackends.find(GraphicsBackendType::Metal) != availableBackends.end())
                backendType = GraphicsBackendType::Metal;
            else if (availableBackends.find(GraphicsBackendType::Direct3D12) != availableBackends.end())
                backendType = GraphicsBackendType::Direct3D12;
            else if (availableBackends.find(GraphicsBackendType::Direct3D11) != availableBackends.end())
                backendType = GraphicsBackendType::Direct3D11;
            else if (availableBackends.find(GraphicsBackendType::Vulkan) != availableBackends.end())
                backendType = GraphicsBackendType::Vulkan;
            else
                backendType = GraphicsBackendType::Null;
        }

        switch (backendType)
        {
#if defined(ALIMER_D3D12)
        case GraphicsBackendType::Direct3D12:
            LOGI("Using Direct3D 12 render driver");
            Instance = new D3D12GraphicsDevice(presentationParameters, flags);
            break;
#endif

#if defined(ALIMER_D3D11)
        case GraphicsBackendType::Direct3D11:
            LOGI("Using Direct3D 11 render driver");
            Instance = new D3D11GraphicsDevice(presentationParameters, flags);
            break;
#endif

        case Alimer::GraphicsBackendType::Vulkan:
            break;
        case Alimer::GraphicsBackendType::Metal:
            break;
        default:
            // TODO: Create null
            break;
        }

        return Instance != nullptr;
    }

    /*void GraphicsDevice::AddGraphicsResource(GraphicsResource* resource)
    {
        ALIMER_ASSERT(resource);

        std::lock_guard<std::mutex> LockGuard(gpuObjectMutex);
        gpuObjects.push_back(resource);
    }

    void GraphicsDevice::RemoveGraphicsResource(GraphicsResource* resource)
    {
        ALIMER_ASSERT(resource);
        std::lock_guard<std::mutex> LockGuard(gpuObjectMutex);

        auto it = std::find(gpuObjects.begin(), gpuObjects.end(), resource);
        if (it != gpuObjects.end())
        {
            gpuObjects.erase(it);
        }
    }*/
}

