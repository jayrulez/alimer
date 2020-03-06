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

#include "Games/GameWindow.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/SwapChain.h"
using namespace eastl;

namespace Alimer
{
    GameWindow::GameWindow(const string& newTitle, const SizeU& newSize, WindowStyle style)
        : title(newTitle)
        , size(newSize)
        , resizable(any(style& WindowStyle::Resizable))
        , fullscreen(any(style& WindowStyle::Fullscreen))
        , exclusiveFullscreen(any(style& WindowStyle::ExclusiveFullscreen))
    {

    }

    void GameWindow::SetTitle(const string& newTitle)
    {
        title = newTitle;
        BackendSetTitle();
    }

    void GameWindow::SetGraphicsDevice(GraphicsDevice* newDevice)
    {
        device = newDevice;
        SwapChainDescriptor descriptor = {};
        descriptor.width = size.width;
        descriptor.height = size.height;
        swapChain = device->CreateSwapChain(GetNativeHandle(), &descriptor);
    }

    void GameWindow::Present()
    {
        swapChain->Present();
    }
}
