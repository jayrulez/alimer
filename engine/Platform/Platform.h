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

#include <memory>
#include <string>
#include <vector>
#include "Platform/Window.h"

namespace Alimer
{
    /// Identifiers the running platform type.
    enum class PlatformId
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
    enum class PlatformFamily
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

    class Application;

    class ALIMER_API Platform
    {
        friend class Application;

    public:
        Platform(Application* application);
        virtual ~Platform() = default;

        /// Return the current platform name.
        static std::string GetName();

        /// Return the current platform ID.
        static PlatformId GetId();

        /// Return the current platform family.
        static PlatformFamily GetFamily();

        /// Create plaform logic with given application.
        static std::unique_ptr<Platform> Create(Application* application);

        Window& GetMainWindow() const;

        static std::vector<std::string> GetArguments();
        static void SetArguments(const std::vector<std::string>& args);

    protected:
        void InitApplication();
        virtual void Run() = 0;

        Application* application;
        static std::vector<std::string> arguments;
    
        std::unique_ptr<Window> window{ nullptr };
    };
}
