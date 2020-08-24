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

#include "core/application.h"
#include <emscripten.h>
#include <emscripten/html5.h>

namespace Alimer
{
    namespace
    {
        EM_BOOL frame_loop(double time, void* userData)
        {
            printf("frame_loop %g!\n", time);
            static_cast<Alimer::Application*>(userData)->run_frame();
            return EM_TRUE;
        }
    }

    int application_main(Application* (*create_application)(int, char**), int argc, char* argv[])
    {
        printf("hello, world!\n");
        auto app = std::unique_ptr<Alimer::Application>(create_application(argc, argv));

        if (app)
        {
            emscripten_request_animation_frame_loop(frame_loop, app.get());

            app.reset();
            return 0;
        }

        return 1;
    }
}
