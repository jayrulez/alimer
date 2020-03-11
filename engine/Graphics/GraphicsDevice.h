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

#include "Graphics/BackendTypes.h"
#include "Graphics/CommandContext.h"
#include <EASTL/unique_ptr.h>

namespace alimer
{
    class Texture;
    class SwapChain;
    class GraphicsBuffer;

    class ALIMER_API GraphicsDevice final
    {
    public:
        /// Constructor.
        GraphicsDevice(const eastl::string& applicationName,
                       GraphicsDeviceFlags flags = GraphicsDeviceFlags::None,
                       GPUPowerPreference powerPreference = GPUPowerPreference::DontCare,
                       bool headless = false);

        /// Destructor.
        ~GraphicsDevice();

        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice(GraphicsDevice&&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;

        /// Called by validation layer.
        void notify_validation_error(const char* message);

        void wait_idle();
        bool begin_frame();
        void end_frame();

    private:
        bool backend_create();
        void backend_destroy();

        /// The application name.
        eastl::string applicationName;

        /// Device flags.
        GraphicsDeviceFlags flags;

        /// GPU device power preference.
        GPUPowerPreference powerPreference;

        /// Enable headless mode.
        bool headless;

#if defined(ALIMER_VULKAN)
        VulkanDeviceFeatures features{};
        VkInstance instance{ VK_NULL_HANDLE };
        VkDebugUtilsMessengerEXT debug_messenger{ VK_NULL_HANDLE };

        VkDevice device{ VK_NULL_HANDLE };
        //VolkDeviceTable deviceTable;
#elif defined(ALIMER_D3D12)
#endif

    };
}
