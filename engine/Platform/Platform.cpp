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

#include "Core/Assert.h"
#include "Core/Log.h"
#include "Platform/Platform.h"
#include "Platform/Application.h"

namespace Alimer
{
    std::vector<std::string> Platform::arguments = {};

    Platform::Platform(Application* application)
        : application{ application }
    {
        ALIMER_ASSERT(application);
    }

    Window& Platform::GetMainWindow() const
    {
        return *window;
    }

    void Platform::InitApplication()
    {
        application->InitBeforeRun();
    }

    std::string Platform::GetName()
    {
        return "Windows";
    }

    PlatformId Platform::GetId()
    {
        return PlatformId::Windows;
    }

    PlatformFamily Platform::GetFamily()
    {
        return PlatformFamily::Desktop;
    }

    std::vector<std::string> Platform::GetArguments()
    {
        return Platform::arguments;
    }

    void Platform::SetArguments(const std::vector<std::string>& args)
    {
        arguments = args;
    }
}
