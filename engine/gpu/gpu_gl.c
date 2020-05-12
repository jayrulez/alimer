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
    vgpu_depth_stencil_state depth_stencil;
} vgpu_pipeline_gl;

typedef struct {
    vgpu_vertex_attribute_gl attribute;
    GLuint vertex_buffer;
} vgpu_vertex_attribute_cache_gl;

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
} gl;


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

typedef struct VGPURendererGL {
    /* Associated vgpu_device */
    VGPUDevice gpuDevice;

    GLuint vao;
    GLuint defaultFramebuffer;

    VGPUDeviceCaps caps;
    vgpu_gl_cache cache;
    vgpu_buffer_gl* ubo_buffer;
} VGPURendererGL;

#define GL_THROW(s) vgpuLog(VGPULogLevel_Error, s)
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

static GLenum _agpu_gl_get_buffer_target(vgpu_gl_buffer_target target) {
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

static GLenum _gl_GetBufferUsage(VGPUBufferUsage usage)
{
    if (usage && VGPUBufferUsage_Dynamic) {
        return GL_DYNAMIC_DRAW;
    }

    if (usage && VGPUBufferUsage_CPUAccessible) {
        return GL_DYNAMIC_DRAW;
    }

    return GL_STATIC_DRAW;
}

#if !defined(AGPU_WEBGL)
static GLbitfield _gl_getBufferFlags(VGPUBufferUsage usage)
{
    GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
    if (usage & VGPUBufferUsage_Dynamic) {
        flags |= GL_DYNAMIC_STORAGE_BIT;
    }

    if (usage & VGPUBufferUsage_CPUAccessible) {
        flags |= GL_MAP_READ_BIT;
    }

    return flags;
}
#endif

static void _agpu_gl_bind_buffer(VGPURendererGL* renderer, vgpu_gl_buffer_target target, GLuint buffer, bool force) {
    if (force || renderer->cache.buffers[target] != target) {
        GLenum gl_target = _agpu_gl_get_buffer_target(target);
        if (gl_target != GL_NONE) {
            GL_CHECK(glBindBuffer(gl_target, buffer));
        }

        renderer->cache.buffers[target] = buffer;
    }
}

static void _agpu_gl_use_program(VGPURendererGL* renderer, uint32_t program) {
    if (renderer->cache.program != program) {
        renderer->cache.program = program;
        GL_CHECK(glUseProgram(program));
        /* TODO: Increate shader switches stats */
    }
}

static void _agpu_gl_reset_state_cache(VGPURendererGL* renderer) {
    memset(&renderer->cache, 0, sizeof(renderer->cache));
    renderer->cache.insidePass = false;

    for (uint32_t i = 0; i < _GL_BUFFER_TARGET_COUNT; i++) {
        _agpu_gl_bind_buffer(renderer, (vgpu_gl_buffer_target)i, 0, true);
    }

    renderer->cache.enabled_locations = 0;
    for (uint32_t i = 0; i < renderer->caps.limits.maxVertexInputAttributes; i++)
    {
        renderer->cache.attributes[i].attribute.buffer_index = -1;
        renderer->cache.attributes[i].attribute.shaderLocation = (GLuint)-1;
        GL_CHECK(glDisableVertexAttribArray(i));
    }

    for (uint32_t i = 0; i < VGPU_MAX_VERTEX_BUFFER_BINDINGS; i++)
    {
        renderer->cache.vertex_buffers[i] = 0;
        renderer->cache.vertex_buffer_offsets[i] = 0;
    }

    renderer->cache.index.buffer = 0;
    renderer->cache.index.offset = 0;
    renderer->cache.current_pipeline = NULL;
    renderer->cache.program = 0;
    GL_CHECK(glUseProgram(0));

    /* depth-stencil state */
    renderer->cache.depth_stencil.depth_compare = VGPU_COMPARE_FUNCTION_ALWAYS;
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

static bool gl_init(VGPUDevice device, const VGpuDeviceDescriptor* descriptor) {
#ifdef AGPU_GL
    gladLoadGLLoader((GLADloadproc)descriptor->gl.GetProcAddress);
#elif defined(AGPU_GLES)
    gladLoadGLES2Loader((GLADloadproc)descriptor->gl.GetProcAddress);
#endif

    VGPURendererGL* renderer = (VGPURendererGL*)device->renderer;

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

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&renderer->defaultFramebuffer);
    glGenVertexArrays(1, &renderer->vao);
    glBindVertexArray(renderer->vao);
    _GPU_GL_CHECK_ERROR();

    /* Init limits */
    _GPU_GL_CHECK_ERROR();

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&renderer->caps.limits.maxTextureDimension2D);
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, (GLint*)&renderer->caps.limits.maxTextureDimension3D);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, (GLint*)&renderer->caps.limits.maxTextureDimensionCube);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, (GLint*)&renderer->caps.limits.maxTextureArrayLayers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, (GLint*)&renderer->caps.limits.maxColorAttachments);

    GL_CHECK(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, (GLint*)&renderer->caps.limits.maxVertexInputAttributes));
    if (renderer->caps.limits.maxVertexInputAttributes > VGPU_MAX_VERTEX_ATTRIBUTES) {
        renderer->caps.limits.maxVertexInputAttributes = VGPU_MAX_VERTEX_ATTRIBUTES;
    }
    renderer->caps.limits.maxVertexInputBindings = renderer->caps.limits.maxVertexInputAttributes;
    renderer->caps.limits.maxVertexInputAttributeOffset = VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
    renderer->caps.limits.maxVertexInputBindingStride = VGPU_MAX_VERTEX_BUFFER_STRIDE;


    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, (GLint*)&renderer->caps.limits.max_uniform_buffer_size);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint*)&renderer->caps.limits.min_uniform_buffer_offset_alignment);
#if !defined(AGPU_WEBGL)
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, (GLint*)&renderer->caps.limits.max_storage_buffer_size);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, (GLint*)&renderer->caps.limits.min_storage_buffer_offset_alignment);
    if (GLAD_GL_EXT_texture_filter_anisotropic) {
        GLfloat attr = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &attr);
        renderer->caps.limits.max_sampler_anisotropy = (uint32_t)attr;
    }

    // Viewport
    glGetIntegerv(GL_MAX_VIEWPORTS, (GLint*)&renderer->caps.limits.maxViewports);

#if !defined(AGPU_GLES)
    glGetIntegerv(GL_MAX_PATCH_VERTICES, (GLint*)&renderer->caps.limits.maxTessellationPatchSize);
#endif

    float point_sizes[2];
    float line_width_range[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, point_sizes);
    glGetFloatv(GL_LINE_WIDTH_RANGE, line_width_range);

    // Compute
    if (gl.ext.compute)
    {
        glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, (GLint*)&renderer->caps.limits.maxComputeSharedMemorySize);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, (GLint*)&renderer->caps.limits.maxComputeWorkGroupCount[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, (GLint*)&renderer->caps.limits.maxComputeWorkGroupCount[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, (GLint*)&renderer->caps.limits.maxComputeWorkGroupCount[2]);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, (GLint*)&renderer->caps.limits.maxComputeWorkGroupInvocations);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, (GLint*)&renderer->caps.limits.maxComputeWorkGroupSize[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, (GLint*)&renderer->caps.limits.maxComputeWorkGroupSize[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, (GLint*)&renderer->caps.limits.maxComputeWorkGroupSize[2]);
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
    renderer->caps.limits.maxViewportDimensions[0] = (uint32_t)maxViewportDims[0];
    renderer->caps.limits.maxViewportDimensions[1] = (uint32_t)maxViewportDims[1];
    renderer->caps.limits.pointSizeRange[0] = point_sizes[0];
    renderer->caps.limits.pointSizeRange[1] = point_sizes[1];
    renderer->caps.limits.lineWidthRange[0] = line_width_range[0];
    renderer->caps.limits.lineWidthRange[1] = line_width_range[1];
    _GPU_GL_CHECK_ERROR();

    /* Reset state cache. */
    _agpu_gl_reset_state_cache(renderer);

    const vgpu_buffer_info ubo_buffer_info = {
        .size = 1024 * 1024, // 1 MB starting size
        .usage = VGPUBufferUsage_Uniform | VGPUBufferUsage_Dynamic
    };
    renderer->ubo_buffer = (vgpu_buffer_gl*)vgpu_buffer_create(&ubo_buffer_info);
#ifdef AGPU_WEBGL
    renderer->ubo_buffer->data = malloc(ubo_buffer_info->size);
#else

#ifdef AGPU_GL
    if (gl.ext.direct_state_access)
    {
        renderer->ubo_buffer->data = glMapNamedBufferRange(renderer->ubo_buffer->id, 0, ubo_buffer_info.size, GL_MAP_WRITE_BIT);
    }
    else
#endif 
    {
        renderer->ubo_buffer->data = glMapBufferRange(GL_UNIFORM_BUFFER, 0, ubo_buffer_info.size, GL_MAP_WRITE_BIT);
    }
#endif

    return true;
}

static void gl_destroy(VGPUDevice device) {
    VGPURendererGL* renderer = (VGPURendererGL*)device->renderer;
    if (renderer->vao) {
        glDeleteVertexArrays(1, &renderer->vao);
    }
    _GPU_GL_CHECK_ERROR();

    AGPU_FREE(renderer);
    AGPU_FREE(device);
}

static void gl_frame_wait(VGPURenderer* driverData) {
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

static void gl_frame_finish(VGPURenderer* driverData) {

}

static VGPUBackendType gl_getBackend(void) {
    return VGPUBackendType_OpenGL;
}

static const VGPUDeviceCaps* gl_get_caps(VGPURenderer* driverData) {
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    return &renderer->caps;
}

static AGPUPixelFormat gl_get_default_depth_format(VGPURenderer* driverData) {
    return AGPUPixelFormat_Depth32Float;
}

static AGPUPixelFormat gl_get_default_depth_stencil_format(VGPURenderer* driverData) {
    return AGPUPixelFormat_Depth24Plus;
}

/* Buffer */
static vgpu_buffer* gl_buffer_create(VGPURenderer* driverData, const vgpu_buffer_info* info) {
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    vgpu_buffer_gl* buffer = _AGPU_ALLOC_HANDLE(vgpu_buffer_gl);
    buffer->size = (GLsizeiptr)info->size;

    if (info->usage & VGPUBufferUsage_Vertex) buffer->target = GL_BUFFER_TARGET_VERTEX;
    else if (info->usage & VGPUBufferUsage_Index) buffer->target = GL_BUFFER_TARGET_INDEX;
    else if (info->usage & VGPUBufferUsage_Uniform) buffer->target = GL_BUFFER_TARGET_UNIFORM;
    else if (info->usage & VGPUBufferUsage_Storage) buffer->target = GL_BUFFER_TARGET_STORAGE;
    else if (info->usage & VGPUBufferUsage_Indirect) buffer->target = GL_BUFFER_TARGET_INDIRECT;
    else buffer->target = GL_BUFFER_TARGET_COPY_DST;

#ifdef AGPU_GL
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
        _agpu_gl_bind_buffer(renderer, buffer->target, buffer->id, false);

        GLenum gl_usage = _gl_GetBufferUsage(info->usage);

#if defined(AGPU_WEBGL)
        GL_CHECK(glBufferData(buffer->target, info->size, info->content, gl_usage));
#else
        GLenum gl_target = _agpu_gl_get_buffer_target(buffer->target);
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
    return (vgpu_buffer*)buffer;
}

static void gl_buffer_destroy(VGPURenderer* driverData, vgpu_buffer* handle) {
    vgpu_buffer_gl* buffer = (vgpu_buffer_gl*)handle;
    GL_CHECK(glDeleteBuffers(1, &buffer->id));
    AGPU_FREE(buffer);
}

static void gl_buffer_sub_data(VGPURenderer* driverData, vgpu_buffer* handle, VGPUDeviceSize offset, VGPUDeviceSize size, const void* pData)
{
    vgpu_buffer_gl* buffer = (vgpu_buffer_gl*)handle;
#ifdef AGPU_GL
    if (gl.ext.direct_state_access)
    {
        if (size == 0)
            size = buffer->size - offset;

        void* mapped_data = glMapNamedBufferRange(buffer->id, offset, size, GL_MAP_WRITE_BIT);
        _GPU_GL_CHECK_ERROR();
        memcpy(mapped_data, pData, size);
        glUnmapNamedBuffer(buffer->id);
        //glNamedBufferSubData(buffer->id, offset, buffer->size, pData);
    }
    else
#endif 
    {
        VGPURendererGL* renderer = (VGPURendererGL*)driverData;
        _agpu_gl_bind_buffer(renderer, buffer->target, buffer->id, false);
        GLenum gl_target = _agpu_gl_get_buffer_target(buffer->target);
        glBufferSubData(gl_target, offset, buffer->size, pData);
    }
    _GPU_GL_CHECK_ERROR();
}

/* Texture */
static VGPUTexture* gl_create_texture(VGPURenderer* driverData, const VGPUTextureInfo* info)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    vgpu_texture_gl* texture = _AGPU_ALLOC_HANDLE(vgpu_texture_gl);

#ifdef AGPU_GL
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

    return (VGPUTexture*)texture;
}

static void gl_destroy_texture(VGPURenderer* driverData, VGPUTexture* handle) {
    vgpu_texture_gl* texture = (vgpu_texture_gl*)handle;
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

static vgpu_shader gl_create_shader(VGPURenderer* driverData, const vgpu_shader_info* info) {
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
    vgpu_shader_gl* result = _AGPU_ALLOC_HANDLE(vgpu_shader_gl);
    result->id = program;
    return (vgpu_shader)result;
}

static void gl_destroy_shader(VGPURenderer* driverData, vgpu_shader handle) {
    vgpu_shader_gl* shader = (vgpu_shader_gl*)handle;
    GL_CHECK(glDeleteProgram(shader->id));
    AGPU_FREE(shader);
}

/* Pipeline */
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

static GLenum _agpu_gl_index_type(VGPUIndexType format) {
    static const GLenum types[] = {
        [VGPUIndexType_UInt16] = GL_UNSIGNED_SHORT,
        [VGPUIndexType_UInt32] = GL_UNSIGNED_INT,
    };
    return types[format];
}

static GLenum _agpu_gl_vertex_format_type(VGPUVertexFormat format) {
    switch (format) {
    case VGPUVertexFormat_UChar2:
    case VGPUVertexFormat_UChar4:
    case VGPUVertexFormat_UChar2Norm:
    case VGPUVertexFormat_UChar4Norm:
        return GL_UNSIGNED_BYTE;
    case VGPUVertexFormat_Char2:
    case VGPUVertexFormat_Char4:
    case VGPUVertexFormat_Char2Norm:
    case VGPUVertexFormat_Char4Norm:
        return GL_BYTE;
    case VGPUVertexFormat_UShort2:
    case VGPUVertexFormat_UShort4:
    case VGPUVertexFormat_UShort2Norm:
    case VGPUVertexFormat_UShort4Norm:
        return GL_UNSIGNED_SHORT;
    case VGPUVertexFormat_Short2:
    case VGPUVertexFormat_Short4:
    case VGPUVertexFormat_Short2Norm:
    case VGPUVertexFormat_Short4Norm:
        return GL_SHORT;
    case VGPUVertexFormat_Half2:
    case VGPUVertexFormat_Half4:
        return GL_HALF_FLOAT;
    case VGPUVertexFormat_Float:
    case VGPUVertexFormat_Float2:
    case VGPUVertexFormat_Float3:
    case VGPUVertexFormat_Float4:
        return GL_FLOAT;
    case VGPUVertexFormat_UInt:
    case VGPUVertexFormat_UInt2:
    case VGPUVertexFormat_UInt3:
    case VGPUVertexFormat_UInt4:
        return GL_UNSIGNED_INT;
    case VGPUVertexFormat_Int:
    case VGPUVertexFormat_Int2:
    case VGPUVertexFormat_Int3:
    case VGPUVertexFormat_Int4:
        return GL_INT;
    default:
        VGPU_UNREACHABLE();
    }
}

static GLboolean _agpu_gl_vertex_format_normalized(VGPUVertexFormat format) {
    switch (format) {
    case VGPUVertexFormat_UChar2Norm:
    case VGPUVertexFormat_UChar4Norm:
    case VGPUVertexFormat_Char2Norm:
    case VGPUVertexFormat_Char4Norm:
    case VGPUVertexFormat_UShort2Norm:
    case VGPUVertexFormat_UShort4Norm:
    case VGPUVertexFormat_Short2Norm:
    case VGPUVertexFormat_Short4Norm:
        return GL_TRUE;
    default:
        return GL_FALSE;
    }
}

static GLboolean _agpu_gl_vertex_format_integer(VGPUVertexFormat format) {
    switch (format) {
    case VGPUVertexFormat_UChar2:
    case VGPUVertexFormat_UChar4:
    case VGPUVertexFormat_Char2:
    case VGPUVertexFormat_Char4:
    case VGPUVertexFormat_UShort2:
    case VGPUVertexFormat_UShort4:
    case VGPUVertexFormat_Short2:
    case VGPUVertexFormat_Short4:
    case VGPUVertexFormat_UInt:
    case VGPUVertexFormat_UInt2:
    case VGPUVertexFormat_UInt3:
    case VGPUVertexFormat_UInt4:
    case VGPUVertexFormat_Int:
    case VGPUVertexFormat_Int2:
    case VGPUVertexFormat_Int3:
    case VGPUVertexFormat_Int4:
        return true;
    default:
        return false;
    }
}

static agpu_pipeline gl_create_pipeline(VGPURenderer* driverData, const vgpu_pipeline_info* info)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    vgpu_pipeline_gl* pipeline = _AGPU_ALLOC_HANDLE(vgpu_pipeline_gl);
    pipeline->shader = (vgpu_shader_gl*)info->shader;
    pipeline->primitive_type = get_gl_primitive_type(info->primitive_topology);
    pipeline->index_type = _agpu_gl_index_type(info->indexType);
    pipeline->attribute_count = 0;

    /* Setup vertex attributes */
    for (uint32_t attrIndex = 0; attrIndex < renderer->caps.limits.maxVertexInputAttributes; attrIndex++) {
        const VGPUVertexAttributeInfo* attrDesc = &info->vertexInfo.attributes[attrIndex];
        if (attrDesc->format == VGPUVertexFormat_Invalid) {
            break;
        }

        const VGPUVertexBufferLayoutInfo* layoutDesc = &info->vertexInfo.layouts[attrDesc->bufferIndex];

        vgpu_vertex_attribute_gl* gl_attr = &pipeline->attributes[pipeline->attribute_count++];
        gl_attr->buffer_index = (int8_t)attrDesc->bufferIndex;
        gl_attr->shaderLocation = attrIndex;
        gl_attr->stride = (GLsizei)layoutDesc->stride;
        gl_attr->offset = attrDesc->offset;
        gl_attr->size = vgpuGetVertexFormatComponentsCount(attrDesc->format);
        gl_attr->type = _agpu_gl_vertex_format_type(attrDesc->format);
        gl_attr->normalized = _agpu_gl_vertex_format_normalized(attrDesc->format);
        gl_attr->integer = _agpu_gl_vertex_format_integer(attrDesc->format);
        if (layoutDesc->stepMode == AGPUInputStepMode_Vertex) {
            gl_attr->divisor = 0;
        }
        else {
            gl_attr->divisor = 1;
        }
    }

    pipeline->depth_stencil = info->depth_stencil;
    //pipeline->gl.blend = desc->blend;
    //pipeline->gl.rast = desc->rasterizer;

    return (agpu_pipeline)pipeline;
}

static void gl_destroy_pipeline(VGPURenderer* driverData, agpu_pipeline handle) {
    vgpu_pipeline_gl* pipeline = (vgpu_pipeline_gl*)handle;
    AGPU_FREE(pipeline);
}

/* CommandBuffer */
static void gl_cmdBeginRenderPass(VGPURenderer* driverData, const VGPURenderPassDescriptor* descriptor)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    AGPU_ASSERT(!renderer->cache.insidePass);
    renderer->cache.insidePass = true;
}

static void gl_cmdEndRenderPass(VGPURenderer* driverData)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    AGPU_ASSERT(renderer->cache.insidePass);
    _GPU_GL_CHECK_ERROR();

    renderer->cache.insidePass = false;
}

static void gl_cmdSetPipeline(VGPURenderer* driverData, agpu_pipeline handle)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    vgpu_pipeline_gl* pipeline = (vgpu_pipeline_gl*)handle;
    _GPU_GL_CHECK_ERROR();
    if (renderer->cache.current_pipeline != pipeline) {
        /* Apply depth-stencil state */
        const vgpu_depth_stencil_state* new_ds = &pipeline->depth_stencil;
        vgpu_depth_stencil_state* cache_ds = &renderer->cache.depth_stencil;

        if (new_ds->depth_compare == VGPU_COMPARE_FUNCTION_ALWAYS &&
            !new_ds->depth_write_enabled) {
            glDisable(GL_DEPTH_TEST);
        }
        else {
            glEnable(GL_DEPTH_TEST);
        }

        if (new_ds->depth_write_enabled != cache_ds->depth_write_enabled) {
            cache_ds->depth_write_enabled = new_ds->depth_write_enabled;
            glDepthMask(new_ds->depth_write_enabled);
        }

        if (new_ds->depth_compare != cache_ds->depth_compare) {
            cache_ds->depth_compare = new_ds->depth_compare;
            glDepthFunc(_vgpu_gl_compare_func(new_ds->depth_compare));
        }

        _agpu_gl_use_program(renderer, pipeline->shader->id);
        renderer->cache.current_pipeline = pipeline;
    }
}

static void gl_cmdSetVertexBuffer(VGPURenderer* driverData, uint32_t slot, vgpu_buffer* buffer, uint64_t offset)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    renderer->cache.vertex_buffers[slot] = ((vgpu_buffer_gl*)buffer)->id;
    renderer->cache.vertex_buffer_offsets[slot] = offset;
}

static void gl_cmdSetIndexBuffer(VGPURenderer* driverData, vgpu_buffer* buffer, uint64_t offset)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    renderer->cache.index.buffer = ((vgpu_buffer_gl*)buffer)->id;
    renderer->cache.index.offset = offset;
}

static void gl_set_uniform_buffer(VGPURenderer* driverData, uint32_t set, uint32_t binding, vgpu_buffer* handle)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    vgpu_buffer_gl* buffer = (vgpu_buffer_gl*)handle;

    //GLuint loc = glGetUniformBlockIndex(renderer->cache.program, "Transform");
    //glUniformBlockBinding(renderer->cache.program, loc, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer->id);
    _GPU_GL_CHECK_ERROR();
}

static void gl_set_uniform_buffer_data(VGPURenderer* driverData, uint32_t set, uint32_t binding, const void* data, VGPUDeviceSize size)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    memcpy(renderer->ubo_buffer->data, data, size);
    glBindBufferRange(GL_UNIFORM_BUFFER, binding, renderer->ubo_buffer->id, 0, size);
    _GPU_GL_CHECK_ERROR();
}

static void _agpu_gl_prepare_draw(VGPURendererGL* renderer)
{
    AGPU_ASSERT(renderer->cache.insidePass);

    if (renderer->cache.index.buffer != 0) {
        _agpu_gl_bind_buffer(renderer, GL_BUFFER_TARGET_INDEX, renderer->cache.index.buffer, false);
    }

    uint16_t current_enable_locations = 0;
    for (uint32_t i = 0; i < renderer->cache.current_pipeline->attribute_count; i++) {
        vgpu_vertex_attribute_gl* gl_attr = &renderer->cache.current_pipeline->attributes[i];
        vgpu_vertex_attribute_cache_gl* cache_attr = &renderer->cache.attributes[i];

        GLuint gl_vb = 0;
        uint64_t offset = 0;

        bool cache_attr_dirty = false;
        if (gl_attr->buffer_index >= 0) {
            gl_vb = renderer->cache.vertex_buffers[gl_attr->buffer_index];
            offset = renderer->cache.vertex_buffer_offsets[gl_attr->buffer_index] + gl_attr->offset;
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
                _agpu_gl_bind_buffer(renderer, GL_BUFFER_TARGET_VERTEX, renderer->cache.vertex_buffers[gl_attr->buffer_index], false);
                if (gl_attr->integer) {
                    glVertexAttribIPointer(
                        gl_attr->shaderLocation,
                        gl_attr->size,
                        gl_attr->type,
                        gl_attr->stride,
                        (const GLvoid*)(GLintptr)offset);
                }
                else {
                    glVertexAttribPointer(
                        gl_attr->shaderLocation,
                        gl_attr->size,
                        gl_attr->type,
                        gl_attr->normalized,
                        gl_attr->stride,
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

    uint16_t diff = current_enable_locations ^ renderer->cache.enabled_locations;
    if (diff != 0) {
        for (uint32_t i = 0; i < renderer->caps.limits.maxVertexInputAttributes; i++) {
            if (diff & (1 << i)) {
                if (current_enable_locations & (1 << i)) {
                    glEnableVertexAttribArray(i);
                }
                else {
                    glDisableVertexAttribArray(i);
                }
            }
        }

        renderer->cache.enabled_locations = current_enable_locations;
    }
}

static void gl_cmdDraw(VGPURenderer* driverData, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    _agpu_gl_prepare_draw(renderer);

    if (instanceCount > 1) {
        glDrawArraysInstanced(renderer->cache.current_pipeline->primitive_type, firstVertex, vertexCount, instanceCount);
    }
    else {
        glDrawArrays(renderer->cache.current_pipeline->primitive_type, firstVertex, vertexCount);
    }
    _GPU_GL_CHECK_ERROR();
}

static void gl_cmdDrawIndexed(VGPURenderer* driverData, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex)
{
    VGPURendererGL* renderer = (VGPURendererGL*)driverData;
    _agpu_gl_prepare_draw(renderer);

    GLenum glIndexType = renderer->cache.current_pipeline->index_type;
    const int i_size = (glIndexType == GL_UNSIGNED_SHORT) ? 2 : 4;
    const GLvoid* indices = (const GLvoid*)(GLintptr)(firstIndex * i_size + renderer->cache.index.offset);

    if (instanceCount > 1) {
        glDrawElementsInstanced(renderer->cache.current_pipeline->primitive_type, indexCount, glIndexType, indices, instanceCount);
    }
    else {
        glDrawElements(renderer->cache.current_pipeline->primitive_type, indexCount, glIndexType, indices);
    }
    _GPU_GL_CHECK_ERROR();
}

/* Driver functions */
static bool gl_supported(void) {
    return true;
}

static VGPUDeviceImpl* gl_create_device(void) {
    VGPUDevice device;
    VGPURendererGL* renderer;

    device = (VGPUDeviceImpl*)VGPU_MALLOC(sizeof(VGPUDeviceImpl));
    ASSIGN_DRIVER(gl);

    /* Init the vk renderer */
    renderer = (VGPURendererGL*)_AGPU_ALLOC_HANDLE(VGPURendererGL);

    /* Reference vgpu_device and renderer together. */
    renderer->gpuDevice = device;
    device->renderer = (VGPURenderer*)renderer;

    return device;
}

agpu_driver gl_driver = {
    gl_supported,
    gl_create_device
};

#endif /* defined(GPU_GL_BACKEND) */
