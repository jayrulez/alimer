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

#include "graphics/Types.h"

namespace alimer
{
    class GPUProvider;
    class GPUDevice;

    /// Defines the physical GPU adapter class.
    class ALIMER_API GPUAdapter
    {
    public:
        /// Destructor.
        virtual ~GPUAdapter() = default;

        /// Get the creation provider.
        inline GPUProvider* GetProvider() const noexcept { return _provider; }

        /// Get the backend type.
        BackendType GetBackendType() const noexcept { return _backend; }

        /// Gets the adapter PCI Vendor ID (VID).
        uint32_t GetVendorId() const noexcept { return _vendorId; }

        /// Gets the adapter PCI Device ID (DID).
        uint32_t GetDeviceId() const noexcept { return _deviceId; }

        /// Get the type of the adapter.
        GraphicsAdapterType GetAdapterType() const noexcept { return _adapterType; }

        /// Gets the name of the adapter.
        const std::string& GetName() const noexcept { return _name; }

    protected:
        /// Constructor.
        GPUAdapter(GPUProvider* provider, BackendType backend)
            : _provider(provider)
            , _backend(backend)
        {
        }

        GPUProvider* _provider;
        BackendType _backend;
        uint32_t _vendorId = 0;
        uint32_t _deviceId = 0;
        GraphicsAdapterType _adapterType = GraphicsAdapterType::Unknown;
        std::string _name;

    private:
        ALIMER_DISABLE_COPY_MOVE(GPUAdapter);
    };
}
