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

#include "Core/Window.h"
#include "Core/Input.h"
#include "Core/Log.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#if defined(_WIN32)
#   define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#   define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#   define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#include "imgui_impl_glfw.h"

namespace alimer
{
    namespace
    {
        void OnGLFWError(int code, const char* description) {
            LOGE("GLFW  Error (code {}): {}", code, description);
        }

        struct GLFWInitializer
        {
            GLFWInitializer()
            {
                glfwSetErrorCallback(OnGLFWError);
                if (!glfwInit())
                {
                    LOGE("GLFW couldn't be initialized.");
                }
            }

            ~GLFWInitializer()
            {
                glfwTerminate();
            }
        };

        GLFWInitializer s_glfw_Initializer;

        MouseButton FromGlfw(int button)
        {
            switch (button)
            {
            case GLFW_MOUSE_BUTTON_LEFT:
                return MouseButton::Left;
            case GLFW_MOUSE_BUTTON_RIGHT:
                return MouseButton::Right;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                return MouseButton::Middle;
            case GLFW_MOUSE_BUTTON_4:
                return MouseButton::XButton1;
            case GLFW_MOUSE_BUTTON_5:
                return MouseButton::XButton2;
            default:
                return MouseButton::Count;
            }
        }

        ModifierKeys ModifiersFromGlfw(int mods)
        {
            ModifierKeys modifiers = ModifierKeys::None;
            if (mods & GLFW_MOD_ALT)
                modifiers |= ModifierKeys::Alt;
            if (mods & GLFW_MOD_CONTROL)
                modifiers |= ModifierKeys::Control;
            if (mods & GLFW_MOD_SHIFT)
                modifiers |= ModifierKeys::Shift;
            if (mods & GLFW_MOD_SUPER)
                modifiers |= ModifierKeys::Meta;
            return modifiers;
        }

        void Glfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
        {
            //auto* engineWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
            double x{};
            double y{};
            glfwGetCursorPos(window, &x, &y);

            Object::GetInput()->PostMousePressEvent(
                static_cast<int32_t>(x),
                static_cast<int32_t>(y),
                FromGlfw(button),
                ModifiersFromGlfw(mods),
                action == GLFW_PRESS);
        }
    }

    Window::~Window()
    {
        Close();
    }

    void Window::Close()
    {
        if (window != nullptr)
        {
            ImGui_ImplGlfw_Shutdown();
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            window = nullptr;
        }
    }

    void Window::BeginFrame()
    {
        ImGui_ImplGlfw_NewFrame();
    }

    bool Window::ShouldClose() const
    {
        if (window == nullptr) {
            return true;
        }

        return glfwWindowShouldClose(window) == GLFW_TRUE;
    }

    bool Window::IsVisible() const
    {
        if (window == nullptr) {
            return false;
        }

        return glfwGetWindowAttrib(window, GLFW_VISIBLE) == GLFW_TRUE;
    }

    bool Window::IsMaximized() const
    {
        if (window == nullptr) {
            return false;
        }

        return glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE;
    }

    bool Window::IsMinimized() const
    {
        if (window == nullptr) {
            return false;
        }

        return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
    }

    bool Window::IsFullscreen() const
    {
        return fullscreen || exclusiveFullscreen;
    }

    bool Window::SetSize(const UInt2& size_, WindowFlags flags)
    {
        resizable = any(flags & WindowFlags::Resizable);
        fullscreen = any(flags & WindowFlags::Fullscreen);
        exclusiveFullscreen = any(flags & WindowFlags::ExclusiveFullscreen);

        if (window == nullptr)
        {
            /*if (any(flags & WindowFlags::OpenGL))
            {
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
                glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
                //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            }
            else*/
            {
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            }

            glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
            if ((flags & WindowFlags::Hidden) != WindowFlags::None) {
                glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            }
            else
            {
                glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
            }

            //glfwWindowHint(GLFW_DECORATED, (flags & WINDOW_FLAG_BORDERLESS) ? GLFW_FALSE : GLFW_TRUE);

            if (any(flags & WindowFlags::Minimized))
            {
                glfwWindowHint(GLFW_ICONIFIED, GLFW_TRUE);
            }
            else if (any(flags & WindowFlags::Maximized))
            {
                glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
            }

            GLFWmonitor* monitor = nullptr;
            if (fullscreen)
            {
                monitor = glfwGetPrimaryMonitor();
            }

            if (exclusiveFullscreen)
            {
                monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);

                glfwWindowHint(GLFW_RED_BITS, mode->redBits);
                glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
                glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
                glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
                glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
            }

            window = glfwCreateWindow(static_cast<int>(size_.x), static_cast<int>(size_.x), title.c_str(), monitor, nullptr);
            if (window == nullptr)
            {
                LOGE("GLFW: Failed to create window.");
                return false;
            }

            glfwDefaultWindowHints();
            glfwSetWindowUserPointer(window, this);
            glfwSetMouseButtonCallback(window, Glfw_MouseButtonCallback);
            //glfwSetKeyCallback(glfwWindow, glfw_key_callback);

        }
        else
        {
            glfwSetWindowSize(window, static_cast<int>(size_.x), static_cast<int>(size_.y));
            glfwSetWindowAttrib(window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
        }

        // Update size from GLFW
        int w;
        int h;
        glfwGetWindowSize(window, &w, &h);
        size.x = static_cast<uint32_t>(w);
        size.y = static_cast<uint32_t>(h);

        /*if (any(flags & WindowFlags::OpenGL))
        {
            glfwMakeContextCurrent(handle);
            ImGui_ImplGlfw_InitForOpenGL(handle, true);
        }
        else
        {
            ImGui_ImplGlfw_InitForVulkan(handle, true);
        }*/

        return true;
    }

    void Window::SetTitle(const String& newTitle)
    {
        title = newTitle;
        if (window)
        {
            glfwSetWindowTitle(window, newTitle.c_str());
        }
    }

    WindowHandle Window::GetHandle() const
    {
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
        return glfwGetWin32Window(window);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        WindowHandle handle{};
        handle.display = glfwGetX11Display();
        handle.window = glfwGetX11Window();
        return handle;
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
        return glfwGetCocoaWindow(window);
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
        WindowHandle handle{};
        handle.display = glfwGetWaylandDisplay();
        handle.window = = glfwGetWaylandWindow(window);
        return handle;
#endif
    }
}
