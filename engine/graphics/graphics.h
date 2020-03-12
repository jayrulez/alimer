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

#include "Graphics/CommandContext.h"
#include <EASTL/unique_ptr.h>

namespace alimer
{
    class Texture;
    class SwapChain;
    class GraphicsBuffer;

    namespace graphics
    {
        struct BufferHandle { uint32_t id; };

        namespace Backend
        {
            /// Enum describing the Device backend.
            enum Enum {
                /// Null backend.
                Null,
                /// Vulkan backend.
                Vulkan,
                /// Direct3D 12 backend.
                Direct3D12,
                /// Metal backend.
                Metal,
                /// Default best platform supported backend.
                Count
            };

            static const char* s_value_names[] = {
                "Null", "Vulkan", "Direct3D12", "Metal", "Count"
            };

            static const char* ToString(Enum e) {
                return s_value_names[(int)e];
            }

        } // namespace GraphicsBackend


        class ALIMER_API Device
        {
        public:
            /// Constructor.
            Device(Backend::Enum backend);

            /// Destructor.
            virtual ~Device() = default;

            Device(const Device&) = delete;
            Device(Device&&) = delete;
            Device& operator=(const Device&) = delete;
            Device& operator=(Device&&) = delete;

            static Device* create(Backend::Enum preferredBackend = Backend::Count);

            /// Called by validation layer.
            //void notify_validation_error(const char* message);

            //void wait_idle();
            //bool begin_frame();
            //void end_frame();

        private:
            Backend::Enum backend;

            /// The application name.
            eastl::string applicationName;

            /// Device flags.
            GraphicsDeviceFlags flags;

            /// GPU device power preference.
            GPUPowerPreference powerPreference;

            /// Enable headless mode.
            bool headless;
        };
    } // namespace graphics
} // namespace alimer
