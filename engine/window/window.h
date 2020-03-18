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

#include "window/Event.h"
#include "Core/Utils.h"
#include "math/Size.h"
#include <string>

namespace alimer
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

    using native_handle = void*;
    using native_display = void*;

    class GPUDevice;
    class WindowImpl;

    /// Defines an OS Window.
    class ALIMER_API Window final
    {
    public:
        /// Constructor.
        Window(GPUDevice* device, const std::string& newTitle, const SizeU& newSize, WindowStyle style);

        /// Destructor.
        ~Window();

        /// Close the window.
        void close();

        /// Return whether or not the window is open,
        bool isOpen() const;

        inline const SizeU& GetSize() const noexcept { return size; }

        /// Set the window title.
        void set_title(const std::string& newTitle);

        /// Return the window title.
        const std::string& get_title() const { return title; }

        /// Return whether the window is minimized.
        bool IsMinimized() const;

        native_handle get_native_handle() const;
        native_display get_native_display() const;

    private:
        GPUDevice* device;
        std::string title;
        SizeU size;
        bool resizable = false;
        bool fullscreen = false;
        bool exclusiveFullscreen = false;
        bool highDpi = true;
        bool visible = true;
        WindowImpl* impl;
        
        ALIMER_DISABLE_COPY_MOVE(Window);
    };
}
