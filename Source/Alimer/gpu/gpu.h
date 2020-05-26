#pragma once

#ifdef __cplusplus
#   define GPU_API extern "C"
#else
#   define GPU_API extern
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef uint32_t gpu_flags;

enum {
    GPU_MAX_LOG_MESSAGE_LENGTH = 1024,
    GPU_MAX_COLOR_ATTACHMENTS = 8u,
};

typedef struct GPUDevice GPUDevice;
typedef struct GPUContext GPUContext;
typedef struct gpu_buffer_t* gpu_buffer;
typedef struct gpu_texture_t* gpu_texture;
typedef struct gpu_sampler_t* gpu_sampler;
typedef struct gpu_pass_t* gpu_pass;
typedef struct gpu_shader gpu_shader;
typedef struct gpu_pipeline_t* gpu_pipeline;

/* Enums */
typedef enum GPULogLevel {
    GPULogLevel_Error,
    GPULogLevel_Warn,
    GPULogLevel_Info,
    GPULogLevel_Debug,
    GPULogLevel_Force32 = 0x7FFFFFFF
} GPULogLevel;

typedef enum GPUBackendType {
    GPUBackendType_Null = 0,
    GPUBackendType_Direct3D11 = 1,
    GPUBackendType_Direct3D12 = 2,
    GPUBackendType_Metal = 3,
    GPUBackendType_Vulkan = 4,
    GPUBackendType_OpenGL = 5,
    GPUBackendType_Force32 = 0x7FFFFFFF
} GPUBackendType;

typedef enum GPUTexfureFormat {
    // 8-bit pixel formats
    GPU_TEXTURE_FORMAT_R8,
    GPU_TEXTURE_FORMAT_R8I,
    GPU_TEXTURE_FORMAT_R8U,
    GPU_TEXTURE_FORMAT_R8S,
    // 16-bit pixel formats
    GPU_TEXTURE_FORMAT_R16,
    GPU_TEXTURE_FORMAT_R16I,
    GPU_TEXTURE_FORMAT_R16U,
    GPU_TEXTURE_FORMAT_R16S,
    GPU_TEXTURE_FORMAT_R16F,
    GPU_TEXTURE_FORMAT_RG8,
    GPU_TEXTURE_FORMAT_RG8I,
    GPU_TEXTURE_FORMAT_RG8U,
    GPU_TEXTURE_FORMAT_RG8S,
    // 32-bit pixel formats
    GPU_TEXTURE_FORMAT_R32I,
    GPU_TEXTURE_FORMAT_R32U,
    GPU_TEXTURE_FORMAT_R32F,
    GPU_TEXTURE_FORMAT_RG16,
    GPU_TEXTURE_FORMAT_RG16I,
    GPU_TEXTURE_FORMAT_RG16U,
    GPU_TEXTURE_FORMAT_RG16S,
    GPU_TEXTURE_FORMAT_RG16F,
    GPU_TEXTURE_FORMAT_RGBA8,
    GPU_TEXTURE_FORMAT_RGBA8_SRGB,
    GPU_TEXTURE_FORMAT_RGBA8I,
    GPU_TEXTURE_FORMAT_RGBA8U,
    GPU_TEXTURE_FORMAT_RGBA8S,
    GPU_TEXTURE_FORMAT_BGRA8,
    GPU_TEXTURE_FORMAT_BGRA8_SRGB,
    // Packed 32-Bit Pixel formats
    GPU_TEXTURE_FORMAT_RGB10A2,
    GPU_TEXTURE_FORMAT_RG11B10F,
    // 64-Bit Pixel Formats
    GPU_TEXTURE_FORMAT_RG32I,
    GPU_TEXTURE_FORMAT_RG32U,
    GPU_TEXTURE_FORMAT_RG32F,
    GPU_TEXTURE_FORMAT_RGBA16,
    GPU_TEXTURE_FORMAT_RGBA16I,
    GPU_TEXTURE_FORMAT_RGBA16U,
    GPU_TEXTURE_FORMAT_RGBA16S,
    GPU_TEXTURE_FORMAT_RGBA16F,
    // 128-Bit Pixel Formats
    GPU_TEXTURE_FORMAT_RGBA32I,
    GPU_TEXTURE_FORMAT_RGBA32U,
    GPU_TEXTURE_FORMAT_RGBA32F,
    // Depth-stencil formats
    GPU_TEXTURE_FORMAT_D32F,
    GPU_TEXTURE_FORMAT_D16,
    GPU_TEXTURE_FORMAT_D24S8,
    // Compressed BC formats
    GPU_TEXTURE_FORMAT_BC1,
    GPU_TEXTURE_FORMAT_BC1_SRGB,
    GPU_TEXTURE_FORMAT_BC2,
    GPU_TEXTURE_FORMAT_BC2_SRGB,
    GPU_TEXTURE_FORMAT_BC3,
    GPU_TEXTURE_FORMAT_BC3_SRGB,
    GPU_TEXTURE_FORMAT_BC4,
    GPU_TEXTURE_FORMAT_BC4S,
    GPU_TEXTURE_FORMAT_BC5,
    GPU_TEXTURE_FORMAT_BC5S,
    GPU_TEXTURE_FORMAT_BC6H,
    GPU_TEXTURE_FORMAT_BC6HS,
    GPU_TEXTURE_FORMAT_BC7,
    GPU_TEXTURE_FORMAT_BC7_SRGB,
    GPUTexfureFormat_Force32 = 0x7FFFFFFF
} GPUTexfureFormat;

typedef enum GPUTextureUsage {
    GPUTextureUsage_None = 0x00000000,
    GPUTextureUsage_Sampled = (1 << 0),
    GPUTextureUsage_Storage = (1 << 1),
    GPUTextureUsage_OutputAttachment = (1 << 2),
    GPUTextureUsage_Force32 = 0x7FFFFFFF
} GPUTextureUsage;

typedef enum GPUTextureType {
    GPUTextureType_2D,
    GPUTextureType_3D,
    GPUTextureType_Cube,
    GPUTextureType_Array,
    GPUTextureType_Force32 = 0x7FFFFFFF
} GPUTextureType;

typedef enum GPUBufferUsage {
    GPUBufferUsage_None = 0,
    GPUBufferUsage_Vertex = (1 << 0),
    GPUBufferUsage_Index = (1 << 1),
    GPUBufferUsage_Uniform = (1 << 2),
    GPUBufferUsage_Storage = (1 << 3),
    GPUBufferUsage_Force32 = 0x7FFFFFFF
} GPUBufferUsage;

typedef enum GPUPresentInterval
{
    GPUPresentInterval_Default,
    GPUPresentInterval_One,
    GPUPresentInterval_Two,
    GPUPresentInterval_Immediate,
    GPUPresentInterval_Force32 = 0x7FFFFFFF
} GPUPresentInterval;

/* Structures */
typedef struct GPUSwapChainDescriptor {
    void* windowHandle;
    uint32_t width;
    uint32_t height;
    bool isFullScreen;
    GPUTexfureFormat colorFormat;
    GPUTexfureFormat depthStencilFormat;
    GPUPresentInterval presentationInterval;
    uint32_t sampleCount;
} GPUSwapChainDescriptor;

typedef struct GPUTextureViewDescriptor {
    gpu_texture source;
    GPUTexfureFormat format;
    GPUTextureType type;
    uint32_t baseMipmap;
    uint32_t mipmapCount;
    uint32_t baseLayer;
    uint32_t layerCount;
} GPUTextureViewDescriptor;

typedef struct GPUTextureDescriptor {
    uint32_t usage;
    GPUTextureType type;
    GPUTexfureFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_levels;
    uint32_t array_layers;
    const char* label;
    uint64_t external_handle;
} GPUTextureDescriptor;

typedef struct {
    uint64_t size;
    uint32_t usage;
    /*void* data;*/
    const char* label;
} gpu_buffer_info;

// Sampler
typedef enum {
    GPU_FILTER_NEAREST,
    GPU_FILTER_LINEAR
} gpu_filter;

typedef struct {
    gpu_filter mag;
    gpu_filter min;
    gpu_filter mip;
    const char* label;
} gpu_sampler_info;

GPU_API gpu_sampler gpu_sampler_create(const gpu_sampler_info* info);
GPU_API void gpu_sampler_destroy(gpu_sampler sampler);

// Pass
typedef enum {
    GPU_LOAD_OP_LOAD,
    GPU_LOAD_OP_CLEAR,
    GPU_LOAD_OP_DISCARD
} gpu_load_op;

typedef struct {
    gpu_texture texture;
    uint32_t layer;
    uint32_t level;
    gpu_load_op load;
    float clear[4];
} gpu_color_attachment;

typedef struct {
    gpu_texture texture;
    gpu_load_op load;
    float clear;
    struct {
        gpu_load_op load;
        uint8_t clear;
    } stencil;
} gpu_depth_attachment;

typedef struct {
    gpu_color_attachment color[GPU_MAX_COLOR_ATTACHMENTS];
    gpu_depth_attachment depth;
    uint32_t size[2];
    const char* label;
} gpu_pass_info;

GPU_API gpu_pass gpu_pass_create(const gpu_pass_info* info);
GPU_API void gpu_pass_destroy(gpu_pass pass);

// Shader
typedef struct {
    const void* code;
    size_t size;
    const char* entry;
} gpu_shader_source;

typedef struct {
    gpu_shader_source vertex;
    gpu_shader_source fragment;
    gpu_shader_source compute;
    const char* label;
} gpu_shader_info;

size_t gpu_sizeof_shader(void);
bool gpu_shader_init(gpu_shader* shader, gpu_shader_info* info);
void gpu_shader_destroy(gpu_shader* shader);

/* Pipeline */
typedef enum {
    GPU_FLOAT_F32,
    GPU_VEC2_F32,
    GPU_VEC2_F16,
    GPU_VEC2_U16N,
    GPU_VEC2_I16N,
    GPU_VEC3_F32,
    GPU_VEC4_F32,
    GPU_VEC4_F16,
    GPU_VEC4_U16N,
    GPU_VEC4_I16N,
    GPU_VEC4_U8N,
    GPU_VEC4_I8N,
    GPU_UINT_U32,
    GPU_UVEC2_U32,
    GPU_UVEC3_U32,
    GPU_UVEC4_U32,
    GPU_INT_I32,
    GPU_IVEC2_I32,
    GPU_IVEC3_I32,
    GPU_IVEC4_I32
} gpu_attribute_format;

typedef struct {
    uint8_t location;
    uint8_t buffer;
    uint8_t format;
    uint8_t offset;
} gpu_attribute;

typedef struct {
    uint16_t stride;
    uint16_t divisor;
} gpu_buffer_layout;

typedef enum {
    GPU_DRAW_POINTS,
    GPU_DRAW_LINES,
    GPU_DRAW_LINE_STRIP,
    GPU_DRAW_TRIANGLES,
    GPU_DRAW_TRIANGLE_STRIP
} gpu_draw_mode;

typedef enum {
    GPU_INDEX_U16,
    GPU_INDEX_U32
} gpu_index_stride;

typedef enum {
    GPU_CULL_NONE,
    GPU_CULL_FRONT,
    GPU_CULL_BACK
} gpu_cull_mode;

typedef enum {
    GPU_WINDING_CCW,
    GPU_WINDING_CW
} gpu_winding;

typedef enum {
    GPU_COMPARE_FUNCTION_UNDEFINED = 0,
    GPU_COMPARE_FUNCTION_NEVER = 1,
    GPU_COMPARE_FUNCTION_LESS = 2,
    GPU_COMPARE_FUNCTION_LESS_EQUAL = 3,
    GPU_COMPARE_FUNCTION_GREATER = 4,
    GPU_COMPARE_FUNCTION_GREATER_EQUAL = 5,
    GPU_COMPARE_FUNCTION_EQUAL = 6,
    GPU_COMPARE_FUNCTION_NOT_EQUAL = 7,
    GPU_COMPARE_FUNCTION_ALWAYS = 8,
    _GPU_COMPARE_FUNCTION_FORCE_U32 = 0x7FFFFFFF
} gpu_compare_function;

typedef enum {
    GPU_STENCIL_KEEP,
    GPU_STENCIL_ZERO,
    GPU_STENCIL_REPLACE,
    GPU_STENCIL_INCREMENT,
    GPU_STENCIL_DECREMENT,
    GPU_STENCIL_INCREMENT_WRAP,
    GPU_STENCIL_DECREMENT_WRAP,
    GPU_STENCIL_INVERT
} gpu_stencil_action;

typedef struct {
    gpu_compare_function compare;
    gpu_stencil_action fail;
    gpu_stencil_action depthFail;
    gpu_stencil_action pass;
    uint8_t readMask;
    uint8_t writeMask;
    uint8_t reference;
} gpu_stencil_state;

typedef enum {
    GPU_BLEND_ZERO,
    GPU_BLEND_ONE,
    GPU_BLEND_SRC_COLOR,
    GPU_BLEND_ONE_MINUS_SRC_COLOR,
    GPU_BLEND_SRC_ALPHA,
    GPU_BLEND_ONE_MINUS_SRC_ALPHA,
    GPU_BLEND_DST_COLOR,
    GPU_BLEND_ONE_MINUS_DST_COLOR,
    GPU_BLEND_DST_ALPHA,
    GPU_BLEND_ONE_MINUS_DST_ALPHA
} gpu_blend_factor;

typedef enum {
    GPU_BLEND_ADD,
    GPU_BLEND_SUB,
    GPU_BLEND_RSUB,
    GPU_BLEND_MIN,
    GPU_BLEND_MAX
} gpu_blend_op;

typedef struct {
    struct {
        gpu_blend_factor src;
        gpu_blend_factor dst;
        gpu_blend_op op;
    } color, alpha;
    bool enabled;
} gpu_blend_state;

typedef struct {
    gpu_shader* shader;
    gpu_pass pass;
    gpu_buffer_layout buffers[16];
    gpu_attribute attributes[16];
    gpu_index_stride indexStride;
    gpu_draw_mode drawMode;
    gpu_cull_mode cullMode;
    gpu_winding winding;
    float depthOffset;
    float depthOffsetSloped;
    bool depthWrite;
    gpu_compare_function depthTest;
    gpu_stencil_state stencilFront;
    gpu_stencil_state stencilBack;
    bool alphaToCoverage;
    uint8_t colorMask;
    gpu_blend_state blend;
    const char* label;
} gpu_pipeline_info;

GPU_API gpu_pipeline gpu_pipeline_create(const gpu_pipeline_info* info);
GPU_API void gpu_pipeline_destroy(gpu_pipeline pipeline);

typedef struct {
    bool anisotropy;
    bool astc;
    bool dxt;
} gpu_features;

typedef struct {
    uint32_t textureSize;
    uint32_t framebufferSize[2];
    uint32_t framebufferSamples;
} gpu_limits;

/* Callbacks */
typedef void(*GPULogCallback)(void* userData, GPULogLevel level, const char* message);

/* Log functions */
GPU_API void gpuSetLogCallback(GPULogCallback callback, void* userData);
GPU_API void gpuLog(GPULogLevel level, const char* format, ...);

GPU_API GPUDevice* gpuDeviceCreate(GPUBackendType backendType, bool debug, const GPUSwapChainDescriptor* descriptor);
GPU_API void gpuDeviceDestroy(GPUDevice* device);
GPU_API void gpuBeginFrame(GPUDevice* device);
GPU_API void gpuEndFrame(GPUDevice* device);
GPU_API gpu_features gpuDeviceGetFeatures(GPUDevice* device);
GPU_API gpu_limits gpuDeviceGetLimits(GPUDevice* device);

GPU_API GPUContext* gpuDeviceCreateContext(GPUDevice* device, const GPUSwapChainDescriptor* descriptor);
GPU_API void gpuDeviceDestroyContext(GPUDevice* device, GPUContext* context);
GPU_API bool gpuContextResize(GPUContext* context, uint32_t width, uint32_t height);

GPU_API void gpu_render_begin(gpu_pass pass);
GPU_API void gpu_render_finish(void);
GPU_API void gpu_set_pipeline(gpu_pipeline pipeline);
GPU_API void gpu_set_vertex_buffers(gpu_buffer* buffers, uint64_t* offsets, uint32_t count);
GPU_API void gpu_set_index_buffer(gpu_buffer buffer, uint64_t offset);
GPU_API void gpu_draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
GPU_API void gpu_draw_indexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex);
GPU_API void gpu_draw_indirect(gpu_buffer buffer, uint64_t offset, uint32_t drawCount);
GPU_API void gpu_draw_indirect_indexed(gpu_buffer buffer, uint64_t offset, uint32_t drawCount);
GPU_API void gpu_compute(gpu_shader* shader, uint32_t x, uint32_t y, uint32_t z);

/* Texture */
GPU_API gpu_texture gpuDeviceCreateTexture(GPUDevice* device, const GPUTextureDescriptor* descriptor);
GPU_API bool gpu_texture_init_view(gpu_texture texture, const GPUTextureViewDescriptor* info);
GPU_API void gpu_texture_destroy(gpu_texture texture);
GPU_API void gpu_texture_write(gpu_texture texture, uint8_t* data, uint64_t size, uint16_t offset[4], uint16_t extent[4], uint16_t mip);

/* Buffer */
GPU_API gpu_buffer gpu_buffer_create(const gpu_buffer_info* info);
GPU_API void gpu_buffer_destroy(gpu_buffer buffer);
GPU_API uint8_t* gpu_buffer_map(gpu_buffer buffer, uint64_t offset, uint64_t size);
GPU_API void gpu_buffer_unmap(gpu_buffer buffer);

namespace Alimer
{
    struct IAllocator;

    namespace gpu
    {
        /* Handles */
        static constexpr uint32_t kInvalidHandle = 0xFFffFFff;

        struct ContextHandle { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };
        static constexpr ContextHandle kInvalidContext = { kInvalidHandle };

        /* Enums */
        enum class LogLevel : uint32_t {
            Error,
            Warn,
            Info,
            Debug
        };

        enum BackendType : uint32_t {
            Null,
            Direct3D11,
            Direct3D12,
            Vulkan,
            OpenGL,
            Count
        };

        typedef void (*LogCallback)(void* userData, const char* message, LogLevel level);
        typedef void* (*GetProcAddressCallback)(const char* functionName);

        struct Configuration {
            BackendType backendType = BackendType::Count;
            bool debug;
            LogCallback logCallback;
            GetProcAddressCallback getProcAddress;
            void* userData;
        };

        bool initialize(const Configuration& config, IAllocator& allocator);
        void shutdown();
    }
}
