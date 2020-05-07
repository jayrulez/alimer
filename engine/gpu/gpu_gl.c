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

typedef enum {
    GL_BUFFER_TARGET_COPY_SRC,  /* GL_COPY_READ_BUFFER */
    GL_BUFFER_TARGET_COPY_DST,  /* GL_COPY_WRITE_BUFFER */
    GL_BUFFER_TARGET_UNIFORM,   /* GL_UNIFORM_BUFFER */
    GL_BUFFER_TARGET_STORAGE,   /* GL_SHADER_STORAGE_BUFFER */
    GL_BUFFER_TARGET_INDEX,     /* GL_ELEMENT_ARRAY_BUFFER */
    GL_BUFFER_TARGET_VERTEX,    /* GL_ARRAY_BUFFER */
    GL_BUFFER_TARGET_INDIRECT,  /* GL_DRAW_INDIRECT_BUFFER */
    _GL_BUFFER_TARGET_COUNT
} agpu_gl_buffer_target;

typedef struct agpu_buffer_gl {
    GLuint id;
} agpu_buffer_gl;

typedef struct agpu_texture_gl {
    GLuint id;
} agpu_texture_gl;

typedef struct agpu_shader_gl {
    GLuint id;
} agpu_shader_gl;

typedef struct {
    int8_t buffer_index;        /* -1 if attr is not enabled */
    GLuint shaderLocation;
    uint64_t offset;
    uint8_t size;
    GLenum type;
    GLboolean normalized;
    GLboolean integer;
    GLuint divisor;
} agpu_vertex_attribute_gl;

typedef struct {
    agpu_shader_gl* shader;
    GLenum primitive_type;
    GLenum index_type;
    uint32_t attribute_count;
    agpu_vertex_attribute_gl attributes[AGPU_MAX_VERTEX_ATTRIBUTES];
    GLsizei vertex_buffer_strides[AGPU_MAX_VERTEX_BUFFER_BINDINGS];
} agpu_pipeline_gl;

typedef struct {
    agpu_vertex_attribute_gl attribute;
    GLuint vertex_buffer;
} agpu_vertex_attribute_cache_gl;

static struct {
    agpu_config config;

    GLuint vao;
    GLuint default_framebuffer;

    agpu_limits limits;

    struct {
        uint32_t major;
        uint32_t minor;
    } version;

    struct {
        bool compute;
        bool buffer_storage;
        bool texture_storage;
        bool direct_state_access;
    } ext;

    struct {
        agpu_pipeline_gl* current_pipeline;
        GLuint program;
        GLuint buffers[_GL_BUFFER_TARGET_COUNT];
        uint32_t primitiveRestart;
        agpu_vertex_attribute_cache_gl attributes[AGPU_MAX_VERTEX_ATTRIBUTES];
        uint16_t enabled_locations;
        GLuint vertex_buffers[AGPU_MAX_VERTEX_BUFFER_BINDINGS];
        uint64_t vertex_buffer_offsets[AGPU_MAX_VERTEX_BUFFER_BINDINGS];
        struct {
            GLuint buffer;
            uint64_t offset;
        } index;
    } state;
} gl;

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

static GLenum _agpu_gl_get_buffer_target(agpu_gl_buffer_target target) {
    switch (target) {
    case GL_BUFFER_TARGET_COPY_SRC: return GL_COPY_READ_BUFFER;
    case GL_BUFFER_TARGET_COPY_DST: return GL_COPY_WRITE_BUFFER;
    case GL_BUFFER_TARGET_UNIFORM: return GL_UNIFORM_BUFFER;
    case GL_BUFFER_TARGET_INDEX: return GL_ELEMENT_ARRAY_BUFFER;
    case GL_BUFFER_TARGET_VERTEX: return GL_ARRAY_BUFFER;

#if !defined(AGPU_GLES)
    case GL_BUFFER_TARGET_STORAGE: return GL_SHADER_STORAGE_BUFFER;
    case GL_BUFFER_TARGET_INDIRECT: return GL_DRAW_INDIRECT_BUFFER;
#endif

    default: return GL_NONE;
    }
}

static void _agpu_gl_bind_buffer(agpu_gl_buffer_target target, GLuint buffer, bool force) {
    if (force || gl.state.buffers[target] != target) {
        GLenum gl_target = _agpu_gl_get_buffer_target(target);
        if (gl_target != GL_NONE) {
            GL_CHECK(glBindBuffer(gl_target, buffer));
        }

        gl.state.buffers[target] = buffer;
    }
}

static void _agpu_gl_use_program(uint32_t program) {
    if (gl.state.program != program) {
        gl.state.program = program;
        GL_CHECK(glUseProgram(program));
        /* TODO: Increate shader switches stats */
    }
}

static void _agpu_gl_reset_state_cache() {
    memset(&gl.state, 0, sizeof(gl.state));
    for (uint32_t i = 0; i < _GL_BUFFER_TARGET_COUNT; i++) {
        _agpu_gl_bind_buffer((agpu_gl_buffer_target)i, 0, true);
    }

    gl.state.enabled_locations = 0;
    for (uint32_t i = 0; i < gl.limits.max_vertex_attributes; i++) {
        gl.state.attributes[i].attribute.buffer_index = -1;
        gl.state.attributes[i].attribute.shaderLocation = (GLuint)-1;
        GL_CHECK(glDisableVertexAttribArray(i));
    }

    for (uint32_t i = 0; i < AGPU_MAX_VERTEX_BUFFER_BINDINGS; i++) {
        gl.state.vertex_buffers[i] = 0;
        gl.state.vertex_buffer_offsets[i] = 0;
    }

    gl.state.index.buffer = 0;
    gl.state.index.offset = 0;

    gl.state.current_pipeline = NULL;
    gl.state.program = 0;
    GL_CHECK(glUseProgram(0));

    /* depth-stencil state */
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0);

    /* blend state */
    glDisable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);

    /* rasterizer state */
    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glEnable(GL_DITHER);
    glDisable(GL_POLYGON_OFFSET_FILL);
#ifdef AGPU_GLES
    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
#elif defined(AGPU_GL)
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_PRIMITIVE_RESTART);
    gl.state.primitiveRestart = 0xffffffff;
    glPrimitiveRestartIndex(gl.state.primitiveRestart);
    _GPU_GL_CHECK_ERROR();
#endif

    _GPU_GL_CHECK_ERROR();
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

    gl.ext.compute = GLAD_GL_ES_VERSION_3_1 || GLAD_GL_ARB_compute_shader;
    gl.ext.buffer_storage = _agpu_gl_version(4, 2) || GLAD_GL_ARB_buffer_storage;
    gl.ext.texture_storage = _agpu_gl_version(4, 4) || GLAD_GL_ARB_texture_storage;
    gl.ext.direct_state_access = _agpu_gl_version(4, 5) || GLAD_GL_ARB_direct_state_access;
#else

#if defined(AGPU_WEBGL)
    gl.ext.texture_storage = true;
#else
    gl.ext.texture_storage = GLAD_GL_ARB_texture_storage;
#endif

#endif

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl.default_framebuffer);
    glGenVertexArrays(1, &gl.vao);
    glBindVertexArray(gl.vao);
    _GPU_GL_CHECK_ERROR();

    /* Init limits */
    _GPU_GL_CHECK_ERROR();
    GL_CHECK(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, (GLint*)&gl.limits.max_vertex_attributes));
    if (gl.limits.max_vertex_attributes > AGPU_MAX_VERTEX_ATTRIBUTES) {
        gl.limits.max_vertex_attributes = AGPU_MAX_VERTEX_ATTRIBUTES;
    }
    gl.limits.max_vertex_bindings = gl.limits.max_vertex_attributes;
    gl.limits.max_vertex_attribute_offset = AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
    gl.limits.max_vertex_binding_stride = AGPU_MAX_VERTEX_BUFFER_STRIDE;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&gl.limits.max_texture_size_2d);
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, (GLint*)&gl.limits.max_texture_size_3d);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, (GLint*)&gl.limits.max_texture_size_cube);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, (GLint*)&gl.limits.max_texture_array_layers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, (GLint*)&gl.limits.max_color_attachments);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, (GLint*)&gl.limits.max_uniform_buffer_size);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint*)&gl.limits.min_uniform_buffer_offset_alignment);
#if !defined(AGPU_WEBGL)
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, (GLint*)&gl.limits.max_storage_buffer_size);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, (GLint*)&gl.limits.min_storage_buffer_offset_alignment);
    if (GLAD_GL_EXT_texture_filter_anisotropic) {
        GLfloat attr = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &attr);
        gl.limits.max_sampler_anisotropy = (uint32_t)attr;
    }

    // Viewport
    glGetIntegerv(GL_MAX_VIEWPORTS, (GLint*)&gl.limits.max_viewports);

#if !defined(AGPU_GLES)
    glGetIntegerv(GL_MAX_PATCH_VERTICES, (GLint*)&gl.limits.max_tessellation_patch_size);
#endif

    float point_sizes[2];
    float line_width_range[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, point_sizes);
    glGetFloatv(GL_LINE_WIDTH_RANGE, line_width_range);

    // Compute
    if (gl.ext.compute)
    {
        glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, (GLint*)&gl.limits.max_compute_shared_memory_size);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, (GLint*)&gl.limits.max_compute_work_group_count_x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, (GLint*)&gl.limits.max_compute_work_group_count_y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, (GLint*)&gl.limits.max_compute_work_group_count_z);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, (GLint*)&gl.limits.max_compute_work_group_invocations);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, (GLint*)&gl.limits.max_compute_work_group_size_x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, (GLint*)&gl.limits.max_compute_work_group_size_y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, (GLint*)&gl.limits.max_compute_work_group_size_z);
        _GPU_GL_CHECK_ERROR();
    }

#else
    // WebGL
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, point_sizes);
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, line_width_range);

    gl.limits.max_sampler_anisotropy = 1;
#endif

    GLint maxViewportDims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
    gl.limits.max_viewport_width = (uint32_t)maxViewportDims[0];
    gl.limits.max_viewport_height = (uint32_t)maxViewportDims[1];
    gl.limits.point_size_range_min = point_sizes[0];
    gl.limits.point_size_range_max = point_sizes[1];
    gl.limits.line_width_range_min = line_width_range[0];
    gl.limits.line_width_range_max = line_width_range[1];
    _GPU_GL_CHECK_ERROR();

    /* Reset state cache. */
    _agpu_gl_reset_state_cache();

    return true;
}

static void gl_shutdown(void) {

}

static void gl_frame_wait(void) {
    float clearColor[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
    GL_CHECK(glClearBufferfv(GL_COLOR, 0, clearColor));
}

static void gl_frame_finish(void) {

}

static agpu_backend_type gl_query_backend(void) {
    return AGPU_BACKEND_TYPE_OPENGL;
}

static void gl_get_limits(agpu_limits* limits) {
    *limits = gl.limits;
}

static AGPUPixelFormat gl_get_default_depth_format(void) {
    return AGPUPixelFormat_Depth32Float;
}

static AGPUPixelFormat gl_get_default_depth_stencil_format(void) {
    return AGPUPixelFormat_Depth24Plus;
}

/* Buffer */
static agpu_buffer gl_create_buffer(const agpu_buffer_info* info) {
    agpu_buffer_gl* buffer = _AGPU_ALLOC_HANDLE(agpu_buffer_gl);
    const bool readable = false;
    const bool dynamic = false;

    _GPU_GL_CHECK_ERROR();
#ifdef AGPU_GL
    if (gl.ext.direct_state_access)
    {
        glCreateBuffers(1, &buffer->id);
        GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
        if (readable) {
            flags |= GL_MAP_READ_BIT;
        }
        if (dynamic) {
            flags |= GL_DYNAMIC_STORAGE_BIT;
        }

        glNamedBufferStorage(buffer->id, info->size, info->content, flags);
    }
    else
#endif 
    {
        glGenBuffers(1, &buffer->id);
        _agpu_gl_bind_buffer(GL_BUFFER_TARGET_COPY_DST, buffer->id, false);
#if defined(AGPU_WEBGL)
        GL_CHECK(glBufferData(GL_BUFFER_TARGET_COPY_DST, info->size, info->content, GL_STATIC_DRAW));
#else

        GLenum gl_target = _agpu_gl_get_buffer_target(GL_BUFFER_TARGET_COPY_DST);
        if (gl.ext.buffer_storage) {
            /* GL_BUFFER_TARGET_COPY_SRC doesnt work with write bit */
            GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | (readable ? GL_MAP_READ_BIT : 0);
            glBufferStorage(gl_target, info->size, info->content, flags);
            //buffer->data = glMapBufferRange(glType, 0, size, flags | GL_MAP_FLUSH_EXPLICIT_BIT);
        }
        else {
            glBufferData(gl_target, info->size, info->content, GL_STATIC_DRAW);
        }
#endif
    }

    _GPU_GL_CHECK_ERROR();
    return (agpu_buffer)buffer;
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

    // Attribute cache
    GLint attribute_count;
    GL_CHECK(glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attribute_count));
    for (GLint i = 0; i < attribute_count; i++) {
        char name[64];
        GLint size;
        GLenum type;
        GLsizei length;
        glGetActiveAttrib(program, i, 64, &length, &size, &type, name);
        GLint location = glGetAttribLocation(program, name);
    }

    //_agpu_gl_use_program(program);
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

static GLenum _agpu_gl_index_type(AGPUIndexFormat format) {
    static const GLenum types[] = {
        [AGPUIndexFormat_Uint16] = GL_UNSIGNED_SHORT,
        [AGPUIndexFormat_Uint32] = GL_UNSIGNED_INT,
    };
    return types[format];
}

static GLenum _agpu_gl_vertex_format_type(AGPUVertexFormat format) {
    switch (format) {
    case AGPU_VERTEX_FORMAT_UCHAR2:
    case AGPU_VERTEX_FORMAT_UCHAR4:
    case AGPU_VERTEX_FORMAT_UCHAR2NORM:
    case AGPU_VERTEX_FORMAT_UCHAR4NORM:
        return GL_UNSIGNED_BYTE;
    case AGPU_VERTEX_FORMAT_CHAR2:
    case AGPU_VERTEX_FORMAT_CHAR4:
    case AGPU_VERTEX_FORMAT_CHAR2NORM:
    case AGPU_VERTEX_FORMAT_CHAR4NORM:
        return GL_BYTE;
    case AGPU_VERTEX_FORMAT_USHORT2:
    case AGPU_VERTEX_FORMAT_USHORT4:
    case AGPU_VERTEX_FORMAT_USHORT2NORM:
    case AGPU_VERTEX_FORMAT_USHORT4NORM:
        return GL_UNSIGNED_SHORT;
    case AGPU_VERTEX_FORMAT_SHORT2:
    case AGPU_VERTEX_FORMAT_SHORT4:
    case AGPU_VERTEX_FORMAT_SHORT2NORM:
    case AGPU_VERTEX_FORMAT_SHORT4NORM:
        return GL_SHORT;
    case AGPUVertexFormat_Half2:
    case AGPUVertexFormat_Half4:
        return GL_HALF_FLOAT;
    case AGPUVertexFormat_Float:
    case AGPUVertexFormat_Float2:
    case AGPUVertexFormat_Float3:
    case AGPUVertexFormat_Float4:
        return GL_FLOAT;
    case AGPUVertexFormat_UInt:
    case AGPUVertexFormat_UInt2:
    case AGPUVertexFormat_UInt3:
    case AGPUVertexFormat_UInt4:
        return GL_UNSIGNED_INT;
    case AGPUVertexFormat_Int:
    case AGPUVertexFormat_Int2:
    case AGPUVertexFormat_Int3:
    case AGPUVertexFormat_Int4:
        return GL_INT;
    default:
        AGPU_UNREACHABLE();
    }
}

static GLboolean _agpu_gl_vertex_format_normalized(AGPUVertexFormat format) {
    switch (format) {
    case AGPU_VERTEX_FORMAT_UCHAR2NORM:
    case AGPU_VERTEX_FORMAT_UCHAR4NORM:
    case AGPU_VERTEX_FORMAT_CHAR2NORM:
    case AGPU_VERTEX_FORMAT_CHAR4NORM:
    case AGPU_VERTEX_FORMAT_USHORT2NORM:
    case AGPU_VERTEX_FORMAT_USHORT4NORM:
    case AGPU_VERTEX_FORMAT_SHORT2NORM:
    case AGPU_VERTEX_FORMAT_SHORT4NORM:
        return GL_TRUE;
    default:
        return GL_FALSE;
    }
}

static GLboolean _agpu_gl_vertex_format_integer(AGPUVertexFormat format) {
    switch (format) {
    case AGPU_VERTEX_FORMAT_UCHAR2:
    case AGPU_VERTEX_FORMAT_UCHAR4:
    case AGPU_VERTEX_FORMAT_CHAR2:
    case AGPU_VERTEX_FORMAT_CHAR4:
    case AGPU_VERTEX_FORMAT_USHORT2:
    case AGPU_VERTEX_FORMAT_USHORT4:
    case AGPU_VERTEX_FORMAT_SHORT2:
    case AGPU_VERTEX_FORMAT_SHORT4:
    case AGPUVertexFormat_UInt:
    case AGPUVertexFormat_UInt2:
    case AGPUVertexFormat_UInt3:
    case AGPUVertexFormat_UInt4:
    case AGPUVertexFormat_Int:
    case AGPUVertexFormat_Int2:
    case AGPUVertexFormat_Int3:
    case AGPUVertexFormat_Int4:
        return true;
    default:
        return false;
    }
}

static agpu_pipeline gl_create_render_pipeline(const agpu_render_pipeline_info* info) {
    agpu_pipeline_gl* result = _AGPU_ALLOC_HANDLE(agpu_pipeline_gl);
    result->shader = (agpu_shader_gl*)info->shader;
    result->primitive_type = get_gl_primitive_type(info->primitive_topology);
    result->index_type = _agpu_gl_index_type(info->vertexState.indexFormat);
    result->attribute_count = 0;

    /* Setup vertex attributes */
    memset(result->vertex_buffer_strides, 0, sizeof(result->vertex_buffer_strides));
    for (uint32_t bufferIndex = 0; bufferIndex < info->vertexState.vertexBufferCount; bufferIndex++) {
        const AGPUVertexBufferLayoutDescriptor* layoutDesc = &info->vertexState.vertexBuffers[bufferIndex];

        for (uint32_t attr_index = 0; attr_index < layoutDesc->attributeCount; attr_index++) {
            const AGPUVertexAttributeDescriptor* attrDesc = &layoutDesc->attributes[attr_index];
            agpu_vertex_attribute_gl* gl_attr = &result->attributes[result->attribute_count++];
            gl_attr->buffer_index = (int8_t)bufferIndex;
            gl_attr->shaderLocation = attrDesc->shaderLocation;
            gl_attr->offset = result->vertex_buffer_strides[bufferIndex];
            gl_attr->size = agpuGetVertexFormatComponentsCount(attrDesc->format);
            gl_attr->type = _agpu_gl_vertex_format_type(attrDesc->format);
            gl_attr->normalized = _agpu_gl_vertex_format_normalized(attrDesc->format);
            gl_attr->integer = _agpu_gl_vertex_format_integer(attrDesc->format);
            if (layoutDesc->stepMode == AGPUInputStepMode_Vertex) {
                gl_attr->divisor = 0;
            }
            else {
                gl_attr->divisor = 1;
            }

            /* Increase stride */
            result->vertex_buffer_strides[bufferIndex] += agpuGetVertexFormatSize(attrDesc->format);
        }
    }

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
    if (gl.state.current_pipeline != pipeline) {
        _agpu_gl_use_program(pipeline->shader->id);
        gl.state.current_pipeline = pipeline;
    }
}

static void gl_cmdSetVertexBuffer(uint32_t slot, agpu_buffer buffer, uint64_t offset)
{
    gl.state.vertex_buffers[slot] = ((agpu_buffer_gl*)buffer)->id;
    gl.state.vertex_buffer_offsets[slot] = offset;
}

static void gl_cmdSetIndexBuffer(agpu_buffer buffer, uint64_t offset)
{
    gl.state.index.buffer = ((agpu_buffer_gl*)buffer)->id;
    gl.state.index.offset = offset;
}

static void _agpu_gl_prepare_draw() {
    if (gl.state.index.buffer != 0) {
        _agpu_gl_bind_buffer(GL_BUFFER_TARGET_INDEX, gl.state.index.buffer, false);
    }

    uint16_t current_enable_locations = 0;
    for (uint32_t i = 0; i < gl.state.current_pipeline->attribute_count; i++) {
        agpu_vertex_attribute_gl* gl_attr = &gl.state.current_pipeline->attributes[i];
        agpu_vertex_attribute_cache_gl* cache_attr = &gl.state.attributes[i];

        GLuint gl_vb = 0;
        uint64_t offset = 0;
        GLsizei vb_stride = 0;

        bool cache_attr_dirty = false;
        if (gl_attr->buffer_index >= 0) {
            gl_vb = gl.state.vertex_buffers[gl_attr->buffer_index];
            offset = gl.state.vertex_buffer_offsets[gl_attr->buffer_index] + gl_attr->offset;
            vb_stride = gl.state.current_pipeline->vertex_buffer_strides[gl_attr->buffer_index];
            current_enable_locations |= (1 << i);

            if ((gl_vb != cache_attr->vertex_buffer) ||
                (gl_attr->shaderLocation != cache_attr->attribute.shaderLocation) ||
                (gl_attr->size != cache_attr->attribute.size) ||
                (gl_attr->type != cache_attr->attribute.type) ||
                (gl_attr->normalized != cache_attr->attribute.normalized) ||
                (gl_attr->integer != cache_attr->attribute.integer) ||
                (gl_attr->divisor != cache_attr->attribute.divisor) ||
                (offset != cache_attr->attribute.offset))
            {
                _agpu_gl_bind_buffer(GL_BUFFER_TARGET_VERTEX, gl.state.vertex_buffers[gl_attr->buffer_index], false);
                if (gl_attr->integer) {
                    glVertexAttribIPointer(
                        gl_attr->shaderLocation,
                        gl_attr->size,
                        gl_attr->type,
                        vb_stride,
                        (const GLvoid*)(GLintptr)offset);
                }
                else {
                    glVertexAttribPointer(
                        gl_attr->shaderLocation,
                        gl_attr->size,
                        gl_attr->type,
                        gl_attr->normalized,
                        vb_stride,
                        (const GLvoid*)(GLintptr)offset);
                }

                glVertexAttribDivisor(gl_attr->shaderLocation, gl_attr->divisor);

                cache_attr_dirty = true;
            }
        }

        if (cache_attr_dirty) {
            cache_attr->attribute = *gl_attr;
            //cache_attr->attribute.offset = offset;
            cache_attr->vertex_buffer = gl_vb;
        }
    }

    uint16_t diff = current_enable_locations ^ gl.state.enabled_locations;
    if (diff != 0) {
        for (uint32_t i = 0; i < gl.limits.max_vertex_attributes; i++) {
            if (diff & (1 << i)) {
                if (current_enable_locations & (1 << i)) {
                    glEnableVertexAttribArray(i);
                }
                else {
                    glDisableVertexAttribArray(i);
                }
            }
        }

        gl.state.enabled_locations = current_enable_locations;
    }
}

static void gl_cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) {

    _agpu_gl_prepare_draw();

    if (instanceCount > 1) {
        glDrawArraysInstanced(gl.state.current_pipeline->primitive_type, firstVertex, vertexCount, instanceCount);
    }
    else {
        glDrawArrays(gl.state.current_pipeline->primitive_type, firstVertex, vertexCount);
    }
    _GPU_GL_CHECK_ERROR();
}

static void gl_cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex) {
    _agpu_gl_prepare_draw();

    GLenum glIndexType = gl.state.current_pipeline->index_type;
    const int i_size = (glIndexType == GL_UNSIGNED_SHORT) ? 2 : 4;
    const GLvoid* indices = (const GLvoid*)(GLintptr)(firstIndex * i_size + gl.state.index.offset);

    if (instanceCount > 1) {
        glDrawElementsInstanced(gl.state.current_pipeline->primitive_type, indexCount, glIndexType, indices, instanceCount);
    }
    else {
        glDrawElements(gl.state.current_pipeline->primitive_type, indexCount, glIndexType, indices);
    }
    _GPU_GL_CHECK_ERROR();
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
