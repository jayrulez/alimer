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

#if defined(VGPU_BACKEND_OPENGL) 
#include "vgpu_backend.h"

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
    //vgpu_depth_stencil_state depth_stencil;
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

/* Renderer functions */
static bool gl_init(const vgpu_config* info)
{
    if (!vgpu_opengl_supported()) {
        return false;
    }

#ifdef AGPU_GL
    if (!gladLoadGLLoader((GLADloadproc)info->get_proc_address)) {
        return false;
    }
#elif defined(AGPU_GLES)
    gladLoadGLES2Loader((GLADloadproc)info->get_proc_address);
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
    gl.ext.buffer_storage = _vgpu_gl_version(4, 2) || GLAD_GL_ARB_buffer_storage;
    gl.ext.texture_storage = _vgpu_gl_version(4, 4) || GLAD_GL_ARB_texture_storage;
    gl.ext.direct_state_access = _vgpu_gl_version(4, 5) || GLAD_GL_ARB_direct_state_access;
#else

#if defined(AGPU_WEBGL)
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
#if !defined(AGPU_WEBGL)
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, (GLint*)&gl.caps.limits.max_storage_buffer_size);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, (GLint*)&gl.caps.limits.min_storage_buffer_offset_alignment);
    if (GLAD_GL_EXT_texture_filter_anisotropic) {
        GLfloat attr = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &attr);
        gl.caps.limits.maxSamplerAnisotropy = (uint32_t)attr;
    }

    // Viewport
    glGetIntegerv(GL_MAX_VIEWPORTS, (GLint*)&gl.caps.limits.maxViewports);

#if !defined(AGPU_GLES)
    glGetIntegerv(GL_MAX_PATCH_VERTICES, (GLint*)&gl.caps.limits.maxTessellationPatchSize);
#endif

    float point_sizes[2];
    float line_width_range[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, point_sizes);
    glGetFloatv(GL_LINE_WIDTH_RANGE, line_width_range);

    // Compute
    /*if (gl.ext.compute)
    {
        glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, (GLint*)&gl.caps.limits.maxComputeSharedMemorySize);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, (GLint*)&gl.caps.limits.maxComputeWorkGroupCount[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, (GLint*)&gl.caps.limits.maxComputeWorkGroupCount[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, (GLint*)&gl.caps.limits.maxComputeWorkGroupCount[2]);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, (GLint*)&gl.caps.limits.maxComputeWorkGroupInvocations);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, (GLint*)&gl.caps.limits.maxComputeWorkGroupSize[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, (GLint*)&gl.caps.limits.maxComputeWorkGroupSize[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, (GLint*)&gl.caps.limits.maxComputeWorkGroupSize[2]);
        _GPU_GL_CHECK_ERROR();
    }*/

#else
    // WebGL
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, point_sizes);
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, line_width_range);

    gl.limits.max_sampler_anisotropy = 1;
#endif

    GLint maxViewportDims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
    /*gl.caps.limits.maxViewports[0] = (uint32_t)maxViewportDims[0];
    gl.caps.limits.maxViewportDimensions[1] = (uint32_t)maxViewportDims[1];
    gl.caps.limits.pointSizeRange[0] = point_sizes[0];
    gl.caps.limits.pointSizeRange[1] = point_sizes[1];
    gl.caps.limits.lineWidthRange[0] = line_width_range[0];
    gl.caps.limits.lineWidthRange[1] = line_width_range[1];*/
    _GPU_GL_CHECK_ERROR();

    /* Reset state cache. */
    //_vgpu_gl_reset_state_cache(renderer);

    return true;
}

static void gl_destroy(void)
{
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

static void gl_begin_frame(void)
{
}

static void gl_end_frame(void)
{
}

static VGPUTexture gl_create_texture(const VGPUTextureDescriptor* desc)
{
    return vgpu::kInvalidTexture;
}

static void gl_destroy_texture(VGPUTexture handle)
{
}

/* Driver functions */
bool vgpu_opengl_supported(void) {
    return true;
}

vgpu_renderer* vgpu_opengl_create_device(void) {
    static vgpu_renderer renderer = { nullptr };
    ASSIGN_DRIVER(gl);
    return &renderer;
}

#else

#include "vgpu/vgpu.h"

bool vgpu_opengl_supported(void) {
    return false;
}

VGPUDevice vgpu_opengl_create_device(void) {
    return nullptr;
}

#endif /* defined(VGPU_D3D11) */
