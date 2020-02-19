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

#include "Application.h"
#include "AppContext.h"

/* Needed by EASTL. */
#if !defined(ALIMER_EXPORTS)
ALIMER_API void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::operator new(size);
}

ALIMER_API void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::operator new(size);
}
#endif

namespace alimer
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

    void Application::Run()
    {
        if (_running) {
            //LOGF("Application is already running");
        }

        if (_exiting) {
            //LOGF("Application is exiting");
        }
    }
}
