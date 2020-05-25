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

#include "D3D11Plugin.h"
#include "D3D11GraphicsProvider.h"
#include "Core/Engine.h"

namespace Alimer
{
    D3D11Plugin::D3D11Plugin(Engine& engine)
        : engine{ engine }
    {

    }

    void D3D11Plugin::Init()
    {
        engine.RegisterGraphicsProviderFactory(new D3D11GraphicsProviderFactory());
    }

    const char* D3D11Plugin::GetName() const
    {
        static const std::string sPluginName = "Alimer.Direct3D11";
        return sPluginName.c_str();
    }
}

extern "C"
{
    ALIMER_INTERFACE_EXPORT Alimer::IPlugin* AlimerCreatePlugin(Alimer::Engine& engine)
    {
        return new Alimer::D3D11Plugin(engine);
    }
}
