//
// Copyright (c) 2019-2020 Amer Koleci.
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

#if defined(__EMSCRIPTEN__) 
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl32.h>
#elif defined(__ANDROID__)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#elif _WIN32
#pragma comment(lib, "opengl32.lib")
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB  0x20A9

#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_FLAGS_ARB             0x2094

#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT   0x00000004
#endif

#if !defined(__EMSCRIPTEN__)
// OpenGL Value Types
typedef ptrdiff_t		GLintptr;
typedef ptrdiff_t		GLsizeiptr;
typedef unsigned int	GLenum;
typedef unsigned char	GLboolean;
typedef unsigned int	GLbitfield;
typedef void			GLvoid;
typedef signed char		GLbyte;		/* 1-byte signed */
typedef short			GLshort;	/* 2-byte signed */
typedef int				GLint;		/* 4-byte signed */
typedef unsigned char	GLubyte;	/* 1-byte unsigned */
typedef unsigned short	GLushort;	/* 2-byte unsigned */
typedef unsigned int	GLuint;		/* 4-byte unsigned */
typedef int				GLsizei;	/* 4-byte signed */
typedef float			GLfloat;	/* single precision float */
typedef float			GLclampf;	/* single precision float in [0,1] */
typedef double			GLdouble;	/* double precision float */
typedef double			GLclampd;	/* double precision float in [0,1] */
typedef char			GLchar;

// OpenGL Constants
#define GL_NO_ERROR 0
#define GL_DONT_CARE 0x1100
#define GL_ZERO 0x0000
#define GL_ONE 0x0001
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_HALF_FLOAT 0x140B
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_UNSIGNED_SHORT_5_5_5_1_REV 0x8366
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_DEPTH_BUFFER_BIT 0x0100
#define GL_STENCIL_BUFFER_BIT 0x0400
#define GL_SCISSOR_TEST 0x0C11
#define GL_DEPTH_TEST 0x0B71
#define GL_STENCIL_TEST 0x0B90
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_CULL_FACE 0x0B44
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_BLEND 0x0BE2
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_CONSTANT_COLOR 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_SRC1_ALPHA 0x8589
#define GL_SRC1_COLOR 0x88F9
#define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#define GL_ONE_MINUS_SRC1_ALPHA 0x88FB
#define GL_MIN 0x8007
#define GL_MAX 0x8008
#define GL_FUNC_ADD 0x8006
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207
#define GL_INVERT 0x150A
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_INCR 0x1E02
#define GL_DECR 0x1E03
#define GL_INCR_WRAP 0x8507
#define GL_DECR_WRAP 0x8508
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MIRRORED_REPEAT 0x8370
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_RED 0x1903
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_LUMINANCE 0x1909
#define GL_RGB8 0x8051
#define GL_RGBA8 0x8058
#define GL_RGBA4 0x8056
#define GL_RGB5_A1 0x8057
#define GL_RGB10_A2_EXT 0x8059
#define GL_RGBA16 0x805B
#define GL_BGRA 0x80E1
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_RG 0x8227
#define GL_RG8 0x822B
#define GL_RG16 0x822C
#define GL_R16F 0x822D
#define GL_R32F 0x822E
#define GL_RG16F 0x822F
#define GL_RG32F 0x8230
#define GL_RGBA32F 0x8814
#define GL_RGBA16F 0x881A
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_STENCIL 0x84F9
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_TEXTURE_LOD_BIAS 0x8501
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_TEXTURE0 0x84C0
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_RENDERBUFFER 0x8D41
#define GL_MAX_DRAW_BUFFERS 0x8824
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_SAMPLES_PASSED 0x8914
#define GL_MULTISAMPLE 0x809D
#define GL_MAX_SAMPLES 0x8D57
#define GL_SAMPLE_MASK 0x8E51
#define GL_DELETE_STATUS 0x8B80
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_VALIDATE_STATUS 0x8B83
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_ATTACHED_SHADERS 0x8B85
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#define GL_FLOAT_VEC2 0x8B50
#define GL_FLOAT_VEC3 0x8B51
#define GL_FLOAT_VEC4 0x8B52
#define GL_SAMPLER_2D 0x8B5E
#define GL_FLOAT_MAT3x2 0x8B67
#define GL_FLOAT_MAT4 0x8B5C
#define GL_NUM_EXTENSIONS 0x821D
#define GL_DEBUG_SOURCE_API 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY 0x8249
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#define GL_DEBUG_SOURCE_OTHER 0x824B
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_TYPE_PUSH_GROUP 0x8269
#define GL_DEBUG_TYPE_POP_GROUP 0x826A
#define GL_DEBUG_TYPE_MARKER 0x8268
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_RENDERBUFFER_BINDING   0x8CA7
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#define GL_COPY_READ_BUFFER 0x8F36
#define GL_COPY_WRITE_BUFFER 0x8F37
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_UNIFORM_BUFFER_BINDING 0x8A28
#define GL_UNIFORM_BUFFER_START 0x8A29
#define GL_UNIFORM_BUFFER_SIZE 0x8A2A

#define GL_COMPUTE_SHADER 0x91B9
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING 0x90D3
#define GL_SHADER_STORAGE_BUFFER_START 0x90D4
#define GL_SHADER_STORAGE_BUFFER_SIZE 0x90D5

// OpenGL Functions
#define GL_FUNCTIONS \
    GL_FUNC(GetString, const GLubyte*, GLenum name)\
    GL_FUNC(GetError, GLenum, void) \
    GL_FUNC(GetIntegerv, void, GLenum name, GLint* data) \
    GL_FUNC(Flush, void, void) \
    GL_FUNC(Enable, void, GLenum mode) \
    GL_FUNC(Disable, void, GLenum mode) \
    GL_FUNC(Clear, void, GLenum mask) \
	GL_FUNC(ClearColor, void, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) \
	GL_FUNC(ClearDepth, void, GLdouble depth) \
	GL_FUNC(ClearStencil, void, GLint stencil) \
    GL_FUNC(ColorMask, void, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) \
	GL_FUNC(DepthFunc, void, GLenum func)\
	GL_FUNC(DepthMask, void, GLboolean enabled) \
    GL_FUNC(Viewport, void, GLint x, GLint y, GLint width, GLint height) \
	GL_FUNC(Scissor, void, GLint x, GLint y, GLint width, GLint height) \
    GL_FUNC(CullFace, void, GLenum mode) \
    GL_FUNC(GenBuffers, void, GLint n, GLuint* arrays) \
	GL_FUNC(BindBuffer, void, GLenum target, GLuint buffer) \
	GL_FUNC(BufferData, void, GLenum target, GLsizeiptr size, const void* data, GLenum usage) \
	GL_FUNC(BufferSubData, void, GLenum target, GLintptr offset, GLsizeiptr size, const void* data) \
	GL_FUNC(DeleteBuffers, void, GLint n, GLuint* buffers) \
    GL_FUNC(CreateShader, GLuint, GLenum type) \
	GL_FUNC(AttachShader, void, GLuint program, GLuint shader) \
	GL_FUNC(DetachShader, void, GLuint program, GLuint shader) \
	GL_FUNC(DeleteShader, void, GLuint shader) \
	GL_FUNC(ShaderSource, void, GLuint shader, GLsizei count, const GLchar** string, const GLint* length) \
	GL_FUNC(CompileShader, void, GLuint shader) \
	GL_FUNC(GetShaderiv, void, GLuint shader, GLenum pname, GLint* result) \
	GL_FUNC(GetShaderInfoLog, void, GLuint shader, GLint maxLength, GLsizei* length, GLchar* infoLog) \
    GL_FUNC(CreateProgram, GLuint, ) \
	GL_FUNC(DeleteProgram, void, GLuint program) \
	GL_FUNC(LinkProgram, void, GLuint program) \
	GL_FUNC(GetProgramiv, void, GLuint program, GLenum pname, GLint* result) \
	GL_FUNC(GetProgramInfoLog, void, GLuint program, GLint maxLength, GLsizei* length, GLchar* infoLog) \
	GL_FUNC(GetActiveUniform, void, GLuint program, GLuint index, GLint bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) \
	GL_FUNC(GetActiveAttrib, void, GLuint program, GLuint index, GLint bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) \
	GL_FUNC(UseProgram, void, GLuint program) \
    GL_FUNC(GenVertexArrays, void, GLint n, GLuint* arrays) \
    GL_FUNC(DeleteVertexArrays, void, GLint n, GLuint* arrays) \
	GL_FUNC(BindVertexArray, void, GLuint id) \
    GL_FUNC(EnableVertexAttribArray, void, GLuint location) \
	GL_FUNC(DisableVertexAttribArray, void, GLuint location) \
	GL_FUNC(VertexAttribPointer, void, GLuint index, GLint size, GLenum type, GLboolean normalized, GLint stride, const void* pointer) \
	GL_FUNC(VertexAttribDivisor, void, GLuint index, GLuint divisor) \
    GL_FUNC(DrawArrays, void, GLenum mode, GLint first, GLsizei count) \
    GL_FUNC(DrawArraysInstanced, void, GLenum mode, GLint first, GLsizei count, GLsizei instancecount) \
    GL_FUNC(DrawElements, void, GLenum mode, GLint count, GLenum type, void* indices) \
	GL_FUNC(DrawElementsInstanced, void, GLenum mode, GLint count, GLenum type, void* indices, GLint amount) \

#endif /* !defined(__EMSCRIPTEN__) */

#include <spirv_glsl.hpp>

typedef enum agpu_gl_profile_type
{
    agpu_gl_profile_type_core = 0,
    agpu_gl_profile_type_compatibility,
    agpu_gl_profile_type_es
} agpu_gl_profile_type;

typedef struct agpu_gl_version {
    int major;
    int minor;
    agpu_gl_profile_type profile_type;
} agpu_gl_version;

struct gl_buffer {
    GLuint id;
    GLenum gl_type;
};

struct gl_shader {
    GLuint program;
};

struct gl_pipeline {
    gl_shader* shader;
    GLenum primitive_type;
};

struct gl_state {
    gl_pipeline* current_pipeline;
};

/* Global data */
static struct {
    bool available_initialized;
    bool available;

    agpu_config config;
    agpu_caps caps;

#ifdef __EMSCRIPTEN__
#elif __ANDROID__
    EGLDisplay egl_display;
    EGLSurface egl_surface;
    EGLContext egl_context;
    EGLConfig  egl_config;
#elif _WIN32
    HWND  hwnd;
    HDC   hdc;
    HGLRC context;
#endif

    agpu_gl_version version;
    uint32_t width;
    uint32_t height;

    GLuint default_framebuffer;
    GLuint default_vao;

    gl_state state;
} gl;

#if !defined(__EMSCRIPTEN__)
#define GL_FUNC(name, ret, ...) typedef ret (*name ## Func) (__VA_ARGS__); name ## Func gl##name;
GL_FUNCTIONS
#undef GL_FUNC
#endif

#define _AGPU_GL_CHECK_ERROR() { AGPU_ASSERT(glGetError() == GL_NO_ERROR); }

#if defined(_WIN32)
LRESULT CALLBACK _agpu_wgl_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(window, message, wParam, lParam);
}

static bool _agpu_wgl_init(const agpu_swapchain_info* info)
{
    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    if (!hInstance)
        return false;

    WNDCLASSW wc;
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = _agpu_wgl_window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = nullptr;
    wc.hCursor = nullptr;
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"DummyGLWindow";
    if (!RegisterClassW(&wc))
        return false;

    HWND dummy_window = CreateWindowExW(0, wc.lpszClassName, L"Dummy GL Window", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, wc.hInstance, 0);
    if (!dummy_window)
        return false;

    HDC dummy_dc = GetDC(dummy_window);
    if (!dummy_dc)
        return false;

    PIXELFORMATDESCRIPTOR format_desc = { sizeof(PIXELFORMATDESCRIPTOR) };
    format_desc.nVersion = 1;
    format_desc.iPixelType = PFD_TYPE_RGBA;
    format_desc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    format_desc.cColorBits = 32;
    format_desc.cAlphaBits = 8;
    format_desc.iLayerType = PFD_MAIN_PLANE;
    format_desc.cDepthBits = 24;
    format_desc.cStencilBits = 8;

    int pixel_format = ChoosePixelFormat(dummy_dc, &format_desc);
    if (!pixel_format) {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Wgl: Failed to find a suitable pixel format.");
        return false;
    }

    if (!SetPixelFormat(dummy_dc, pixel_format, &format_desc)) {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Wgl: Failed to set the pixel format.");
        return false;
    }

    /* Create dummy context. */
    HGLRC dummy_context = wglCreateContext(dummy_dc);
    if (!dummy_context) {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Wgl:Failed to create a dummy OpenGL rendering context.");
        return false;
    }

    if (!wglMakeCurrent(dummy_dc, dummy_context)) {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Wgl:Failed to activate dummy OpenGL rendering context.");
        return false;
    }

    typedef BOOL(WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
    typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int* attribList);
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatProc = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsProc = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));

    /* Destroy all dummy stuff */
    wglMakeCurrent(dummy_dc, nullptr);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_dc);
    DestroyWindow(dummy_window);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    gl.hwnd = (HWND)info->window_handle;
    gl.hdc = GetDC(gl.hwnd);

    RECT bounds;
    GetWindowRect(gl.hwnd, &bounds);
    gl.width = (uint32_t)(bounds.right - bounds.left);
    gl.height = (uint32_t)(bounds.bottom - bounds.top);

    const int attribute_list[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, true,
        WGL_SUPPORT_OPENGL_ARB, true,
        WGL_DOUBLE_BUFFER_ARB, true,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24, // TODO: Use depth_stencil_format
        WGL_STENCIL_BITS_ARB, 8, // TODO: Use depth_stencil_format
        WGL_SAMPLE_BUFFERS_ARB, info->sample_count > 0 ? 1 : 0,
        WGL_SAMPLES_ARB, static_cast<int>(info->sample_count),
        //WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, settings.srgb ? 1 : 0,
        0,
    };

    pixel_format = 0;
    UINT num_formats;
    if (!wglChoosePixelFormatProc(gl.hdc, attribute_list, nullptr, 1, &pixel_format, &num_formats))
    {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Wgl: Failed to choose pixel format");
        return false;
    }

    memset(&format_desc, 0, sizeof(format_desc));
    DescribePixelFormat(gl.hdc, pixel_format, sizeof(format_desc), &format_desc);
    if (!SetPixelFormat(gl.hdc, pixel_format, &format_desc))
    {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Wgl: Failed to set pixel format");
        return false;
    }

    agpu_gl_version versions[] =
    {
        {4, 6, agpu_gl_profile_type_core},
        {4, 5, agpu_gl_profile_type_core},
        {4, 4, agpu_gl_profile_type_core},
        {4, 3, agpu_gl_profile_type_core},
        {4, 2, agpu_gl_profile_type_core},
        {4, 1, agpu_gl_profile_type_core},
        {4, 0, agpu_gl_profile_type_core},
        {3, 3, agpu_gl_profile_type_core},
        // GLES
        {3, 2, agpu_gl_profile_type_es},
        {3, 1, agpu_gl_profile_type_es},
        {3, 0, agpu_gl_profile_type_es},
        {2, 0, agpu_gl_profile_type_es},
    };

    for (auto version : versions)
    {
        int profile = 0;
        switch (version.profile_type)
        {
        case agpu_gl_profile_type_core:
            profile = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
            break;
        case agpu_gl_profile_type_compatibility:
            profile = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
            break;
        case agpu_gl_profile_type_es:
            profile = WGL_CONTEXT_ES2_PROFILE_BIT_EXT;
            break;

        default:
            break;
        }

        const int context_attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, version.major,
            WGL_CONTEXT_MINOR_VERSION_ARB, version.minor,
            WGL_CONTEXT_FLAGS_ARB, gl.config.debug ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
            WGL_CONTEXT_PROFILE_MASK_ARB, profile,
            0
        };

        gl.context = wglCreateContextAttribsProc(gl.hdc, 0, context_attribs);

        if (gl.context)
        {
            gl.version = version;
            break;
        }
    }

    if (!wglMakeCurrent(gl.hdc, gl.context))
    {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Wgl: Failed to set OpenGL rendering context");
        return false;
    }

    return true;
}

void* gl_get_proc_address(const char* function)
{
    static HMODULE opengl_dll = LoadLibraryA("opengl32.dll");
    void* func = (void*)wglGetProcAddress(function);

    if (func == 0 || (func == (void*)0x1) || (func == (void*)0x2) || (func == (void*)0x3) || (func == (void*)-1)) {
        func = (void*)GetProcAddress(opengl_dll, function);
    }

    return func;
}

void gl_swap_buffers(void)
{
    if (!SwapBuffers(gl.hdc))
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Wgl: Failed to swap buffers");
}

#endif

/* Device/Renderer */
static bool gl_init(const char* app_name, const agpu_config* config)
{
    memcpy(&gl.config, config, sizeof(gl.config));

    bool context_init_result = false;
#if defined(_WIN32)
    context_init_result = _agpu_wgl_init(&config->swapchain_info);
#endif
    if (!context_init_result)
        return false;

    // Load opengl functions
#if !defined(__EMSCRIPTEN__)
#define GL_FUNC(name, ...) gl##name = (name##Func)(gl_get_proc_address("gl"#name));
    GL_FUNCTIONS
#undef GL_FUNC
#endif

        /* Log some info */
        agpu_log(AGPU_LOG_LEVEL_INFO, "AGPU driver: OpenGL");
    agpu_log(AGPU_LOG_LEVEL_INFO, "OpenGL Renderer: %s", (char*)glGetString(GL_RENDERER));
    agpu_log(AGPU_LOG_LEVEL_INFO, "OpenGL Driver: %s", (char*)glGetString(GL_VERSION));
    agpu_log(AGPU_LOG_LEVEL_INFO, "OpenGL Vendor: %s", (char*)glGetString(GL_VENDOR));

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl.default_framebuffer);
    _AGPU_GL_CHECK_ERROR();
    glGenVertexArrays(1, &gl.default_vao);
    glBindVertexArray(gl.default_vao);
    _AGPU_GL_CHECK_ERROR();
    return true;
}

static void gl_shutdown(void)
{
    glDeleteVertexArrays(1, &gl.default_vao);
    _AGPU_GL_CHECK_ERROR();
    memset(&gl, 0, sizeof(gl));
}

static bool gl_frame_begin(void)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    return true;
}

static void gl_frame_finish(void)
{
    gl_swap_buffers();
}

static void gl_query_caps(agpu_caps* caps) {
    *caps = gl.caps;
}

/* Buffer */
static GLenum _agpu_gl_buffer_type(agpu_buffer_type type)
{
    switch (type) {
    case AGPU_BUFFER_TYPE_VERTEX:
        return GL_ARRAY_BUFFER;
    case AGPU_BUFFER_TYPE_INDEX:
        return GL_ELEMENT_ARRAY_BUFFER;
    case AGPU_BUFFER_TYPE_UNIFORM:
        return GL_UNIFORM_BUFFER;
        //case AGPU_BUFFER_TYPE_SHADER_STORAGE: return GL_SHADER_STORAGE_BUFFER;
        //case AGPU_BUFFER_TYPE_GENERIC: return GL_COPY_WRITE_BUFFER;
    default:
        AGPU_UNREACHABLE();
        return GL_ARRAY_BUFFER;
    }
}


static GLenum _agpu_gl_buffer_usage(agpu_buffer_usage usage) {
    switch (usage) {
    case AGPU_BUFFER_USAGE_IMMUTABLE:
        return GL_STATIC_DRAW;
    case AGPU_BUFFER_USAGE_DYNAMIC:
        return GL_DYNAMIC_DRAW;
    case AGPU_BUFFER_USAGE_STREAM:
        return GL_STREAM_DRAW;
    default:
        AGPU_UNREACHABLE();
        return GL_STATIC_DRAW;
    }
}

static agpu_buffer gl_buffer_create(const agpu_buffer_info* info)
{
    gl_buffer* buffer = new gl_buffer();
    buffer->gl_type = _agpu_gl_buffer_type(info->type);

    glGenBuffers(1, &buffer->id);
    glBindBuffer(buffer->gl_type, buffer->id);
    glBufferData(buffer->gl_type, info->size, info->data, _agpu_gl_buffer_usage(info->usage));
    _AGPU_GL_CHECK_ERROR();
    return (agpu_buffer)buffer;
}

static void gl_buffer_destroy(agpu_buffer handle)
{
}

/* Shader */
static GLuint _agpu_gl_compile_shader(GLenum type, const char** sources, int* lengths, GLsizei count) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, count, sources, lengths);
    glCompileShader(shader);

    int isShaderCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
    if (!isShaderCompiled) {
        int logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char* log = (char*)malloc(logLength);
        AGPU_ASSERT(log);
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        const char* name;
        switch (type) {
        case GL_VERTEX_SHADER: name = "vertex shader"; break;
        case GL_FRAGMENT_SHADER: name = "fragment shader"; break;
        case GL_COMPUTE_SHADER: name = "compute shader"; break;
        default: name = "shader"; break;
        }
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Could not compile %s:\n%s", name, log);
        free(log);
    }

    return shader;
}

static GLuint _agpu_gl_compile_spirv(GLenum type, const agpu_shader_source* shader_source)
{
    spirv_cross::CompilerGLSL::Options options;
    // The range of Z-coordinate in the clipping volume of OpenGL is [-w, w], while it is
    // [0, w] in D3D12, Metal and Vulkan, so we should normalize it in shaders in all
    // backends. See the documentation of
    // spirv_cross::CompilerGLSL::Options::vertex::fixup_clipspace for more details.
    //options.vertex.flip_vert_y = true;
    options.vertex.fixup_clipspace = true;
    options.version = 450;

    spirv_cross::CompilerGLSL compiler(static_cast<const uint32_t*>(shader_source->code), shader_source->size / 4);
    compiler.set_common_options(options);
    //compiler.set_entry_point("main", spv::ExecutionModelVertex);
    compiler.build_combined_image_samplers();

    std::string glsl_code = compiler.compile();
    const char* glsl_code_str = glsl_code.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &glsl_code_str, nullptr);
    glCompileShader(shader);

    int isShaderCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
    if (!isShaderCompiled) {
        int logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char* log = (char*)malloc(logLength);
        AGPU_ASSERT(log);
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        const char* name;
        switch (type) {
        case GL_VERTEX_SHADER: name = "vertex shader"; break;
        case GL_FRAGMENT_SHADER: name = "fragment shader"; break;
        case GL_COMPUTE_SHADER: name = "compute shader"; break;
        default: name = "shader"; break;
        }
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Could not compile %s:\n%s", name, log);
        free(log);
    }

    return shader;
}

static bool _agpu_gl_link_program(GLuint program) {
    glLinkProgram(program);

    int is_linked;
    glGetProgramiv(program, GL_LINK_STATUS, &is_linked);
    if (!is_linked) {
        int log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        char* log = (char*)malloc(log_length);
        AGPU_ASSERT(log);
        glGetProgramInfoLog(program, log_length, &log_length, log);
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Could not link shader:\n%s", log);
        free(log);
        glDeleteProgram(program);
        return false;
    }

    return true;
}

static agpu_shader gl_shader_create(const agpu_shader_info* info)
{
    GLuint vertex_shader = _agpu_gl_compile_spirv(GL_VERTEX_SHADER, &info->vertex);
    _AGPU_GL_CHECK_ERROR();
    GLuint fragment_shader = _agpu_gl_compile_spirv(GL_FRAGMENT_SHADER, &info->fragment);
    _AGPU_GL_CHECK_ERROR();

    // Link
    uint32_t program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    //glBindAttribLocation(program, AGPU_SHADER_POSITION, "agpu_position");
    bool link_status = _agpu_gl_link_program(program);
    glDetachShader(program, vertex_shader);
    glDeleteShader(vertex_shader);
    glDetachShader(program, fragment_shader);
    glDeleteShader(fragment_shader);
    if (!link_status) {
        return nullptr;
    }

    _AGPU_GL_CHECK_ERROR();
    gl_shader* shader = new gl_shader();
    shader->program = program;

    return (agpu_shader)shader;
}

static void gl_shader_destroy(agpu_shader handle)
{

}

/* Texture */
static agpu_texture gl_texture_create(const agpu_texture_info* info)
{
    return nullptr;
}

static void gl_texture_destroy(agpu_texture handle)
{
}

/* Pipeline */
static agpu_pipeline gl_pipeline_create(const agpu_pipeline_info* info)
{
    gl_pipeline* pipeline = new gl_pipeline();
    pipeline->shader = (gl_shader*)info->shader;
    pipeline->primitive_type = GL_TRIANGLES;
    return (agpu_pipeline)pipeline;
}

static void gl_pipeline_destroy(agpu_pipeline handle)
{
}

/* Commands */
static void gl_push_debug_group(const char* name)
{
}

static void gl_pop_debug_group(void)
{
}

static void gl_insert_debug_marker(const char* name)
{
}

static void gl_begin_render_pass(const agpu_render_pass_info* info)
{

}

static void gl_end_render_pass(void)
{
}

static void gl_bind_pipeline(agpu_pipeline handle)
{
    gl_pipeline* pipeline = (gl_pipeline*)handle;
    if (gl.state.current_pipeline == pipeline)
        return;

    gl.state.current_pipeline = pipeline;
    glUseProgram(pipeline->shader->program);
    _AGPU_GL_CHECK_ERROR();
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    _AGPU_GL_CHECK_ERROR();
}

static void gl_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex)
{
    /* TODO: Add glDrawArraysInstancedBaseInstance ? */
    if (instance_count > 1)
    {
        glDrawArraysInstanced(gl.state.current_pipeline->primitive_type, first_vertex, vertex_count, instance_count);
    }
    else
    {
        glDrawArrays(gl.state.current_pipeline->primitive_type, first_vertex, vertex_count);
    }
    _AGPU_GL_CHECK_ERROR();
}


/* Driver */
static bool gl_is_supported(void)
{
    if (gl.available_initialized) {
        return gl.available;
    }

    gl.available_initialized = true;
    gl.available = true;
    return true;
};

static agpu_renderer* gl_create_renderer(void)
{
    static agpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(gl);
    return &renderer;
}

agpu_driver GL_Driver = {
    AGPU_BACKEND_TYPE_OPENGL,
    gl_is_supported,
    gl_create_renderer
};

#endif /* defined(AGPU_DRIVER_OPENGL)  */
