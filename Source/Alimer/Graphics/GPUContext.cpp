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

#include "Core/Log.h"
#include "Graphics/GPUContext.h"

namespace alimer
{
    GPUContext::GPUContext(uint32 width, uint32 height, bool isMain_)
        : extent{width, height}
        , isMain(isMain_)
        , verticalSync(isMain)
    {

    }

    bool GPUContext::BeginFrame()
    {
        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame first.");

        if (!BeginFrameImpl()) {
            return false;
        }

        // Now the frame is active again.
        frameActive = true;
        return true;
    }

    void GPUContext::EndFrame()
    {
        ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame");

        EndFrameImpl();

        // Frame is not active anymore.
        frameActive = false;
    }

    GPUTexture* GPUContext::GetCurrentTexture() const
    {
        // Backend objects are created after the first call of BeginFrame.
        if (colorTextures.empty())
            return nullptr;

        return colorTextures[activeFrameIndex].Get();
    }

    GPUTexture* GPUContext::GetDepthStencilTexture() const
    {
        return depthStencilTexture.Get();
    }
}

