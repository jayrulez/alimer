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

#include "core/Utils.h"
#include "graphics/Types.h"
#include <string>

namespace alimer
{
    class GraphicsProvider;

    /// Defines the physical graphics adapter class.
    class ALIMER_API GraphicsAdapter
    {
    public:
        /// Destructor.
        virtual ~GraphicsAdapter() = default;

        inline GraphicsProvider* GetProvider() const { return provider; }

        /// Gets the adapter PCI Vendor ID (VID).
        virtual uint32_t GetVendorId() const = 0;

        /// Gets the adapter PCI Device ID (DID).
        virtual uint32_t GetDeviceId() const = 0;

        /// Get the type of the adapter.
        virtual GraphicsAdapterType GetType() const = 0;

        /// Gets the name of the adapter.
        virtual const std::string& GetName() const = 0;

    protected:
        /// Constructor.
        GraphicsAdapter::GraphicsAdapter(GraphicsProvider* provider_)
            : provider(provider_)
        {
        }
       
    private:
        GraphicsProvider* provider;

        ALIMER_DISABLE_COPY_MOVE(GraphicsAdapter);
    };
}
