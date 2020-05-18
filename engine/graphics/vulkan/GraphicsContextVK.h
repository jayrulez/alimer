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

#include "graphics/GraphicsContext.h"
#include "os/os.h"
#include "VulkanBackend.h"
#include <vector>

namespace alimer
{
    class TextureVK;
    class CommandQueueVK;

    class ALIMER_API GraphicsContextVK final : public GraphicsContext
    {
    public:
        GraphicsContextVK(GraphicsDeviceVK* device, VkSurfaceKHR surface, uint32_t width, uint32_t height);
        ~GraphicsContextVK() override;

        void Destroy();

        bool beginFrame() override;
        void endFrame() override;

        VkSwapchainKHR getHandle() const { return handle; }
        uint32_t getCurrentBackBufferIndex() const { return frameIndex; }
        TextureVK* getCurrentTexture() const { return buffers[frameIndex]; }

    private:
        bool resize(uint32_t width, uint32_t height);

        GraphicsDeviceVK* device;
        const VolkDeviceTable* table;
        CommandQueueVK* commandQueue = nullptr;

        VkSurfaceKHR    surface = VK_NULL_HANDLE;
        VkSwapchainKHR  handle = VK_NULL_HANDLE;
        VkSurfaceFormatKHR  vkFormat;

        std::vector<TextureVK*> buffers;
        uint32_t frameIndex = 0;
        uint32_t maxInflightFrames{ 3 };
    };
}
