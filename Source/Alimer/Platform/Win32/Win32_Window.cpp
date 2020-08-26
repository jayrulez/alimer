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

#include "Win32_Window.h"
#include "Win32_Include.h"

using namespace std;

namespace Alimer
{
    namespace
    {
        LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            Win32_Window* window = reinterpret_cast<Win32_Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if(!window)
                return DefWindowProcW(hWnd, message, wParam, lParam);

            switch (message)
            {
            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            case WM_PAINT:
                /*if (s_in_sizemove && game)
                {
                    game->Tick();
                }
                else*/
                {
                    PAINTSTRUCT ps;
                    (void)BeginPaint(hWnd, &ps);
                    EndPaint(hWnd, &ps);
                }
                break;
            }

            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
    }

    Win32_Window::Win32_Window(const std::string& title,  uint32_t width, uint32_t height)
        : Window()
    {
        static const TCHAR AppWindowClass[] = TEXT("AlimerApp");
        HINSTANCE hInstance = GetModuleHandleW(nullptr);

        static bool wcexInit = false;
        if (!wcexInit) {
            // Register class
            WNDCLASSEXW wcex = {};
            wcex.cbSize = sizeof(WNDCLASSEXW);
            wcex.style = CS_HREDRAW | CS_VREDRAW;
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

        // TODO: Center
        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;

        const auto windowStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_BORDER | WS_DLGFRAME | WS_THICKFRAME | WS_GROUP | WS_TABSTOP;
        const auto windowExStyle = WS_EX_APPWINDOW;

        RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRectEx(&windowRect, windowStyle, FALSE, windowExStyle);

        width = (width > 0) ? windowRect.right - windowRect.left : CW_USEDEFAULT;
        height = (height > 0) ? windowRect.bottom - windowRect.top : CW_USEDEFAULT;

        auto wideTitle = ToUtf16(title);
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

        SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }

    void Win32_Window::Show()
    {
        ShowWindow(handle, SW_SHOW);
    }

    void Win32_Window::SetPlatformTitle(const std::string& newTitle)
    {
        auto wideTitle = ToUtf16(title);
        SetWindowTextW(handle, wideTitle.c_str());
    }

    Rect Win32_Window::GetBounds() const
    {
        POINT pos{};
        RECT rc{};
        ClientToScreen(handle, &pos);
        GetClientRect(handle, &rc);

        Rect result{};
        result.x = static_cast<float>(pos.x);
        result.y = static_cast<float>(pos.y);
        result.width = static_cast<float>(rc.right - rc.left);
        result.height = static_cast<float>(rc.bottom - rc.top);
        return result;
    }

    bool Win32_Window::IsVisible() const
    {
        return ::IsWindowVisible(handle);
    }

    bool Win32_Window::IsMaximized() const
    {
        return IsZoomed(handle);
    }

    bool Win32_Window::IsMinimized() const
    {
        return IsIconic(handle);
    }

    NativeHandle Win32_Window::GetNativeHandle() const
    {
        return handle;
    }
}
