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

#if defined(GLFW_BACKEND)

#include "os.h"
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

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

typedef struct window_t {
    uint32_t id;
    GLFWwindow* handle;
} window_t;

static struct {
    size_t event_head;
    os_event* events;
    window_t* windows;
} os;

static void on_glfw_error(int code, const char* description) {
    //ALIMER_THROW(description);
}

bool os_init(void) {
    os.event_head = 0;
    os.events = NULL;
    os.windows = NULL;

    glfwSetErrorCallback(on_glfw_error);
#ifdef __APPLE__
    glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif

    int result = glfwInit();
    if (result == GLFW_FALSE)
    {
        //TODO_ERROR_HANDLER(result);
        return false;
    }

    return true;
}

void os_shutdown(void) {
    glfwTerminate();
    os.event_head = 0;
    arrfree(os.events);
    arrfree(os.windows);
    memset(&os, 0, sizeof(os));
}

bool _event_pop(os_event* event) {
    if (os.event_head == arrlenu(os.events)) {
        os.event_head = 0;
        return false;
    }

    *event = os.events[os.event_head++];
    return true;
}

bool event_poll(os_event* event) {
    glfwPollEvents();

    /* Check if all windows are closed. */
    bool all_closed = true;
    for (size_t i = 0; i < arrlenu(os.windows); ++i) {
        if (!glfwWindowShouldClose(os.windows[i].handle)) {
            all_closed = false;
            break;
        }
    }

    if (all_closed)
    {
        os_event evt;
        evt.type = OS_EVENT_QUIT;
        event_push(evt);
    }

    // TODO: Fire quit event when all windows are closed.
    return _event_pop(event);
}

void event_push(os_event event) {
    arrpush(os.events, event);
}

/* Window functions */
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    window_t* user_data = (window_t*)glfwGetWindowUserPointer(window);

    os_event ev;
    ev.type = action == GLFW_RELEASE ? OS_EVENT_KEY_UP : OS_EVENT_KEY_DOWN;
    ev.key.window_id = window_get_id(user_data);
    ev.key.code = key;
    ev.key.alt = (mods & GLFW_MOD_ALT) != 0;
    ev.key.ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    ev.key.shift = (mods & GLFW_MOD_SHIFT) != 0;
    ev.key.system = (mods & GLFW_MOD_SUPER) != 0;
    event_push(ev);
}

window_t* window_create(const char* title, uint32_t width, uint32_t height, uint32_t flags) {
#if defined(ALIMER_GRAPHICS_OPENGL)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

    glfwWindowHint(GLFW_RESIZABLE, (flags & WINDOW_FLAG_RESIZABLE) ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, !(flags & WINDOW_FLAG_HIDDEN) ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, (flags & WINDOW_FLAG_BORDERLESS) ? GLFW_FALSE : GLFW_TRUE);

    if (flags & WINDOW_FLAG_MINIMIZED)
    {
        glfwWindowHint(GLFW_ICONIFIED, GLFW_TRUE);
    }
    else if (flags & WINDOW_FLAG_MAXIMIZED)
    {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    }

    GLFWmonitor* monitor = NULL;
    if (flags & WINDOW_FLAG_FULLSCREEN)
    {
        monitor = glfwGetPrimaryMonitor();
    }

    if (flags & WINDOW_FLAG_EXCLUSIVE_FULLSCREEN)
    {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }

    GLFWwindow* handle = glfwCreateWindow((int)width, (int)height, title, monitor, NULL);
    if (!handle)
    {
        //ALIMER_LOGERROR("GLFW: Failed to create window.");
        return NULL;
    }

    glfwDefaultWindowHints();
    glfwSetKeyCallback(handle, glfw_key_callback);

    /* Setup os data. */
    uint32_t window_id = (uint32_t)arrlenu(os.windows);
    window_t window = {
        .id = window_id,
        .handle = handle
    };
    stbds_arrpush(os.windows, window);
    glfwSetWindowUserPointer(handle, &window);
    return &os.windows[window_id];
}

void window_destroy(window_t* window) {
    glfwSetWindowShouldClose(window->handle, GLFW_TRUE);
    glfwDestroyWindow(window->handle);
    stbds_arrdel(os.windows, window->id);
}

uint32_t window_get_id(window_t* window) {
    return window->id;
}

window_t* window_from_id(uint32_t id) {
    return &os.windows[id];
}

void window_maximize(window_t* window) {
    glfwMaximizeWindow(window->handle);
}

void window_minimize(window_t* window) {
    glfwIconifyWindow(window->handle);
}

void window_restore(window_t* window) {
    glfwRestoreWindow(window->handle);
}

void window_resize(window_t* window, uint32_t width, uint32_t height) {
    glfwSetWindowSize(window->handle, (int)width, (int)height);
}

OS_EXPORT void window_set_title(window_t* window, const char* title) {
    glfwSetWindowTitle(window->handle, title);
}

void window_get_position(window_t* window, int* x, int* y) {
    glfwGetWindowPos(window->handle, x, y);
}

void window_set_position(window_t* window, int x, int y) {
    glfwSetWindowPos(window->handle, x, y);
}

bool window_set_centered(window_t* window) {
    int sx = 0, sy = 0;
    int px = 0, py = 0;
    int mx = 0, my = 0;
    int monitor_count = 0;
    int best_area = 0;
    int final_x = 0, final_y = 0;

    glfwGetWindowSize(window->handle, &sx, &sy);
    glfwGetWindowPos(window->handle, &px, &py);

    // Iterate throug all monitors
    GLFWmonitor** m = glfwGetMonitors(&monitor_count);
    if (!m)
        return false;

    for (int j = 0; j < monitor_count; ++j)
    {

        glfwGetMonitorPos(m[j], &mx, &my);
        const GLFWvidmode* mode = glfwGetVideoMode(m[j]);
        if (!mode)
            continue;

        // Get intersection of two rectangles - screen and window
        int minX = max(mx, px);
        int minY = max(my, py);

        int maxX = min(mx + mode->width, px + sx);
        int maxY = min(my + mode->height, py + sy);

        // Calculate area of the intersection
        int area = max(maxX - minX, 0) * max(maxY - minY, 0);

        // If its bigger than actual (window covers more space on this monitor)
        if (area > best_area)
        {
            // Calculate proper position in this monitor
            final_x = mx + (mode->width - sx) / 2;
            final_y = my + (mode->height - sy) / 2;

            best_area = area;
        }
    }

    // We found something
    if (best_area)
        glfwSetWindowPos(window->handle, final_x, final_y);

    // Something is wrong - current window has NOT any intersection with any monitors. Move it to the default
    // one.
    else
    {
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        if (primary)
        {
            const GLFWvidmode* desktop = glfwGetVideoMode(primary);

            if (desktop) {
                glfwSetWindowPos(window->handle, (desktop->width - sx) / 2, (desktop->height - sy) / 2);
            }

            return false;
        }
        else {
            return false;
        }
    }

    return true;
}

void window_get_size(window_t* window, uint32_t* width, uint32_t* height) {
    int w, h;
    glfwGetWindowSize(window->handle, &w, &h);
    if (width) {
        *width = (uint32_t)w;
    }

    if (height) {
        *height = (uint32_t)h;
    }
}

bool window_is_open(window_t* window) {
    return (window && glfwWindowShouldClose(window->handle));
}

bool window_is_visible(window_t* window) {
    return (window->handle && glfwGetWindowAttrib(window->handle, GLFW_VISIBLE));
}

bool window_is_maximized(window_t* window) {
    return (window->handle && glfwGetWindowAttrib(window->handle, GLFW_ICONIFIED));
}

bool window_is_minimized(window_t* window) {
    return (window->handle && glfwGetWindowAttrib(window->handle, GLFW_MAXIMIZED));
}

bool window_is_focused(window_t* window) {
    return (window->handle && glfwGetWindowAttrib(window->handle, GLFW_FOCUSED));
}

#if defined(_WIN32) || defined(_WIN64)
HWND window_handle(window_t* window) {
    return glfwGetWin32Window(window->handle);
}

HMONITOR window_monitor(window_t* window) {
    return MonitorFromWindow(window_handle(window), MONITOR_DEFAULTTOPRIMARY);
}
#endif

/* Clipboard functions */
const char* clipboard_get_text(void) {
    return glfwGetClipboardString(NULL);
}

void clipboard_set_text(const char* text) {
    glfwSetClipboardString(NULL, text);
}

#endif /* defined(GLFW_BACKEND) */
