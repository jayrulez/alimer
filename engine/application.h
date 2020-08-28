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

#include "graphics/graphics.h"

namespace Alimer
{
    enum class EventType : uint8_t
    {
        Unkwnown = 0,
        Quit,
    };

    struct Event
    {
        EventType type;
    };

    struct Config
    {
        uint32_t width;
        uint32_t height;
        bool fullscreen;
        bool resizable;
        bool debug;
        int vsync;
        uint32_t sample_count;
        const char* title;

        graphics::BackendType graphics_backend;
        graphics::PowerPreference power_preference;

        /* Callbacks */
        void* user_data;
        void (*on_startup)(void* user_data);
        void (*on_shutdown)(void* user_data);
        void (*on_event)(void* user_data, const Event* event);
    };

    namespace App
    {
#if !defined(ALIMER_SHARED_LIBRARY)
        extern Config main(int argc, char* argv[]);
#endif

        // Run the application.
        bool run(const Config* config);

        // Returns whether the application is running.
        bool is_running(void);

        // Tick one frame.
        void tick(void);

        // Gets the config data used to run the application
        const Config* get_config();

        // Gets the main window width.
        uint32_t get_width();

        // Gets the main window height.
        uint32_t get_height();
    }
} 
