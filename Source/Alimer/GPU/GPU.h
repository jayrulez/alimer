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

#include "Core/Platform.h"
#include "Graphics/Types.h"
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

namespace alimer
{
    enum class PowerPreference
    {
        Default,
        HighPerformance,
        LowPower
    };

    enum class GPUKnownVendorId : uint32_t {
        None = 0,
        AMD = 0x1002,
        Intel = 0x8086,
        Nvidia = 0x10DE,
        ARM = 0x13B5,
        ImgTec = 0x1010,
        Qualcomm = 0x5143
    };

    enum class GPUAdapterType : uint32_t {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    class GPUAdapter
    {
    public:
        GPUAdapter(RendererType backendType);
        virtual ~GPUAdapter() = default;

    protected:
        RendererType backendType;
        eastl::string name;
        uint32_t vendorId = 0;
        uint32_t deviceId = 0;
        GPUAdapterType adapterType = GPUAdapterType::Unknown;
    };

    class GPU final 
    {
    public:
        static void EnableBackendValidation(bool enableBackendValidation);
        static bool IsBackendValidationEnabled();
        static void EnableGPUBasedBackendValidation(bool enableGPUBasedBackendValidation);
        static bool IsGPUBasedBackendValidationEnabled();

        static eastl::unique_ptr<GPUAdapter> RequestAdapter(PowerPreference powerPreference, RendererType backendType = RendererType::Count);
    };
}
