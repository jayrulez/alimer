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

#include "Application/Application.h"
#include "Application/AppContext.h"
#include "Core/Log.h"

/* Needed by EASTL. */
#if !defined(ALIMER_EXPORTS)
ALIMER_API void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return new uint8_t[size];
}

ALIMER_API void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return new uint8_t[size];
}
#endif

namespace Alimer
{
    Application::Application(const Configuration& config)
        : Application(nullptr, config)
    {

    }

    Application::Application(AppContext* context, const Configuration& config)
        : _config(config)
        , _ownContext(false)
    {
        _context = context;
        if (!_context) {
            _context = AppContext::CreateDefault(this);
            _ownContext = true;
        }
    }

    Application::~Application()
    {
        if (_ownContext)
        {
            SafeDelete(_context);
        }
    }

    void Application::InitBeforeRun()
    {

    }

    void Application::Run()
    {
        if (_running) {
            ALIMER_LOGERROR("Application is already running");
            return;
        }

        if (_exiting) {
            ALIMER_LOGERROR("Application is exiting");
            return;
        }

        _context->Run();
    }

    void Application::Tick()
    {

    }

    void Application::GetDefaultWindowSize(uint32_t* width, uint32_t* height) const
    {
        if (width)
            *width = 1280;

        if (height)
            *height = 720;
    }

    Window* Application::GetMainWindow() const
    {
        return _context->GetMainWindow();
    }
}
