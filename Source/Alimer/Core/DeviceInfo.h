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

#include "Core/Assert.h"
#include <string>

namespace Alimer
{
    /// Identifiers the running platform type.
    enum class PlatformId : uint32_t
    {
        /// Unknown platform.
        Unknown,
        /// Windows platform.
        Windows,
        /// Linux platform.
        Linux,
        /// macOS platform.
        macOS,
        /// Android platform.
        Android,
        /// iOS platform.
        iOS,
        /// tvOS platform.
        tvOS,
        /// Universal Windows platform.
        UWP,
        /// Xbox One platform.
        XboxOne,
        /// Web platform.
        Web
    };

    /// Identifiers the running platform family.
    enum class PlatformFamily : uint32_t
    {
        /// Unknown family.
        Unknown,
        /// Mobile family.
        Mobile,
        /// Desktop family.
        Desktop,
        /// Console family.
        Console
    };

#if defined(_WIN32) || defined(_WIN64)
    enum class WindowsVersion : uint32_t
    {
        Unknown,
        Win7,
        Win8,
        Win81,
        Win10
    };
#endif

    using ProcessId = uint32_t;

    class ALIMER_API DeviceInfo final
    {
    public:
        /// Return the current platform name.
        static const char* GetName();

        /// Return the current platform ID.
        static PlatformId GetId();

        /// Return the current platform family.
        static PlatformFamily GetFamily();

#if defined(_WIN32) || defined(_WIN64)
        /// Return the current windows version.
        static WindowsVersion GetWindowsVersion();
#endif
    };

    /// Returns the current process id (pid)
    ALIMER_API ProcessId GetCurrentProcessId();
}
