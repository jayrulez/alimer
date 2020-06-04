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

#pragma once

#include "graphics/Types.h"
#include "Core/Ptr.h"

namespace alimer
{
    class CommandBuffer;
    class GraphicsDevice;

    /// Defines a queue that organizes command buffers to be executed by a GPU.
    class CommandQueue : public RefCounted
    {
    public:
        CommandQueue(GraphicsDevice* device, CommandQueueType queueType);
        virtual ~CommandQueue() = default;

        virtual void WaitIdle() = 0;

        // Get an available command buffer from the command queue.
        virtual RefPtr<CommandBuffer> GetCommandBuffer() = 0;

        /// Returns the device from which this command queue was created.
        GraphicsDevice* GetDevice() const;
        CommandQueueType GetQueueType() const { return queueType; }

    protected:
        GraphicsDevice* device;
        CommandQueueType queueType;
    };
}
