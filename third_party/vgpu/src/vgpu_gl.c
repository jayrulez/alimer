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

#include "vgpu_backend.h"

#if defined(VGPU_BACKEND_OPENGL) 
#if  defined(__EMSCRIPTEN__)
#   define VGPU_WEBGL
#   include <GLES3/gl3.h>
#   include <GLES2/gl2ext.h>
#   include <GL/gl.h>
#   include <GL/glext.h>
#elif defined(__ANDROID__)
#   define VGPU_GLES
#   include <glad/glad.h>
#else
#   define VGPU_GL
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
} vgpu_gl_buffer_target;

typedef struct {
    uint32_t id;
    GLsizeiptr size;
    vgpu_gl_buffer_target target;
    void* data;
} vgpu_buffer_gl;

typedef struct {
    GLuint id;
} vgpu_texture_gl;

typedef struct {
    GLuint id;
} vgpu_sampler_gl;

typedef struct {
    GLuint id;
} vgpu_shader_gl;

typedef struct {
    int8_t buffer_index;        /* -1 if attr is not enabled */
    GLuint shaderLocation;
    GLsizei stride;
    uint64_t offset;
    uint8_t size;
    GLenum type;
    GLboolean normalized;
    GLboolean integer;
    GLuint divisor;
} vgpu_vertex_attribute_gl;

typedef struct {
    vgpu_shader_gl* shader;
    GLenum primitive_type;
    GLenum index_type;
    uint32_t attribute_count;
    vgpu_vertex_attribute_gl attributes[VGPU_MAX_VERTEX_ATTRIBUTES];
    //vgpu_depth_stencil_state depth_stencil;
} vgpu_pipeline_gl;

typedef struct {
    vgpu_vertex_attribute_gl attribute;
    GLuint vertex_buffer;
} vgpu_vertex_attribute_cache_gl;

typedef struct vgpu_gl_cache {
    bool insidePass;
    vgpu_pipeline_gl* current_pipeline;
    GLuint program;
    GLuint buffers[_GL_BUFFER_TARGET_COUNT];
    uint32_t primitiveRestart;
    vgpu_vertex_attribute_cache_gl attributes[VGPU_MAX_VERTEX_ATTRIBUTES];
    uint16_t enabled_locations;
    GLuint vertex_buffers[VGPU_MAX_VERTEX_BUFFER_BINDINGS];
    uint64_t vertex_buffer_offsets[VGPU_MAX_VERTEX_BUFFER_BINDINGS];
    struct {
        GLuint buffer;
        uint64_t offset;
    } index;
    vgpu_depth_stencil_state depth_stencil;
} vgpu_gl_cache;



static struct {
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

    vgpu_caps caps;

    GLuint vao;
    GLuint defaultFramebuffer;
    vgpu_gl_cache cache;
    vgpu_buffer_gl* ubo_buffer;
} gl;

#define GL_THROW(s) vgpu_log(VGPU_LOG_LEVEL_ERROR, s)
#define GL_CHECK_STR(c, s) if (!(c)) { GL_THROW(s); }

#if defined(NDEBUG) || (defined(__APPLE__) && !defined(DEBUG))
#define GL_CHECK( gl_code ) gl_code
#else
#define GL_CHECK( gl_code ) do \
    { \
        gl_code; \
        GLenum err = glGetError(); \
        GL_CHECK_STR(err == GL_NO_ERROR, _vgpu_gl_get_error_string(err)); \
    } while(0)
#endif

#define _GPU_GL_CHECK_ERROR() { VGPU_ASSERT(glGetError() == GL_NO_ERROR); }

static const char* _vgpu_gl_get_error_string(GLenum result) {
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

static bool _vgpu_gl_version(uint32_t major, uint32_t minor) {
    if (gl.version.major > major)
    {
        return true;
    }

    return gl.version.major == major && gl.version.minor >= minor;
}

/* Global conversion function */
static GLenum _vgpu_gl_compare_func(vgpu_compare_function cmp) {
    switch (cmp) {
    case VGPU_COMPARE_FUNCTION_NEVER:          return GL_NEVER;
    case VGPU_COMPARE_FUNCTION_LESS:           return GL_LESS;
    case VGPU_COMPARE_FUNCTION_LESS_EQUAL:     return GL_LEQUAL;
    case VGPU_COMPARE_FUNCTION_GREATER:        return GL_GREATER;
    case VGPU_COMPARE_FUNCTION_GREATER_EQUAL:  return GL_GEQUAL;
    case VGPU_COMPARE_FUNCTION_EQUAL:          return GL_EQUAL;
    case VGPU_COMPARE_FUNCTION_NOT_EQUAL:      return GL_NOTEQUAL;
    case VGPU_COMPARE_FUNCTION_ALWAYS:         return GL_ALWAYS;
    default: VGPU_UNREACHABLE();
    }
}

static GLenum _vgpu_gl_get_buffer_target(vgpu_gl_buffer_target target) {
    switch (target) {
    case GL_BUFFER_TARGET_COPY_SRC: return GL_COPY_READ_BUFFER;
    case GL_BUFFER_TARGET_COPY_DST: return GL_COPY_WRITE_BUFFER;
    case GL_BUFFER_TARGET_UNIFORM: return GL_UNIFORM_BUFFER;
    case GL_BUFFER_TARGET_INDEX: return GL_ELEMENT_ARRAY_BUFFER;
    case GL_BUFFER_TARGET_VERTEX: return GL_ARRAY_BUFFER;

#if !defined(VGPU_GLES)
    case GL_BUFFER_TARGET_STORAGE: return GL_SHADER_STORAGE_BUFFER;
    case GL_BUFFER_TARGET_INDIRECT: return GL_DRAW_INDIRECT_BUFFER;
#endif

    default: return GL_NONE;
    }
}

static void _vgpu_gl_bind_buffer(vgpu_gl_buffer_target target, GLuint buffer, bool force) {
    if (force || gl.cache.buffers[target] != target) {
        GLenum gl_target = _vgpu_gl_get_buffer_target(target);
        if (gl_target != GL_NONE) {
            GL_CHECK(glBindBuffer(gl_target, buffer));
        }

        gl.cache.buffers[target] = buffer;
    }
}

/* Renderer functions */
static void _vgpu_gl_reset_state_cache(void) {
    memset(&gl.cache, 0, sizeof(gl.cache));
    gl.cache.insidePass = false;

    for (uint32_t i = 0; i < _GL_BUFFER_TARGET_COUNT; i++) {
        _vgpu_gl_bind_buffer((vgpu_gl_buffer_target)i, 0, true);
    }

    gl.cache.enabled_locations = 0;
    for (uint32_t i = 0; i < gl.caps.limits.max_vertex_attributes; i++)
    {
        gl.cache.attributes[i].attribute.buffer_index = -1;
        gl.cache.attributes[i].attribute.shaderLocation = (GLuint)-1;
        GL_CHECK(glDisableVertexAttribArray(i));
    }

    for (uint32_t i = 0; i < VGPU_MAX_VERTEX_BUFFER_BINDINGS; i++)
    {
        gl.cache.vertex_buffers[i] = 0;
        gl.cache.vertex_buffer_offsets[i] = 0;
    }

    gl.cache.index.buffer = 0;
    gl.cache.index.offset = 0;
    gl.cache.current_pipeline = NULL;
    gl.cache.program = 0;
    GL_CHECK(glUseProgram(0));

    /* depth-stencil state */
    gl.cache.depth_stencil.depth_compare = VGPU_COMPARE_FUNCTION_ALWAYS;
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
    renderer->cache.primitiveRestart = 0xffffffff;
    glPrimitiveRestartIndex(renderer->cache.primitiveRestart);
    _GPU_GL_CHECK_ERROR();
#endif

    _GPU_GL_CHECK_ERROR();
}

static vgpu_bool gl_init(const vgpu_config* info)
{
    if (!gl_driver.supported()) {
        return false;
    }

#ifdef VGPU_GL
    if (!gladLoadGLLoader((GLADloadproc)info->get_proc_address)) {
        return false;
    }
#elif defined(VGPU_GLES)
    gladLoadGLES2Loader((GLADloadproc)info->get_proc_address);
#endif

#ifdef VGPU_GL
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
    gl.ext.buffer_storage = _vgpu_gl_version(4, 2) || GLAD_GL_ARB_buffer_storage;
    gl.ext.texture_storage = _vgpu_gl_version(4, 4) || GLAD_GL_ARB_texture_storage;
    gl.ext.direct_state_access = _vgpu_gl_version(4, 5) || GLAD_GL_ARB_direct_state_access;
#else

#if defined(VGPU_WEBGL)
    gl.ext.texture_storage = true;
#else
    gl.ext.texture_storage = GLAD_GL_ARB_texture_storage;
#endif

#endif

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl.defaultFramebuffer);
    glGenVertexArrays(1, &gl.vao);
    glBindVertexArray(gl.vao);
    _GPU_GL_CHECK_ERROR();

    /* Init limits */
    _GPU_GL_CHECK_ERROR();

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&gl.caps.limits.max_texture_size_2d);
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, (GLint*)&gl.caps.limits.max_texture_size_3d);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, (GLint*)&gl.caps.limits.max_texture_size_cube);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, (GLint*)&gl.caps.limits.max_texture_array_layers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, (GLint*)&gl.caps.limits.max_color_attachments);

    GL_CHECK(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, (GLint*)&gl.caps.limits.max_vertex_attributes));
    if (gl.caps.limits.max_vertex_attributes > VGPU_MAX_VERTEX_ATTRIBUTES) {
        gl.caps.limits.max_vertex_attributes = VGPU_MAX_VERTEX_ATTRIBUTES;
    }
    gl.caps.limits.max_vertex_bindings = gl.caps.limits.max_vertex_attributes;
    gl.caps.limits.max_vertex_attribute_offset = VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
    gl.caps.limits.max_vertex_binding_stride = VGPU_MAX_VERTEX_BUFFER_STRIDE;


    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, (GLint*)&gl.caps.limits.max_uniform_buffer_size);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint*)&gl.caps.limits.min_uniform_buffer_offset_alignment);
#if !defined(VGPU_WEBGL)
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, (GLint*)&gl.caps.limits.max_storage_buffer_size);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, (GLint*)&gl.caps.limits.min_storage_buffer_offset_alignment);
    if (GLAD_GL_EXT_texture_filter_anisotropic) {
        GLfloat attr = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &attr);
        gl.caps.limits.max_sampler_anisotropy = attr;
    }

    // Viewport
    glGetIntegerv(GL_MAX_VIEWPORTS, (GLint*)&gl.caps.limits.max_viewports);

#if !defined(VGPU_GLES)
    glGetIntegerv(GL_MAX_PATCH_VERTICES, (GLint*)&gl.caps.limits.max_tessellation_patch_size);
#endif

    float point_sizes[2];
    float line_width_range[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, point_sizes);
    glGetFloatv(GL_LINE_WIDTH_RANGE, line_width_range);

    // Compute
    if (gl.ext.compute)
    {
        glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, (GLint*)&gl.caps.limits.max_compute_shared_memory_size);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, (GLint*)&gl.caps.limits.max_compute_work_group_count_x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, (GLint*)&gl.caps.limits.max_compute_work_group_count_y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, (GLint*)&gl.caps.limits.max_compute_work_group_count_z);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, (GLint*)&gl.caps.limits.max_compute_work_group_invocations);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, (GLint*)&gl.caps.limits.max_compute_work_group_size_x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, (GLint*)&gl.caps.limits.max_compute_work_group_size_y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, (GLint*)&gl.caps.limits.max_compute_work_group_size_z);
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
    gl.caps.limits.max_viewport_width = (uint32_t)maxViewportDims[0];
    gl.caps.limits.max_viewport_height = (uint32_t)maxViewportDims[1];
    gl.caps.limits.point_size_range_min = point_sizes[0];
    gl.caps.limits.point_size_range_max = point_sizes[1];
    gl.caps.limits.line_width_range_min = line_width_range[0];
    gl.caps.limits.line_width_range_max = line_width_range[1];
    _GPU_GL_CHECK_ERROR();

    /* Reset state cache. */
    _vgpu_gl_reset_state_cache();

    return true;
}

static void gl_destroy(void) {
    if (gl.vao) {
        glDeleteVertexArrays(1, &gl.vao);
    }
    _GPU_GL_CHECK_ERROR();
}

static vgpu_backend_type gl_get_backend(void) {
    return VGPU_BACKEND_TYPE_OPENGL;
}

static vgpu_caps gl_get_caps(void)
{
    return gl.caps;
}

static VGPUTextureFormat gl_GetDefaultDepthFormat(void)
{
    return VGPUTextureFormat_Depth32Float;
}

static VGPUTextureFormat gl_GetDefaultDepthStencilFormat(void)
{
    return VGPUTextureFormat_Depth24Plus;
}

static void gl_begin_frame(void) {
    float clearColor[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    const bool doDepthClear = true;
    if (doDepthClear) {
        glDepthMask(GL_TRUE);
    }

    if (doDepthClear)
    {
        float clearDepth = 1.0f;
        glClearBufferfv(GL_DEPTH, 0, &clearDepth);
    }

    _GPU_GL_CHECK_ERROR();
}

static void gl_end_frame(void)
{
}

/* Texture */
static vgpu_texture gl_create_texture(const VGPUTextureDescriptor* info)
{
    vgpu_texture_gl* texture = _VGPU_ALLOC_HANDLE(vgpu_texture_gl);

#ifdef VGPU_GL
    if (gl.ext.direct_state_access)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &texture->id);
        glTextureParameteri(texture->id, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glTextureParameteri(texture->id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture->id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(texture->id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(texture->id, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

        glTextureStorage2D(texture->id, 1, GL_RGBA8, info->size.width, info->size.height);
        glTextureSubImage2D(texture->id, 0, 0, 0, info->size.width, info->size.height, GL_RGBA, GL_UNSIGNED_BYTE, info->data);

        glBindTextureUnit(0, texture->id);
    }
    else
#endif 
    {
        glGenTextures(1, &texture->id);
        glBindTexture(GL_TEXTURE_2D, texture->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info->size.width, info->size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, info->data);
        glActiveTexture(GL_TEXTURE0);
    }

    _GPU_GL_CHECK_ERROR();

    return (vgpu_texture)texture;
}

static void gl_destroy_texture(vgpu_texture handle) {
    vgpu_texture_gl* texture = (vgpu_texture_gl*)handle;
    GL_CHECK(glDeleteTextures(1, &texture->id));
    VGPU_FREE(texture);
}

/* Buffer */
static GLenum _gl_GetBufferUsage(vgpu_buffer_usage usage)
{
    if (usage && VGPU_BUFFER_USAGE_DYNAMIC) {
        return GL_DYNAMIC_DRAW;
    }

    if (usage && VGPU_BUFFER_USAGE_CPU_ACCESSIBLE) {
        return GL_DYNAMIC_DRAW;
    }

    return GL_STATIC_DRAW;
}


#if !defined(VGPU_WEBGL)
static GLbitfield _gl_getBufferFlags(vgpu_buffer_usage usage)
{
    GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
    if (usage & VGPU_BUFFER_USAGE_DYNAMIC) {
        flags |= GL_DYNAMIC_STORAGE_BIT;
    }

    if (usage & VGPU_BUFFER_USAGE_CPU_ACCESSIBLE) {
        flags |= GL_MAP_READ_BIT;
    }

    return flags;
}
#endif

static vgpu_buffer gl_create_buffer(const vgpu_buffer_info* info)
{
    vgpu_buffer_gl* buffer = _VGPU_ALLOC_HANDLE(vgpu_buffer_gl);
    buffer->size = (GLsizeiptr)info->size;

    if (info->usage & VGPU_BUFFER_USAGE_VERTEX) buffer->target = GL_BUFFER_TARGET_VERTEX;
    else if (info->usage & VGPU_BUFFER_USAGE_INDEX) buffer->target = GL_BUFFER_TARGET_INDEX;
    else if (info->usage & VGPU_BUFFER_USAGE_UNIFORM) buffer->target = GL_BUFFER_TARGET_UNIFORM;
    else if (info->usage & VGPU_BUFFER_USAGE_STORAGE) buffer->target = GL_BUFFER_TARGET_STORAGE;
    else if (info->usage & VGPU_BUFFER_USAGE_INDIRECT) buffer->target = GL_BUFFER_TARGET_INDIRECT;
    else buffer->target = GL_BUFFER_TARGET_COPY_DST;

#ifdef VGPU_GL
    if (gl.ext.direct_state_access)
    {
        glCreateBuffers(1, &buffer->id);

        GLbitfield flags = _gl_getBufferFlags(info->usage);
        glNamedBufferStorage(buffer->id, info->size, info->data, flags);
    }
    else
#endif 
    {
        glGenBuffers(1, &buffer->id);
        _vgpu_gl_bind_buffer(buffer->target, buffer->id, false);

        GLenum gl_usage = _gl_GetBufferUsage(info->usage);

#if defined(AGPU_WEBGL)
        GL_CHECK(glBufferData(buffer->target, info->size, info->content, gl_usage));
#else
        GLenum gl_target = _vgpu_gl_get_buffer_target(buffer->target);
        if (gl.ext.buffer_storage) {
            /* GL_BUFFER_TARGET_COPY_SRC doesnt work with write bit */
            GLbitfield flags = _gl_getBufferFlags(info->usage);
            glBufferStorage(gl_target, info->size, info->data, flags);
            //buffer->data = glMapBufferRange(glType, 0, size, flags | GL_MAP_FLUSH_EXPLICIT_BIT);
        }
        else {
            glBufferData(gl_target, info->size, info->data, gl_usage);
        }
#endif
    }

    _GPU_GL_CHECK_ERROR();
    return (vgpu_buffer)buffer;
}

static void gl_destroy_buffer(vgpu_buffer handle) {
    vgpu_buffer_gl* buffer = (vgpu_buffer_gl*)handle;
    GL_CHECK(glDeleteBuffers(1, &buffer->id));
    VGPU_FREE(buffer);
}

/* Sampler */
GLenum _vgpu_gl_mag_filter(vgpu_filter filter) {
    switch (filter) {
    case VGPU_FILTER_NEAREST:
        return GL_NEAREST;
    case VGPU_FILTER_LINEAR:
        return GL_LINEAR;
    default:
        _VGPU_UNREACHABLE();
    }
}

GLenum _vgpu_gl_min_filter(vgpu_filter min_filter, vgpu_filter mipMap_filter) {
    switch (min_filter) {
    case VGPU_FILTER_NEAREST:
        switch (mipMap_filter) {
        case VGPU_FILTER_NEAREST:
            return GL_NEAREST_MIPMAP_NEAREST;
        case VGPU_FILTER_LINEAR:
            return GL_NEAREST_MIPMAP_LINEAR;
        default:
            _VGPU_UNREACHABLE();
        }
    case VGPU_FILTER_LINEAR:
        switch (mipMap_filter) {
        case VGPU_FILTER_NEAREST:
            return GL_LINEAR_MIPMAP_NEAREST;
        case VGPU_FILTER_LINEAR:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            _VGPU_UNREACHABLE();
        }
    default:
        _VGPU_UNREACHABLE();
    }
}

GLenum _vgpu_gl_address_mode(vgpu_address_mode mode) {
    switch (mode) {
    case VGPU_ADDRESS_MODE_CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;
    case VGPU_ADDRESS_MODE_REPEAT:
        return GL_REPEAT;
    case VGPU_ADDRESS_MODE_MIRROR_REPEAT:
        return GL_MIRRORED_REPEAT;
    //case VGPU_ADDRESS_MODE_CLAMP_TO_BORDER:
    //    return GL_CLAMP_TO_BORDER;

    default:
        _VGPU_UNREACHABLE();
    }
}


vgpu_sampler gl_create_sampler(const vgpu_sampler_info* info) {
    vgpu_sampler_gl* sampler = _VGPU_ALLOC_HANDLE(vgpu_sampler_gl);
#ifdef VGPU_GL
    if (gl.ext.direct_state_access)
    {
        glCreateSamplers(1, &sampler->id);
    }
    else
#endif 
    {
        glGenSamplers(1, &sampler->id);
    }

    glSamplerParameteri(sampler->id, GL_TEXTURE_MAG_FILTER, (GLint)_vgpu_gl_mag_filter(info->mag_filter));
    glSamplerParameteri(sampler->id, GL_TEXTURE_MIN_FILTER, (GLint)_vgpu_gl_min_filter(info->min_filter, info->mipmap_filter));

    glSamplerParameteri(sampler->id, GL_TEXTURE_WRAP_S, _vgpu_gl_address_mode(info->mode_u));
    glSamplerParameteri(sampler->id, GL_TEXTURE_WRAP_T, _vgpu_gl_address_mode(info->mode_v));
    glSamplerParameteri(sampler->id, GL_TEXTURE_WRAP_R, _vgpu_gl_address_mode(info->mode_w));
    glSamplerParameterf(sampler->id, GL_TEXTURE_MIN_LOD, info->lodMinClamp);
    glSamplerParameterf(sampler->id, GL_TEXTURE_MAX_LOD, info->lodMaxClamp);

    if (info->compare != VGPU_COMPARE_FUNCTION_UNDEFINED) {
        glSamplerParameteri(sampler->id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(sampler->id, GL_TEXTURE_COMPARE_FUNC, _vgpu_gl_compare_func(info->compare));
    }

    _GPU_GL_CHECK_ERROR();
    return (vgpu_sampler)sampler;
}

void gl_destroy_sampler(vgpu_sampler handle) {
    vgpu_sampler_gl* sampler = (vgpu_sampler_gl*)handle;
    GL_CHECK(glDeleteSamplers(1, &sampler->id));
    VGPU_FREE(sampler);
}

/* Shader */
static GLuint vgpu_gl_compile_shader(GLenum type, const char* source) {
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

static vgpu_shader gl_create_shader(const vgpu_shader_info* info) {
    GLuint vertex_shader = vgpu_gl_compile_shader(GL_VERTEX_SHADER, (const char*)info->vertex.source);
    GLuint fragment_shader = vgpu_gl_compile_shader(GL_FRAGMENT_SHADER, (const char*)info->fragment.source);
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
    vgpu_shader_gl* result = _VGPU_ALLOC_HANDLE(vgpu_shader_gl);
    result->id = program;
    return (vgpu_shader)result;
}

static void gl_destroy_shader(vgpu_shader handle) {
    vgpu_shader_gl* shader = (vgpu_shader_gl*)handle;
    GL_CHECK(glDeleteProgram(shader->id));
    VGPU_FREE(shader);
}

/* Driver functions */
static vgpu_bool gl_supported(void) {
    return true;
}

vgpu_renderer* gl_init_renderer(void) {
    static vgpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(gl);
    return &renderer;
}

vgpu_driver gl_driver = {
    gl_supported,
    gl_init_renderer
};

#endif /* defined(VGPU_BACKEND_OPENGL) */
