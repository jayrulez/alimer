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

#include "WindowsWindow.h"
#include "Application/Application.h"
#include "Core/Platform.h"
#include "Core/Log.h"

namespace Alimer
{
    static uint32_t s_windowCount = 0;
    constexpr LPCWSTR WINDOW_CLASS_NAME = L"AlimerWindow";
    constexpr DWORD windowFullscreenStyle = WS_CLIPSIBLINGS | WS_GROUP | WS_TABSTOP;
    LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    WindowsWindow::WindowsWindow(const eastl::string& newTitle, uint32_t newWidth, uint32_t newHeight, WindowStyle style)
        : Window(newTitle, newWidth, newHeight, style)
        , hInstance(GetModuleHandleW(nullptr))
    {
        // Register the window class at first call
        if (s_windowCount == 0) {
            // Register class
            WNDCLASSEXW wc;
            ZeroMemory(&wc, sizeof(wc));
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc = WndProc;
            wc.hInstance = hInstance;
            wc.hIcon = LoadIconW(hInstance, L"IDI_ICON");
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = WINDOW_CLASS_NAME;
            wc.hIconSm = LoadIconW(hInstance, L"IDI_ICON");
            if (!RegisterClassExW(&wc))
                return;
        }

        windowStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_BORDER | WS_DLGFRAME | WS_THICKFRAME | WS_GROUP | WS_TABSTOP;
        if (resizable)
            windowStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;

        RECT windowRect = { 0, 0, static_cast<LONG>(newWidth), static_cast<LONG>(newHeight) };
        AdjustWindowRectEx(&windowRect, windowStyle, FALSE, windowExStyle);

        const int x = CW_USEDEFAULT;
        const int y = CW_USEDEFAULT;
        const int windowWidth = (newWidth > 0) ? windowRect.right - windowRect.left : CW_USEDEFAULT;
        const int windowHeight = (newHeight > 0) ? windowRect.bottom - windowRect.top : CW_USEDEFAULT;

        auto wideTitle = ToUtf16(newTitle);
        hWnd = CreateWindowExW(windowExStyle, WINDOW_CLASS_NAME, wideTitle.c_str(), windowStyle,
            x, y, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

        if (!hWnd) {
            ALIMER_LOGERROR("Failed to create window");
            return;
        }

        monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

        if (fullscreen)
            SwitchFullscreen(fullscreen);

        if (!GetClientRect(hWnd, &windowRect)) {
            ALIMER_LOGERROR("Failed to get client rectangle");
        }
        else {
            width = static_cast<uint32_t>(windowRect.right - windowRect.left);
            height = static_cast<uint32_t>(windowRect.bottom - windowRect.top);
        }

        ShowWindow(hWnd, SW_SHOW);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        s_windowCount++;
    }

    WindowsWindow::~WindowsWindow()
    {
    }

    void WindowsWindow::SwitchFullscreen(bool newFullscreen)
    {

    }

    // Windows procedure
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        static bool s_in_sizemove = false;
        WindowsWindow* window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

        switch (message)
        {
        case WM_PAINT:
            if (s_in_sizemove && window)
            {
                //game->Tick();
            }
            else
            {
                PAINTSTRUCT ps;
                (void)BeginPaint(hWnd, &ps);
                EndPaint(hWnd, &ps);
            }
            break;

        case WM_GETMINMAXINFO:
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = 320;
            info->ptMinTrackSize.y = 200;
        }
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }

        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}
