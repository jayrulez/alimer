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

#if defined(ALIMER_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(ALIMER_IMPLEMENTATION)
#           define OS_EXPORT __declspec(dllexport)
#       else
#           define OS_EXPORT __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       if defined(ALIMER_IMPLEMENTATION)
#           define OS_EXPORT __attribute__((visibility("default")))
#       else
#           define OS_EXPORT
#       endif
#   endif  // defined(_WIN32)
#else       
#   define OS_EXPORT
#endif  // defined(ALIMER_SHARED_LIBRARY)


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct window_t window_t;

typedef enum {
    OS_EVENT_UNKNOWN = 0,
    OS_EVENT_QUIT,
    OS_EVENT_WINDOW,
    OS_EVENT_KEY_DOWN,
    OS_EVENT_KEY_UP,
} os_event_type;

typedef enum window_flags {
    WINDOW_FLAG_RESIZABLE = (1 << 0),
    WINDOW_FLAG_FULLSCREEN = (1 << 1),
    WINDOW_FLAG_EXCLUSIVE_FULLSCREEN = (1 << 2),
    WINDOW_FLAG_HIDDEN = (1 << 3),
    WINDOW_FLAG_BORDERLESS = (1 << 4),
    WINDOW_FLAG_MINIMIZED = (1 << 5),
    WINDOW_FLAG_MAXIMIZED = (1 << 6)
} window_flags;

typedef struct os_window_event {
    uint32_t window_id;
} os_window_event;

typedef struct os_key_event {
    uint32_t window_id;
    int code;
    bool alt;	
    bool ctrl;	
    bool shift;	
    bool system;
} os_key_event;

typedef struct {
    os_event_type type;

    union {
        os_window_event window;
        os_key_event key;
    };
} os_event;

#if defined(_WIN32) || defined(_WIN64)
typedef struct HWND__* HWND;
typedef struct HMONITOR__* HMONITOR;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /* Platform functions */
    bool os_init(void);
    void os_shutdown(void);
    bool event_poll(os_event* event);
    void event_push(os_event event);

    /* Dynamic library functions */
    void* library_open(const char* lib_name);
    bool library_is_valid(void* handle);
    void library_close(void* handle);
    void* library_symbol(void* handle, const char* name);

    /* Window functions */
    OS_EXPORT window_t* window_create(const char* title, uint32_t width, uint32_t height, uint32_t flags);
    OS_EXPORT void window_destroy(window_t* window);
    OS_EXPORT uint32_t window_get_id(window_t* window);
    OS_EXPORT window_t* window_from_id(uint32_t id);

    OS_EXPORT void window_maximize(window_t* window);
    OS_EXPORT void window_minimize(window_t* window);
    OS_EXPORT void window_restore(window_t* window);
    OS_EXPORT void window_resize(window_t* window, uint32_t width, uint32_t height);

    OS_EXPORT void window_set_title(window_t* window, const char* title);
    OS_EXPORT void window_get_position(window_t* window, int* x, int* y);
    OS_EXPORT void window_set_position(window_t* window, int x, int y);
    OS_EXPORT bool window_set_centered(window_t* window);

    OS_EXPORT uint32_t window_width(window_t* window);
    OS_EXPORT uint32_t window_height(window_t* window);
    OS_EXPORT void window_get_size(window_t* window, uint32_t* width, uint32_t* height);
    OS_EXPORT bool window_is_open(window_t* window);
    OS_EXPORT bool window_is_visible(window_t* window);
    OS_EXPORT bool window_is_maximized(window_t* window);
    OS_EXPORT bool window_is_minimized(window_t* window);
    OS_EXPORT bool window_is_focused(window_t* window);

#if defined(_WIN32) || defined(_WIN64)
    OS_EXPORT HWND window_handle(window_t* window);
    OS_EXPORT HMONITOR window_monitor(window_t* window);
#endif

    /* Clipboard functions */
    OS_EXPORT const char* clipboard_get_text(void);
    OS_EXPORT void clipboard_set_text(const char* text);

#ifdef __cplusplus
}
#endif
