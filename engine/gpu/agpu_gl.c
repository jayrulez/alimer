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

#include "agpu_internal.h"

#if defined(AGPU_BACKEND_GL)
#if AGPU_GLES
#   include "GLES/gl.h"
#   include "GLES2/gl2.h"
#   include "GLES2/gl2ext.h"
#   include "GLES3/gl3.h"
#   include "GLES3/gl31.h"
#else
#   include "GL/glcorearb.h"
#   include "GL/glext.h"
#endif

#define GL_FOREACH(X)\
    X(glGetError, GETERROR)\
    X(glGetString, GETSTRING)\
    X(glGetIntegerv, GETINTEGERV)\
    X(glEnable, ENABLE)\
    X(glDisable, DISABLE)\
    X(glClear, CLEAR)\
    X(glClearColor, CLEARCOLOR)\
    X(glVertexBindingDivisor, VERTEXBINDINGDIVISOR)\
    X(glVertexAttribBinding, VERTEXATTRIBBINDING)\
    X(glVertexAttribFormat, VERTEXATTRIBFORMAT)\
    X(glVertexAttribIFormat, VERTEXATTRIBIFORMAT)\
    X(glBindVertexBuffer, BINDVERTEXBUFFER)\
    X(glCullFace, CULLFACE)\
    X(glFrontFace, FRONTFACE)\
    X(glPolygonOffset, POLYGONOFFSET)\
    X(glDepthMask, DEPTHMASK)\
    X(glDepthFunc, DEPTHFUNC)\
    X(glColorMask, COLORMASK)\
    X(glBlendFuncSeparate, BLENDFUNCSEPARATE)\
    X(glBlendEquationSeparate, BLENDEQUATIONSEPARATE)\
    X(glDrawArraysInstanced, DRAWARRAYSINSTANCED)\
    X(glDrawElementsInstancedBaseVertex, DRAWELEMENTSINSTANCEDBASEVERTEX)\
    X(glMultiDrawArraysIndirect, MULTIDRAWARRAYSINDIRECT)\
    X(glMultiDrawElementsIndirect, MULTIDRAWELEMENTSINDIRECT)\
    X(glDispatchCompute, DISPATCHCOMPUTE)\
    X(glGenVertexArrays, GENVERTEXARRAYS)\
    X(glDeleteVertexArrays, DELETEVERTEXARRAYS)\
    X(glBindVertexArray, BINDVERTEXARRAY)\
    X(glGenBuffers, GENBUFFERS)\
    X(glDeleteBuffers, DELETEBUFFERS)\
    X(glBindBuffer, BINDBUFFER)\
    X(glBufferStorage, BUFFERSTORAGE)\
    X(glMapBufferRange, MAPBUFFERRANGE)\
    X(glFlushMappedBufferRange, FLUSHMAPPEDBUFFERRANGE)\
    X(glUnmapBuffer, UNMAPBUFFER)\
    X(glInvalidateBufferData, INVALIDATEBUFFERDATA)\
    X(glGenTextures, GENTEXTURES)\
    X(glDeleteTextures, DELETETEXTURES)\
    X(glBindTexture, BINDTEXTURE)\
    X(glTexStorage2D, TEXSTORAGE2D)\
    X(glTexStorage3D, TEXSTORAGE3D)\
    X(glTexSubImage2D, TEXSUBIMAGE2D)\
    X(glTexSubImage3D, TEXSUBIMAGE3D)\
    X(glGenFramebuffers, GENFRAMEBUFFERS)\
    X(glDeleteFramebuffers, DELETEFRAMEBUFFERS)\
    X(glBindFramebuffer, BINDFRAMEBUFFER)\
    X(glFramebufferTexture2D, FRAMEBUFFERTEXTURE2D)\
    X(glFramebufferTextureLayer, FRAMEBUFFERTEXTURELAYER)\
    X(glCheckFramebufferStatus, CHECKFRAMEBUFFERSTATUS)\
    X(glDrawBuffers, DRAWBUFFERS)\
    X(glCreateProgram, CREATEPROGRAM)\
    X(glDeleteProgram, DELETEPROGRAM)\
    X(glUseProgram, USEPROGRAM)\

#define GL_DECLARE(fn, upper) static PFNGL##upper##PROC fn;
#define GL_LOAD(fn, upper) fn = (PFNGL##upper##PROC) config->get_gl_proc_address(#fn);

GL_FOREACH(GL_DECLARE);

#define GL_THROW(str) if (gl_state.config.callback) { gl_state.config.callback(gl_state.config.context, str, true); }
#define GL_CHECK(c, str) if (!(c)) { GL_THROW(str); }

#if defined(NDEBUG) || (defined(__APPLE__) && !defined(DEBUG))
#define GL_ASSERT( gl_code ) gl_code
#else
#define GL_ASSERT( gl_code ) do \
    { \
        gl_code; \
        gl_state.error_code = glGetError(); \
        GL_CHECK(gl_state.error_code == GL_NO_ERROR, "GL error"); \
    } while(0)
#endif

static struct {
    GLenum error_code;
    agpu_config config;
    GLuint vao;
} gl_state;

static agpu_backend gl_get_backend(void) {
    return AGPU_BACKEND_OPENGL;
}

static bool gl_backend_initialize(const agpu_config* config) {
    GL_FOREACH(GL_LOAD);
    gl_state.error_code = GL_NO_ERROR;
    gl_state.config = *config;
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    GL_ASSERT(glGenVertexArrays(1, &gl_state.vao));
    GL_ASSERT(glBindVertexArray(gl_state.vao));
    return true;
}

static void gl_backend_shutdown(void) {
    if (gl_state.vao) {
        GL_ASSERT(glDeleteVertexArrays(1, &gl_state.vao));
    }
}

static void gl_backend_wait_idle(void) {

}

static void gl_backend_begin_frame(void) {
}

static void gl_backend_end_frame(void) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool agpu_gl_supported(void) {
    return true;
}

agpu_renderer* agpu_create_gl_backend(void) {
    static agpu_renderer renderer = { nullptr };
    renderer.get_backend = gl_get_backend;
    renderer.initialize = gl_backend_initialize;
    renderer.shutdown = gl_backend_shutdown;
    renderer.wait_idle = gl_backend_wait_idle;
    renderer.begin_frame = gl_backend_begin_frame;
    renderer.end_frame = gl_backend_end_frame;
    return &renderer;
}

#endif /* defined(AGPU_BACKEND_GL) */
