//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#include "graphics/graphics_driver.h"
#include "graphics_opengl.h"

namespace alimer
{
    namespace graphics
    {
        struct GLBuffer
        {
            enum { MAX_COUNT = 4096 };

            GLuint handle;
        };

        struct GLDeviceBackend {
            GLContext context;

            PFNGLCLEARPROC glClear = nullptr;
            PFNGLCLEARCOLORPROC glClearColor = nullptr;

            Pool<GLBuffer, GLBuffer::MAX_COUNT> buffers;
        };

        static void GLDestroy(DeviceImpl* device) {
            GLDeviceBackend* backend = (GLDeviceBackend*)device->backend;

            // Destroy GL context.
            DestroyGLContext(backend->context);

            delete backend;
            backend = nullptr;

            delete device;
            device = nullptr;
        }

        static void GLBeginFrame(DeviceBackend*) {
            // NOP
        }

        static void GLPresentFrame(DeviceBackend* driver) {
            GLDeviceBackend* backend = (GLDeviceBackend*)driver;

            backend->glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            backend->glClear(GL_COLOR_BUFFER_BIT);

            SwapBuffers(backend->context);
        }

        /* Driver functions */
        static bool GLIsSupported() {
            return true;
        }

        static Device GLCreateDevice(const DeviceParams& params) {
            Device device = new DeviceImpl();
            GLDeviceBackend* backend = new GLDeviceBackend();
            backend->context = CreateGLContext(params.validation, params.windowHandle, params.colorFormat, params.depthStencilFormat, 1u);

            /* Load opengl function*/
#define LOAD(name) GetGLProcAddress(backend->context, backend->name, #name)

            LOAD(glClear);
            LOAD(glClearColor);

            /* Setup function table */
            device->Destroy = GLDestroy;
            device->BeginFrame = GLBeginFrame;
            device->PresentFrame = GLPresentFrame;
            device->backend = (DeviceBackend*)backend;
            return device;
        }

        Driver gl_driver = {
            BackendType::OpenGL,
            GLIsSupported,
            GLCreateDevice
        };
    }
}
