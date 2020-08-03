//
// Copyright (c) 2020 Amer Koleci.
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

#if defined(AGPU_DRIVER_OPENGL)
#include "agpu_driver.h"

#if defined(_WIN32) // Windows
#   define AGPU_INTERFACE_WGL 1
#elif defined(__APPLE__) // macOS, iOS, tvOS
#   include <TargetConditionals.h>
#   if TARGET_OS_WATCH // watchOS
#       error "Apple Watch is not supported"
#   elif TARGET_OS_IOS || TARGET_OS_TV // iOS, tvOS
#       define ALIMER_GLES 1
#       define AGPU_INTERFACE_EAGL 1
#   elif TARGET_OS_MAC // any other Apple OS (check this last because it is defined for all Apple platforms)
#       define AGPU_INTERFACE_CGL 1
#   endif
#elif defined(__ANDROID__) // Android (check this before Linux because __linux__ is also defined for Android)
#   define ALIMER_GLES 1
#   define GFX_EGL 1
#elif defined(__linux__) // Linux
#   if defined(__x86_64__) || defined(__i386__) // x86 Linux
#       define AGPU_INTERFACE_GLX 1
#   elif defined(__arm64__) || defined(__aarch64__) || defined(__arm__) // ARM Linux
#       define ALIMER_GLES 1
#       define AGPU_INTERFACE_EGL 1
#   else
#       error "Unsupported architecture"
#   endif
#elif defined(__EMSCRIPTEN__) // Emscripten
#   define ALIMER_GLES 1
#   define AGPU_INTERFACE_EGL 1
#else
#   error "Unsupported platform"
#endif

#if defined(ALIMER_GLES)
#   include "GLES2/gl2.h"
#   include "GLES2/gl2ext.h"
#   include "GLES3/gl3.h"
#   include "GLES3/gl31.h"
#   include "GLES3/gl32.h"
#else
#   include "GL/glcorearb.h"
#   include "GL/glext.h"
#endif

/* Loader */
#define AGPU_GL_FOREACH(X)\
    X(glGetError, GLGETERROR)\
    X(glGetIntegerv, GLGETINTEGERV)\
    X(glEnable, GLENABLE)\
    X(glDisable, GLDISABLE)\
    X(glVertexBindingDivisor, GLVERTEXBINDINGDIVISOR)\
    X(glVertexAttribBinding, GLVERTEXATTRIBBINDING)\
    X(glVertexAttribFormat, GLVERTEXATTRIBFORMAT)\
    X(glVertexAttribIFormat, GLVERTEXATTRIBIFORMAT)\
    X(glBindVertexBuffer, GLBINDVERTEXBUFFER)\
    X(glCullFace, GLCULLFACE)\
    X(glFrontFace, GLFRONTFACE)\
    X(glPolygonOffset, GLPOLYGONOFFSET)\
    X(glDepthMask, GLDEPTHMASK)\
    X(glDepthFunc, GLDEPTHFUNC)\
    X(glColorMask, GLCOLORMASK)\
    X(glBlendFuncSeparate, GLBLENDFUNCSEPARATE)\
    X(glBlendEquationSeparate, GLBLENDEQUATIONSEPARATE)\
    X(glDrawArraysInstanced, GLDRAWARRAYSINSTANCED)\
    X(glDrawElementsInstancedBaseVertex, GLDRAWELEMENTSINSTANCEDBASEVERTEX)\
    X(glMultiDrawArraysIndirect, GLMULTIDRAWARRAYSINDIRECT)\
    X(glMultiDrawElementsIndirect, GLMULTIDRAWELEMENTSINDIRECT)\
    X(glDispatchCompute, GLDISPATCHCOMPUTE)\
    X(glGenVertexArrays, GLGENVERTEXARRAYS)\
    X(glDeleteVertexArrays, GLDELETEVERTEXARRAYS)\
    X(glBindVertexArray, GLBINDVERTEXARRAY)\
    X(glGenBuffers, GLGENBUFFERS)\
    X(glDeleteBuffers, GLDELETEBUFFERS)\
    X(glBindBuffer, GLBINDBUFFER)\
    X(glBufferStorage, GLBUFFERSTORAGE)\
    X(glMapBufferRange, GLMAPBUFFERRANGE)\
    X(glFlushMappedBufferRange, GLFLUSHMAPPEDBUFFERRANGE)\
    X(glUnmapBuffer, GLUNMAPBUFFER)\
    X(glInvalidateBufferData, GLINVALIDATEBUFFERDATA)\
    X(glGenTextures, GLGENTEXTURES)\
    X(glDeleteTextures, GLDELETETEXTURES)\
    X(glBindTexture, GLBINDTEXTURE)\
    X(glTexStorage2D, GLTEXSTORAGE2D)\
    X(glTexStorage3D, GLTEXSTORAGE3D)\
    X(glTexSubImage2D, GLTEXSUBIMAGE2D)\
    X(glTexSubImage3D, GLTEXSUBIMAGE3D)\
    X(glGenFramebuffers, GLGENFRAMEBUFFERS)\
    X(glDeleteFramebuffers, GLDELETEFRAMEBUFFERS)\
    X(glBindFramebuffer, GLBINDFRAMEBUFFER)\
    X(glFramebufferTexture2D, GLFRAMEBUFFERTEXTURE2D)\
    X(glFramebufferTextureLayer, GLFRAMEBUFFERTEXTURELAYER)\
    X(glCheckFramebufferStatus, GLCHECKFRAMEBUFFERSTATUS)\
    X(glDrawBuffers, GLDRAWBUFFERS)\
    X(glCreateProgram, GLCREATEPROGRAM)\
    X(glDeleteProgram, GLDELETEPROGRAM)\
    X(glUseProgram, GLUSEPROGRAM)\
    X(glClearBufferiv, GLCLEARBUFFERIV)\
    X(glClearBufferuiv, GLCLEARBUFFERUIV)\
    X(glClearBufferfv, GLCLEARBUFFERFV)\
    X(glClearBufferfi, GLCLEARBUFFERFI)\

#define AGPU_GL_DECLARE(fn, upper) static PFN##upper##PROC fn;
#define AGPU_GL_LOAD(fn, upper) fn = (PFN##upper##PROC) config->gl_get_proc_address(#fn);

AGPU_GL_FOREACH(AGPU_GL_DECLARE);

#define _AGPU_GL_THROW(s) if (gl.config.callback) { gl.config.callback(gl.config.context, s, true); }
#define _AGPU_GL_CHECK(c, s) if (!(c)) { _AGPU_GL_THROW(s); }
#define _AGPU_GL_CHECK_ERROR() do { GLenum r = glGetError(); _AGPU_GL_CHECK(r == GL_NO_ERROR, _agpu_gl_get_error_string(r)); } while (0)

static const char* _agpu_gl_get_error_string(GLenum result);

static struct {
    agpu_config config;

    GLuint default_framebuffer;
    GLuint default_vao;
} gl;

/* Renderer functions */
static bool agpu_gl_init(const agpu_config* config) {
    memcpy(&gl.config, config, sizeof(*config));

    AGPU_GL_FOREACH(AGPU_GL_LOAD);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    _AGPU_GL_CHECK_ERROR();

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl.default_framebuffer);
    _AGPU_GL_CHECK_ERROR();

    glGenVertexArrays(1, &gl.default_vao);
    glBindVertexArray(gl.default_vao);
    _AGPU_GL_CHECK_ERROR();
    return true;
}

static void agpu_gl_shutdown(void) {
    glDeleteVertexArrays(1, &gl.default_vao);
    _AGPU_GL_CHECK_ERROR();
    memset(&gl, 0, sizeof(gl));
}

static void agpu_gl_frame_begin(void) {
    float clearColor[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);
    _AGPU_GL_CHECK_ERROR();
}

static void agpu_gl_frame_end(void) {

}

static const char* _agpu_gl_get_error_string(GLenum result) {
    switch (result) {
    case GL_INVALID_ENUM: return "Invalid enum";
    case GL_INVALID_VALUE: return "Invalid value";
    case GL_INVALID_OPERATION: return "Invalid operation";
    case GL_OUT_OF_MEMORY: return "Out of memory";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "Invalid framebuffer operation";
    default: return NULL;
    }
}

/* Driver functions */
static bool agpu_gl_is_supported(void) {
    return true;
}

static agpu_renderer* agpu_gl_init_renderer(void) {
    static agpu_renderer renderer = { 0 };
    renderer.init = agpu_gl_init;
    renderer.shutdown = agpu_gl_shutdown;
    renderer.frame_begin = agpu_gl_frame_begin;
    renderer.frame_end = agpu_gl_frame_end;
    return &renderer;
}

agpu_driver gl_driver = {
    AGPU_BACKEND_OPENGL,
    agpu_gl_is_supported,
    agpu_gl_init_renderer
};

#endif /* defined(AGPU_DRIVER_OPENGL) */
