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

#include "Core/Utils.h"
#include <EASTL/string.h>

namespace Alimer
{
    enum class WindowStyle : uint32_t
    {
        None = 0,
        Resizable = 0x01,
        Fullscreen = 0x02,
        ExclusiveFullscreen = 0x04,
        HighDpi = 0x08,
        Default = Resizable
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(WindowStyle);

    /// Defines an OS Game Window.
    class ALIMER_API GameWindow
    {
    protected:
        GameWindow(const eastl::string& newTitle, uint32_t newWidth, uint32_t newHeight, WindowStyle style);

    public:
        /// Destructor.
        virtual ~GameWindow() = default;

        virtual bool ShouldClose() const = 0;

        /// Set the window title.
        void SetTitle(const eastl::string& newTitle);

        /// Return the window title.
        const eastl::string& GetTitle() const { return title; }

        virtual bool IsMinimized() const = 0;

    private:
        virtual void BackendSetTitle() {};

    protected:
        eastl::string title;
        uint32_t width;
        uint32_t height;
        bool resizable = false;
        bool fullscreen = false;
        bool exclusiveFullscreen = false;
        bool highDpi = true;
        bool visible = true;

    private:
        ALIMER_DISABLE_COPY_MOVE(GameWindow);
    };
}
