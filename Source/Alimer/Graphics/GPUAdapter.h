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

#include "Core/String.h"
#include "Graphics/Types.h"

namespace Alimer
{
    static constexpr uint32 KnownVendorId_AMD = 0x1002;
    static constexpr uint32 KnownVendorId_Intel = 0x8086;
    static constexpr uint32 KnownVendorId_Nvidia = 0x10DE;
    static constexpr uint32 KnownVendorId_Microsoft = 0x1414;
    static constexpr uint32 KnownVendorId_ARM = 0x13B5;
    static constexpr uint32 KnownVendorId_ImgTec = 0x1010;
    static constexpr uint32 KnownVendorId_Qualcomm = 0x5143;

    /// Identifies the physical GPU adapter.
    class ALIMER_API GPUAdapter 
    {
    public:
        /// Destructor.
        virtual ~GPUAdapter() = default;

        /// Gets the GPU device identifier.
        virtual uint32 GetDeviceId() const = 0;

        /// Gets the GPU vendor identifier.
        virtual uint32 GetVendorId() const = 0;

        /// Gets the adapter name.
        virtual String GetName() const = 0;

        /// Gets the adapter type.
        virtual GPUAdapterType GetAdapterType() const = 0;

        // Returns true if adapter's vendor is AMD.
        ALIMER_FORCE_INLINE bool IsAMD() const
        {
            return GetVendorId() == KnownVendorId_AMD;
        }

        // Returns true if adapter's vendor is Intel.
        ALIMER_FORCE_INLINE bool IsIntel() const
        {
            return GetVendorId() == KnownVendorId_Intel;
        }

        // Returns true if adapter's vendor is Nvidia.
        ALIMER_FORCE_INLINE bool IsNvidia() const
        {
            return GetVendorId() == KnownVendorId_Nvidia;
        }

        // Returns true if adapter's vendor is Microsoft.
        ALIMER_FORCE_INLINE bool IsMicrosoft() const
        {
            return GetVendorId() == KnownVendorId_Microsoft;
        }
    };
}

