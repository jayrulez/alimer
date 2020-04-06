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

#include "graphics/GPUDevice.h"
#include "GLBackend.h"

namespace alimer
{
    class GLContext;

    /// Defines the OpenGL GPUDevice.
    class ALIMER_API GLGPUDevice final : public GPUDevice
    {
    public:
        /// Constructor.
        GLGPUDevice(Window* window_, const Desc& desc_);

        /// Destructor.
        ~GLGPUDevice() override;

        PFNGLCLEARPROC glClear = nullptr;
        PFNGLCLEARCOLORPROC glClearColor = nullptr;

    private:
        bool BackendInit() override;
        void BackendShutdown() override;
        void Commit() override;

        template<typename FT>
        void LoadGLFunction(FT& functionPointer, const char* functionName)
        {
            functionPointer = reinterpret_cast<FT> (context->GetGLProcAddress(functionName));
        }

        GLContext* context = nullptr;
    };
}
