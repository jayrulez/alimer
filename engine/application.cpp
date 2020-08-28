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
#include "application.h"
#include "core/log.h"
#include "platform/platform.h"

using namespace Alimer;

namespace
{
    static Config app_config;
    static bool app_is_running = false;
}

bool App::run(const Config* config)
{
    app_config = *config;

    // figure out the graphics api
    if (app_config.graphics_backend == graphics::BackendType::Default)
    {
        app_config.graphics_backend = graphics::get_platform_backend();
        if (app_config.graphics_backend == graphics::BackendType::Default)
        {
            return false;
        }
    }

    // Init platform first.
    if (!Platform::init(&app_config))
    {
        Log::error("Failed to initialize system module");
        return false;
    }

    // Init graphics
    if (!graphics::init(&app_config))
    {
    }

    app_is_running = true;
    // Run platform main loop.
    Platform::run();

    Platform::shutdown();
    return true;
}

bool App::is_running(void)
{
    return app_is_running;
}

void App::tick(void)
{
    // TODO: add timer.

    graphics::begin_frame();
    graphics::end_frame();
}

const Config* App::get_config()
{
    return &app_config;
}

uint32_t App::get_width()
{
    return app_config.width;
}

uint32_t App::get_height()
{
    return app_config.height;
}
