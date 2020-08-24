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

#include "window_windows.h"
#include "windows_private.h"
#include "io/path.h"
//#include <stdexcept>

namespace Alimer
{
    namespace
    {
        LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            switch (message)
            {
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            }

            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
    }

    WindowsWindow::WindowsWindow(const std::string& title, int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        static const TCHAR AppWindowClass[] = TEXT("AlimerApp");
        HINSTANCE hInstance = GetModuleHandleW(nullptr);

        static bool wcexInit = false;
        if (!wcexInit) {
            // Register class
            WNDCLASSEXW wcex = {};
            wcex.cbSize = sizeof(WNDCLASSEXW);
            wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wcex.lpfnWndProc = WndProc;
            wcex.hInstance = hInstance;
            wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
            wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            wcex.lpszClassName = AppWindowClass;
            wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
            if (!RegisterClassExW(&wcex))
            {
                return;
            }
        }

        x = CW_USEDEFAULT;
        y = CW_USEDEFAULT;

        const auto windowStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_BORDER | WS_DLGFRAME | WS_THICKFRAME | WS_GROUP | WS_TABSTOP;
        const auto windowExStyle = WS_EX_APPWINDOW;

        RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRectEx(&windowRect, windowStyle, FALSE, windowExStyle);

        width = (width > 0) ? windowRect.right - windowRect.left : CW_USEDEFAULT;
        height = (height > 0.0F) ? windowRect.bottom - windowRect.top : CW_USEDEFAULT;

        auto wideTitle = Path::to_utf16(title);
        handle = CreateWindowExW(windowExStyle,
            AppWindowClass,
            wideTitle.c_str(),
            windowStyle,
            x, y,
            width, height,
            nullptr, nullptr,
            hInstance,
            nullptr);

        //if (!handle)
        //    throw std::system_error(GetLastError(), std::system_category(), "Failed to create window");

        ShowWindow(handle, SW_SHOW);
        SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
}
