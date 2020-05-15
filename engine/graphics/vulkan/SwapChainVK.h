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

#pragma once

#include "Containers/Array.h"
#include "graphics/ISwapChain.h"
#include "os/os.h"
#include "VulkanBackend.h"

namespace alimer
{
    class TextureVK;
    class CommandQueueVK;

    class ALIMER_API SwapChainVK final : public ISwapChain
    {
    public:
        SwapChainVK(GraphicsDeviceVK* device_);
        ~SwapChainVK() override;

        bool Init(window_t* window, ICommandQueue* commandQueue, const SwapChainDesc* pDesc);
        void Destroy();

        ALIMER_FORCEINLINE const SwapChainDesc& GetDesc() const override { return desc; }
        VkSwapchainKHR GetHandle() const { return handle; }
        uint32_t GetCurrentBackBufferIndex() const { return backBufferIndex; }
        TextureVK* GetCurrentTexture() const { return buffers[backBufferIndex]; }

        IGraphicsDevice* GetDevice() const override;
        ICommandQueue* GetCommandQueue() const override;
        ITexture* GetNextTexture() override;

    private:
        bool InitSwapChain(uint32_t width, uint32_t height);

        GraphicsDeviceVK* device;
        CommandQueueVK* commandQueue = nullptr;

        VkSurfaceKHR    surface = VK_NULL_HANDLE;
        VkSwapchainKHR  handle = VK_NULL_HANDLE;
        VkSurfaceFormatKHR  vkFormat;

        SwapChainDesc desc;
        uint32_t backBufferIndex = 0;
        uint32_t semaphoreIndex = 0;
        Vector<TextureVK*> buffers;
    };
}
