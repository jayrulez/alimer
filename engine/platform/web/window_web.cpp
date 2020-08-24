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

#include "window_web.h"
#include <emscripten.h>
#include <emscripten/html5.h>

using namespace std;

namespace Alimer
{
    namespace
    {
        EM_BOOL web_resize_callback(int eventType, const EmscriptenUiEvent* uiEvent, void* userData)
        {
            if (eventType == EMSCRIPTEN_EVENT_RESIZE)
            {
            }

            return true;
        }
    }

    WindowWeb::WindowWeb(const char* canvasName, const string& title, int32_t x, int32_t y, uint32_t width, uint32_t height)
        : canvasName{ canvasName }
    {
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, web_resize_callback);
    }

    unique_ptr<Window> Window::create(const string& title, int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        return make_unique<WindowWeb>("#canvas", title, x, y, width, height);
    }
}
