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

#include "core/Ptr.h"
#include "core/Utils.h"
#include "graphics/GraphicsSurface.h"
#include <string>
#include <memory>

namespace alimer
{
    class GraphicsProvider;
    class GraphicsDevice;

    static constexpr uint32_t kVendorId_AMD = 0x1002;
    static constexpr uint32_t kVendorId_ARM = 0x13B5;
    static constexpr uint32_t kVendorId_ImgTec = 0x1010;
    static constexpr uint32_t kVendorId_Intel = 0x8086;
    static constexpr uint32_t kVendorId_Nvidia = 0x10DE;
    static constexpr uint32_t kVendorId_Qualcomm = 0x5143;

    /// Defines the physical graphics adapter class.
    class ALIMER_API GraphicsAdapter
    {
    public:
        /// Destructor.
        virtual ~GraphicsAdapter() = default;

        /// Get the creation provider.
        inline GraphicsProvider* GetProvider() const noexcept { return provider; }

        /// Get the backend type.
        BackendType GetBackendType() const noexcept { return backend; }

        /// Gets the adapter PCI Vendor ID (VID).
        uint32_t GetVendorId() const noexcept { return vendorId; }

        /// Gets the adapter PCI Device ID (DID).
        uint32_t GetDeviceId() const noexcept { return deviceId; }

        /// Get the type of the adapter.
        GraphicsAdapterType GetAdapterType() const noexcept { return adapterType; }

        /// Gets the name of the adapter.
        const std::string& GetName() const noexcept { return name; }

        /// Create new graphics device with given surface.
        virtual SharedPtr<GraphicsDevice> CreateDevice(GraphicsSurface* surface) = 0;

        bool IsAMD() const noexcept { return vendorId == kVendorId_AMD; }
        bool IsARM() const noexcept { return vendorId == kVendorId_ARM; }
        bool IsImgTec() const noexcept { return vendorId == kVendorId_ImgTec; }
        bool IsIntel() const noexcept { return vendorId == kVendorId_Intel; }
        bool IsNvidia() const noexcept { return vendorId == kVendorId_Nvidia; }
        bool IsQualcomm() const noexcept { return vendorId == kVendorId_Qualcomm; }

        /// Query device features.
        inline const GPUDeviceFeatures& GetFeatures() const { return features; }

        /// Query device limits.
        inline const GPUDeviceLimits& GetLimits() const { return limits; }

    protected:
        /// Constructor.
        GraphicsAdapter::GraphicsAdapter(GraphicsProvider* provider_, BackendType backend_)
            : provider(provider_)
            , backend(backend_)
        {
        }

        uint32_t vendorId = 0;
        uint32_t deviceId = 0;
        GraphicsAdapterType adapterType = GraphicsAdapterType::Unknown;
        std::string name;
        GPUDeviceFeatures features{};
        GPUDeviceLimits limits{};

    private:
        GraphicsProvider* provider;
        BackendType backend;

        ALIMER_DISABLE_COPY_MOVE(GraphicsAdapter);
    };
}
