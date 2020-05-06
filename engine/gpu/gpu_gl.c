//
// Copyright (c) 2019 Amer Koleci.
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

#if defined(GPU_GL_BACKEND)

#include "gpu_backend.h"

#if  defined(__EMSCRIPTEN__)
#   define AGPU_WEBGL
#   include <GLES3/gl3.h>
#   include <GLES2/gl2ext.h>
#   include <GL/gl.h>
#   include <GL/glext.h>
#elif defined(__ANDROID__)
#   define AGPU_GLES
#   include <glad/glad.h>
#else
#   define AGPU_GL
#   include <glad/glad.h>
#endif

typedef struct agpu_pipeline_gl agpu_pipeline_gl;

static struct {
    agpu_config config;

    GLuint vao;
    GLuint default_framebuffer;

    AGPUDeviceCapabilities caps;

    struct {
        uint32_t major;
        uint32_t minor;
    } version;

    struct {
        bool direct_state_access;
    } ext;

    struct {
        agpu_pipeline_gl* current_pipeline;
        uint32_t program;
        uint32_t primitiveRestart;
    } state;
} gl;

typedef struct agpu_buffer_gl {
    GLuint id;
} agpu_buffer_gl;

typedef struct agpu_texture_gl {
    GLuint id;
} agpu_texture_gl;

typedef struct agpu_shader_gl {
    GLuint id;
} agpu_shader_gl;

typedef struct agpu_pipeline_gl {
    agpu_shader_gl* shader;
    GLenum primitive_type;
} agpu_pipeline_gl;

GL_FOREACH(GL_DECLARE);
#if !defined(AGPU_OPENGLES)
GL_FOREACH_DESKTOP(GL_DECLARE);
#endif

#define GL_THROW(s) if (gl.config.callback) { gl.config.callback(gl.config.context, s, true); }
#define GL_CHECK_STR(c, s) if (!(c)) { GL_THROW(s); }

#if defined(NDEBUG) || (defined(__APPLE__) && !defined(DEBUG))
#define GL_CHECK( gl_code ) gl_code
#else
#define GL_CHECK( gl_code ) do \
    { \
        gl_code; \
        GLenum err = glGetError(); \
        GL_CHECK_STR(err == GL_NO_ERROR, _agpu_gl_get_error_string(err)); \
    } while(0)
#endif

#define _GPU_GL_CHECK_ERROR() { AGPU_ASSERT(glGetError() == GL_NO_ERROR); }

static const char* _agpu_gl_get_error_string(GLenum result) {
    switch (result) {
    case GL_NO_ERROR: return "No error";
    case GL_INVALID_ENUM: return "Invalid enum";
    case GL_INVALID_VALUE: return "Invalid value";
    case GL_INVALID_OPERATION: return "Invalid operation";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "Invalid framebuffer operation";
    case GL_OUT_OF_MEMORY: return "Out of memory";
    case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
#if !defined(AGPU_OPENGLES)
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
#endif
    default: return NULL;
    }
}

static bool _agpu_gl_version(uint32_t major, uint32_t minor) {
    if (gl.version.major > major)
    {
        return true;
    }

    return gl.version.major == major && gl.version.minor >= minor;
}

static void _agpu_gl_use_program(uint32_t program) {
    if (gl.state.program != program) {
        gl.state.program = program;
        GL_CHECK(glUseProgram(program));
        /* TODO: Increate shader switches stats */
    }
}

static void _agpu_gl_reset_state_cache() {
    gl.state.current_pipeline = NULL;
    gl.state.program = 0;
    GL_CHECK(glUseProgram(0));
}

static bool gl_init(const agpu_config* config) {
    memcpy(&gl.config, config, sizeof(*config));

#ifdef AGPU_GL
    gladLoadGLLoader((GLADloadproc)config->gl.get_proc_address);
#elif defined(AGPU_GLES)
    gladLoadGLES2Loader((GLADloadproc)config->gl.get_proc_address);
#endif

#ifdef AGPU_GL
    GL_CHECK(glGetIntegerv(GL_MAJOR_VERSION, (GLint*)&gl.version.major));
    GL_CHECK(glGetIntegerv(GL_MINOR_VERSION, (GLint*)&gl.version.minor));

    GLint num_ext = 0;
    GL_CHECK(glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext));
    for (int i = 0; i < num_ext; i++) {
        const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (ext) {
            if (strstr(ext, "_ARB_direct_state_access")) {
                gl.ext.direct_state_access = true;
            }
        }
    }

    gl.ext.direct_state_access = _agpu_gl_version(4, 5) || GLAD_GL_ARB_direct_state_access || GLAD_GL_EXT_direct_state_access;

    GL_CHECK(glEnable(GL_LINE_SMOOTH));
    GL_CHECK(glEnable(GL_PROGRAM_POINT_SIZE));
    //glEnable(GL_FRAMEBUFFER_SRGB);
    GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
    GL_CHECK(glEnable(GL_PRIMITIVE_RESTART));
    GL_CHECK(glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX));
    gl.state.primitiveRestart = 0xffffffff;
    GL_CHECK(glPrimitiveRestartIndex(gl.state.primitiveRestart));
#else
#endif

    GL_CHECK(glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl.default_framebuffer));
    GL_CHECK(glGenVertexArrays(1, &gl.vao));
    GL_CHECK(glBindVertexArray(gl.vao));

    _agpu_gl_reset_state_cache();
    glViewport(0, 0, config->swapchain->width, config->swapchain->height);

    return true;
}

static void gl_shutdown(void) {

}

static void gl_frame_wait(void) {

}

static void gl_frame_finish(void) {
    float clearColor[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
    GL_CHECK(glClearBufferfv(GL_COLOR, 0, clearColor));
}

static agpu_backend_type gl_query_backend(void) {
    return AGPU_BACKEND_TYPE_OPENGL;
}

static void gl_query_caps(AGPUDeviceCapabilities* caps) {
    *caps = gl.caps;
}

static AGPUPixelFormat gl_get_default_depth_format(void) {
    return AGPUPixelFormat_Depth32Float;
}

static AGPUPixelFormat gl_get_default_depth_stencil_format(void) {
    return AGPUPixelFormat_Depth24Plus;
}

/* Buffer */
static agpu_buffer gl_create_buffer(const agpu_buffer_info* info) {
    agpu_buffer_gl* result;
    result = _AGPU_ALLOC_HANDLE(agpu_buffer_gl);
    const bool readable = false;
    const bool dynamic = false;

#ifdef AGPU_GL
    if (gl.ext.direct_state_access)
    {
        GL_CHECK(glCreateBuffers(1, &result->id));
        GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT  | GL_MAP_WRITE_BIT;
        if (readable) {
            flags |= GL_MAP_READ_BIT;
        }
        if (dynamic) {
            flags |= GL_DYNAMIC_STORAGE_BIT;
        }

        GL_CHECK(glNamedBufferStorage(result->id, info->size, info->content, flags));
    }
    else
#endif 
    {
        GL_CHECK(glGenBuffers(1, &result->id));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, result->id));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, info->size, info->content, GL_STATIC_DRAW));
    }

    return (agpu_buffer)result;
}

static void gl_destroy_buffer(agpu_buffer handle) {
    agpu_buffer_gl* buffer = (agpu_buffer_gl*)handle;
    GL_CHECK(glDeleteBuffers(1, &buffer->id));
    AGPU_FREE(buffer);
}

/* Texture */
static agpu_texture gl_create_texture(const agpu_texture_info* info)
{
    agpu_texture_gl* texture = NULL;
    return (agpu_texture)texture;
}

static void gl_destroy_texture(agpu_texture handle) {
    agpu_texture_gl* texture = (agpu_texture_gl*)handle;
    GL_CHECK(glDeleteTextures(1, &texture->id));
    AGPU_FREE(texture);
}

/* Shader */
static GLuint agpu_gl_compile_shader(GLenum type, const char* source) {
    _GPU_GL_CHECK_ERROR();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint compile_status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (!compile_status) {
        /* compilation failed, log error and delete shader */
        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            GLchar* log_msg = (GLchar*)malloc(log_length);
            glGetShaderInfoLog(shader, log_length, &log_length, log_msg);
            const char* name;
            switch (type) {
            case GL_VERTEX_SHADER: name = "vertex shader"; break;
            case GL_FRAGMENT_SHADER: name = "fragment shader"; break;
            case GL_COMPUTE_SHADER: name = "compute shader"; break;
            default: name = "shader"; break;
            }
            GL_THROW(log_msg);
            free(log_msg);
        }
        glDeleteShader(shader);
        shader = 0;
    }
    _GPU_GL_CHECK_ERROR();
    return shader;
}

static agpu_shader gl_create_shader(const agpu_shader_info* info) {
    GLuint vertex_shader = agpu_gl_compile_shader(GL_VERTEX_SHADER, (const char*)info->vertex.source);
    GLuint fragment_shader = agpu_gl_compile_shader(GL_FRAGMENT_SHADER, (const char*)info->fragment.source);
    if (!(vertex_shader && fragment_shader)) {
        return NULL;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    //glBindAttribLocation(program, AGPU_SHADER_POSITION, "alimerPosition");
    glLinkProgram(program);
    _GPU_GL_CHECK_ERROR();
    GLint link_status;
    GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &link_status));
    if (!link_status) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            GLchar* log_msg = (GLchar*)malloc(log_length);
            glGetProgramInfoLog(program, log_length, &log_length, log_msg);
            GL_THROW(log_msg);
            free(log_msg);
        }
        glDeleteProgram(program);
        return NULL;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    _GPU_GL_CHECK_ERROR();

    _agpu_gl_use_program(program);
    agpu_shader_gl* result = _AGPU_ALLOC_HANDLE(agpu_shader_gl);
    result->id = program;
    return (agpu_shader)result;
}

static void gl_destroy_shader(agpu_shader handle) {
    agpu_shader_gl* shader = (agpu_shader_gl*)handle;
    GL_CHECK(glDeleteProgram(shader->id));
    AGPU_FREE(shader);
}

/* Pipeline */
static GLenum get_gl_primitive_type(agpu_primitive_topology type) {
    static const GLenum types[] = {
        [AGPU_PRIMITIVE_TOPOLOGY_POINTS] = GL_POINTS,
        [AGPU_PRIMITIVE_TOPOLOGY_LINES] = GL_LINES,
        [AGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP] = GL_LINE_STRIP,
        [AGPU_PRIMITIVE_TOPOLOGY_TRIANGLES] = GL_TRIANGLES,
        [AGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP] = GL_TRIANGLE_STRIP
    };
    return types[type];
}

static agpu_pipeline gl_create_pipeline(const agpu_pipeline_info* info) {
    agpu_pipeline_gl* result = _AGPU_ALLOC_HANDLE(agpu_pipeline_gl);
    result->shader = (agpu_shader_gl*)info->shader;
    result->primitive_type = get_gl_primitive_type(info->topology);
    return (agpu_pipeline)result;
}

static void gl_destroy_pipeline(agpu_pipeline handle) {
    agpu_pipeline_gl* pipeline = (agpu_pipeline_gl*)handle;
    AGPU_FREE(pipeline);
}

/* CommandBuffer */
static void gl_set_pipeline(agpu_pipeline handle) {
    agpu_pipeline_gl* pipeline = (agpu_pipeline_gl*)handle;
    _GPU_GL_CHECK_ERROR();
    /*if (gl.state.current_pipeline != pipeline)*/ {
        _agpu_gl_use_program(pipeline->shader->id);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        gl.state.current_pipeline = pipeline;
    }
}

static void gl_set_vertex_buffers(uint32_t first_binding, uint32_t count, const agpu_buffer* buffers) {
    agpu_buffer_gl* gl_buffer = (agpu_buffer_gl*)buffers[0];
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, gl_buffer->id));
}

static void gl_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex) {
    if (instance_count > 1) {
        GL_CHECK(glDrawArraysInstanced(gl.state.current_pipeline->primitive_type, first_vertex, vertex_count, instance_count));
    }
    else {
        GL_CHECK(glDrawArrays(gl.state.current_pipeline->primitive_type, first_vertex, vertex_count));
    }
}

/* Driver functions */
static bool gl_supported(void) {
    return true;
}

static agpu_renderer* gl_create_renderer(void) {
    static agpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(gl);
    return &renderer;
}

agpu_driver gl_driver = {
    gl_supported,
    gl_create_renderer
};

#endif /* defined(GPU_GL_BACKEND) */
