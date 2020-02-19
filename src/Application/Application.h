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

#include "Core/Preprocessor.h"
#include <EASTL/string.h>

namespace alimer
{
    struct Configuration
    {
        eastl::string windowTitle = "Alimer";

        uint32_t windowWidth = 1280;
        uint32_t windowHeight = 720;
    };

    class AppContext;
    class ALIMER_API Application
    {
    public:
        Application(const Configuration& config);
        Application(AppContext* context, const Configuration& config);

        /// Destructor.
        virtual ~Application();

        void Run();

    protected:
        Configuration _config;
        bool _running = false;
        bool _exiting = false;

    private:
        AppContext* _context;
        bool _ownContext;
    };
}