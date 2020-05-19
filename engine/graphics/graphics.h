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

#include <foundation/platform.h>

namespace alimer
{
    namespace graphics
    {
        static constexpr uint32_t kInvalidHandle = 0xFFffFFff;
        struct ContextHandle { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };
        static constexpr ContextHandle kInvalidContext = { kInvalidHandle };

        enum class LogLevel : uint32_t {
            Error,
            Warn,
            Info,
            Debug
        };

        typedef void (*LogCallback)(void* userData, const char* message, LogLevel level);
        typedef void* (*GetProcAddressCallback)(const char* functionName);

        struct Configuration {
            bool debug;
            LogCallback logCallback;
            GetProcAddressCallback getProcAddress;
            void* userData;
        };

        struct ContextInfo {
            uintptr_t handle;
            uint32_t width;
            uint32_t height;
        };

        bool Initialize(const Configuration& config);
        void Shutdown();

        ContextHandle CreateContext(const ContextInfo& info);
        void DestroyContext(ContextHandle handle);
        bool ResizeContext(ContextHandle handle, uint32_t width, uint32_t height);
        bool BeginFrame(ContextHandle handle);
        void EndFrame(ContextHandle handle);
    }
}
