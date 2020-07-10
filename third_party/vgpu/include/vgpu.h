//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#pragma once

#if defined(VGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_API __declspec(dllexport)
#       else
#           define VGPU_API __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_API __attribute__((visibility("default")))
#       else
#           define VGPU_API
#       endif
#   endif  // defined(_WIN32)
#else       // defined(VGPU_SHARED_LIBRARY)
#   define VGPU_API
#endif  // defined(VGPU_SHARED_LIBRARY)

#if defined(_WIN32)
#   define VGPU_CALL __cdecl
#else
#   define VGPU_CALL
#endif

#include <stdint.h>
#include <string>

#ifndef VGPU_ASSERT
#   include <assert.h>
#   define VGPU_ASSERT(c) assert(c)
#endif

#if !defined(VGPU_DEFINE_ENUM_FLAG_OPERATORS)
#define VGPU_DEFINE_ENUM_FLAG_OPERATORS(EnumType, UnderlyingEnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) { return (EnumType)((UnderlyingEnumType)(a) | (UnderlyingEnumType)(b)); } \
inline constexpr EnumType& operator |= (EnumType &a, EnumType b) { return a = a | b; } \
inline constexpr EnumType operator & (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) & ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator &= (EnumType &a, EnumType b) { return a = a & b; } \
inline constexpr EnumType operator ~ (EnumType a) { return EnumType(~((UnderlyingEnumType)a)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) ^ ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator ^= (EnumType &a, EnumType b) { return a = a ^ b; } \
inline constexpr bool any(EnumType a) { return ((UnderlyingEnumType)a) != 0; }
#endif

namespace vgpu
{
    /* Constants */
    static constexpr uint32_t kInvalidId = 0xFFffFFff;
    static constexpr uint32_t kNumInflightFrames = 2u;
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxCommandLists = 16u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;

    /* Handles*/
    struct BufferHandle { uint32_t id; bool isValid() const { return id != kInvalidId; } };
    struct TextureHandle { uint32_t id; bool isValid() const { return id != kInvalidId; } };
    struct ShaderHandle { uint32_t id; bool isValid() const { return id != kInvalidId; } };

    static constexpr BufferHandle kInvalidBuffer = { kInvalidId };
    static constexpr TextureHandle kInvalidTexture = { kInvalidId };
    static constexpr ShaderHandle kInvalidShader = { kInvalidId };
    using CommandList = uint8_t;

    /* Enums */
    enum class LogLevel : uint32_t {
        Error,
        Warn,
        Info,
        Debug
    };

    enum class BackendType : uint32_t {
        Null,
        Vulkan,
        Direct3D11,
        Count
    };

    enum class GPUVendorId : uint32_t
    {
        None = 0,
        AMD = 0x1002,
        Intel = 0x8086,
        Nvidia = 0x10DE,
        ARM = 0x13B5,
        ImgTec = 0x1010,
        Qualcomm = 0x5143
    };

    /// Defines pixel format.
    enum class PixelFormat : uint32_t {
        Invalid = 0,
        // 8-bit pixel formats
        R8Unorm,
        R8Snorm,
        R8Uint,
        R8Sint,
        // 16-bit pixel formats
        R16Uint,
        R16Sint,
        R16Float,
        RG8Unorm,
        RG8Snorm,
        RG8Uint,
        RG8Sint,
        // 32-bit pixel formats
        R32Uint,
        R32Sint,
        R32Float,
        RG16Uint,
        RG16Sint,
        RG16Float,
        RGBA8Unorm,
        RGBA8UnormSrgb,
        RGBA8Snorm,
        RGBA8Uint,
        RGBA8Sint,
        BGRA8Unorm,
        BGRA8UnormSrgb,
        // Packed 32-Bit Pixel formats
        RGB10A2Unorm,
        RG11B10Float,
        // 64-Bit Pixel Formats
        RG32Uint,
        RG32Sint,
        RG32Float,
        RGBA16Uint,
        RGBA16Sint,
        RGBA16Float,
        // 128-Bit Pixel Formats
        RGBA32Uint,
        RGBA32Sint,
        RGBA32Float,
        // Depth-stencil
        Depth16Unorm,
        Depth32Float,
        Depth24UnormStencil8,
        // Compressed BC formats
        BC1RGBAUnorm,
        BC1RGBAUnormSrgb,
        BC2RGBAUnorm,
        BC2RGBAUnormSrgb,
        BC3RGBAUnorm,
        BC3RGBAUnormSrgb,
        BC4RUnorm,
        BC4RSnorm,
        BC5RGUnorm,
        BC5RGSnorm,
        BC6HRGBUfloat,
        BC6HRGBSfloat,
        BC7RGBAUnorm,
        BC7RGBAUnormSrgb,

        Count,
    };

    /// Defines pixel format type.
    enum class PixelFormatType {
        /// Unknown format Type
        Unknown,
        /// floating-point formats.
        Float,
        /// Unsigned normalized formats.
        Unorm,
        /// Unsigned normalized SRGB formats
        UnormSrgb,
        /// Signed normalized formats.
        Snorm,
        /// Unsigned integer formats.
        Uint,
        /// Signed integer formats.
        Sint
    };

    enum class TextureType : uint32_t {
        Type2D,
        Type3D,
        TypeCube
    };

    enum class TextureUsage : uint32_t
    {
        None = 0,
        Sampled = (1 << 0),
        Storage = (1 << 1),
        RenderTarget = (1 << 2)
    };
    VGPU_DEFINE_ENUM_FLAG_OPERATORS(TextureUsage, uint32_t);

    enum class BufferUsage : uint32_t
    {
        None = 0,
        Uniform = (1 << 0),
        Vertex = (1 << 1),
        Index = (1 << 2)
    };
    VGPU_DEFINE_ENUM_FLAG_OPERATORS(BufferUsage, uint32_t);

    enum class ShaderStage : uint32_t {
        Vertex,
        Fragment,
        Compute,
        Count
    };

    enum class LoadOp : uint8_t {
        Clear,
        Load
    };

    enum class PresentInterval : uint32_t
    {
        Default,
        One,
        Two,
        Immediate
    };


    /* Structs */
    typedef struct vgpu_color {
        float r;
        float g;
        float b;
        float a;
    } vgpu_color;

    struct TextureDescription {
        TextureType type = TextureType::Type2D;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        uint32_t width = 1u;
        uint32_t height = 1u;
        uint32_t depth = 1u;
        uint32_t arraySize = 1u;
        uint32_t mipLevels = 1u;
        uint32_t sampleCount = 1u;
        TextureUsage usage = TextureUsage::Sampled;
        const char* label;
    };

    struct ShaderBlob
    {
        uint64_t size;
        uint8_t* data;
    };

    struct ShaderDescription {
        ShaderBlob stages[(uint32_t)ShaderStage::Count];
        const char* label;
    };

    typedef struct vgpu_attachment_info {
        TextureHandle texture;
        uint32_t level;
        uint32_t slice;
    } vgpu_attachment_info;

    typedef struct vgpu_color_attachment_info {
        TextureHandle texture;
        uint32_t level;
        uint32_t slice;
        LoadOp load_op;
        vgpu_color clear_color;
    } vgpu_color_attachment_info;

    typedef struct vgpu_depth_stencil_pass_info {
        LoadOp depth_load_op;
        float clear_depth;
        LoadOp stencil_load_op;
        uint8_t clear_stencil;
    } vgpu_depth_stencil_pass_info;

    typedef struct vgpu_framebuffer_info {
        uint32_t width;
        uint32_t height;
        uint32_t layers;
        vgpu_attachment_info color_attachments[kMaxColorAttachments];
        vgpu_attachment_info depth_stencil;
    } vgpu_framebuffer_info;

    enum class InitFlags : uint32_t {
        None = 0,
        DebugRutime = (1 << 0),
        GPUBasedValidation = (1 << 1),
        GPUPreferenceLowPower = (1 << 2),
        RenderDoc = (1 << 3)
    };
    VGPU_DEFINE_ENUM_FLAG_OPERATORS(InitFlags, uint32_t);

    typedef struct vgpu_allocation_callbacks {
        void* user_data;
        void* (VGPU_CALL* allocate)(void* user_data, size_t size);
        void* (VGPU_CALL* allocate_cleared)(void* user_data, size_t size);
        void (VGPU_CALL* free)(void* user_data, void* ptr);
    } vgpu_allocation_callbacks;

    struct Features {
        bool independentBlend;
        bool computeShader;
        bool tessellationShader;
        bool logicOp;
        bool multiViewport;
        bool indexUInt32;
        bool multiDrawIndirect;
        bool fillModeNonSolid;
        bool samplerAnisotropy;
        bool textureCompressionETC2;
        bool textureCompressionASTC_LDR;
        bool textureCompressionBC;
        bool textureCubeArray;
        bool raytracing;
    };

    struct Limits {
        uint32_t    maxVertexAttributes;
        uint32_t    maxVertexBindings;
        uint32_t    maxVertexAttributeOffset;
        uint32_t    maxVertexBindingStride;
        uint32_t    maxTextureDimension1D;
        uint32_t    maxTextureDimension2D;
        uint32_t    maxTextureDimension3D;
        uint32_t    maxTextureDimensionCube;
        uint32_t    maxTextureArraySize;
        uint32_t    maxColorAttachments;
        uint32_t    maxUniformBufferSize;
        uint32_t    minUniformBufferOffsetAlignment;
        uint32_t    maxStorageBufferSize;
        uint32_t    minStorageBufferOffsetAlignment;
        uint32_t    maxSamplerAnisotropy;
        uint32_t    maxViewports;
        uint32_t    maxViewportWidth;
        uint32_t    maxViewportHeight;
        uint32_t    maxTessellationPatchSize;
        float       pointSizeRangeMin;
        float       pointSizeRangeMax;
        float       lineWidthRangeMin;
        float       lineWidthRangeMax;
        uint32_t    maxComputeSharedMemorySize;
        uint32_t    maxComputeWorkGroupCountX;
        uint32_t    maxComputeWorkGroupCountY;
        uint32_t    maxComputeWorkGroupCountZ;
        uint32_t    maxComputeWorkGroupInvocations;
        uint32_t    maxComputeWorkGroupSizeX;
        uint32_t    maxComputeWorkGroupSizeY;
        uint32_t    maxComputeWorkGroupSizeZ;
    };

    struct Caps {
        BackendType backendType;
        GPUVendorId vendorId;
        uint32_t deviceId;
        std::string adapterName;
        Features features;
        Limits limits;
    };

    struct PresentationParameters {
        uint32_t backbufferWidth;
        uint32_t backbufferHeight;
        PixelFormat backbufferFormat = PixelFormat::BGRA8Unorm;
        PixelFormat depthStencilFormat = PixelFormat::Invalid;
        PresentInterval presentInterval = PresentInterval::One;
        void* windowHandle;
        void* display = nullptr;
    };

    /* Log functions */
    typedef void(VGPU_CALL* LogCallback)(void* user_data, LogLevel level, const char* message);

    VGPU_API void setLogCallback(LogCallback callback, void* userData);
    VGPU_API void log(LogLevel level, const char* format, ...);
    VGPU_API void logError(const char* format, ...);
    VGPU_API void logInfo(const char* format, ...);

    VGPU_API bool init(InitFlags flags, const PresentationParameters& presentationParameters);
    VGPU_API void shutdown(void);
    VGPU_API void beginFrame(void);
    VGPU_API void endFrame(void);
    VGPU_API const Caps* queryCaps();

    /* Resource creation methods */
    VGPU_API TextureHandle createTexture(const TextureDescription& desc, const void* initialData = nullptr);
    VGPU_API void destroyTexture(TextureHandle handle);

    VGPU_API BufferHandle CreateBuffer(uint32_t size, BufferUsage usage, uint32_t stride = 0, const void* initialData = nullptr);
    VGPU_API void DestroyBuffer(BufferHandle handle);
    VGPU_API void* MapBuffer(BufferHandle handle);
    VGPU_API void UnmapBuffer(BufferHandle handle);

    VGPU_API ShaderBlob CompileShader(const char* source, const char* entryPoint, ShaderStage stage);
    VGPU_API ShaderHandle CreateShader(const ShaderDescription& desc);
    VGPU_API void DestroyShader(ShaderHandle handle);

    /* Commands */
    VGPU_API void cmdSetViewport(CommandList commandList, float x, float y, float width, float height, float min_depth = 0.0f, float max_depth = 1.0f);
    VGPU_API void SetScissorRect(CommandList commandList, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    VGPU_API void SetVertexBuffer(CommandList commandList, BufferHandle buffer);
    VGPU_API void SetIndexBuffer(CommandList commandList, BufferHandle buffer);
    VGPU_API void SetShader(CommandList commandList, ShaderHandle shader);

    VGPU_API void BindUniformBuffer(CommandList commandList, uint32_t slot, BufferHandle handle);
    VGPU_API void BindTexture(CommandList commandList, uint32_t slot, TextureHandle handle);
    VGPU_API void DrawIndexed(CommandList commandList, uint32_t indexCount, uint32_t startIndex, int32_t baseVertex);

    /* pixel format helpers */
    struct PixelFormatDesc
    {
        PixelFormat format;
        const char* name;
        uint32_t bytesPerBlock;
        uint32_t channelCount;
        PixelFormatType type;
        struct
        {
            bool isDepth;
            bool isStencil;
            bool isCompressed;
        };
        struct
        {
            uint32_t width;
            uint32_t height;
        } compressionRatio;
        int numChannelBits[4];
    };

    extern const VGPU_API PixelFormatDesc kFormatDesc[];

    /// Get the number of bytes per format
    inline uint32_t getFormatBytesPerBlock(PixelFormat format)
    {
        VGPU_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].bytesPerBlock;
    }

    inline uint32_t getFormatPixelsPerBlock(PixelFormat format)
    {
        VGPU_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width * kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    /// Check if the format has a depth component
    inline bool isDepthFormat(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isDepth;
    }

    /// Check if the format has a stencil component
    inline bool isStencilFormat(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isStencil;
    }

    /// Check if the format has depth or stencil components
    inline bool isDepthStencilFormat(PixelFormat format)
    {
        return isDepthFormat(format) || isStencilFormat(format);
    }

    /// Check if the format is a compressed format
    inline bool isCompressedFormat(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isCompressed;
    }

    /// Get the format compression ration along the x-axis.
    inline uint32_t getFormatWidthCompressionRatio(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width;
    }

    /// Get the format compression ration along the y-axis
    inline uint32_t getFormatHeightCompressionRatio(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    /// Get the number of channels
    inline uint32_t getFormatChannelCount(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].channelCount;
    }

    /** Get the format Type
    */
    inline PixelFormatType getFormatType(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].type;
    }

    inline uint32_t getNumChannelBits(PixelFormat format, int channel)
    {
        return kFormatDesc[(uint32_t)format].numChannelBits[channel];
    }

    /// Check if a format represents sRGB color space
    inline bool isSrgbFormat(PixelFormat format)
    {
        return (getFormatType(format) == PixelFormatType::UnormSrgb);
    }

    /// Convert an SRGB format to linear. If the format is already linear, will return it.
    inline PixelFormat srgbToLinearFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::BC1RGBAUnormSrgb:
            return PixelFormat::BC1RGBAUnorm;
        case PixelFormat::BC2RGBAUnormSrgb:
            return PixelFormat::BC2RGBAUnorm;
        case PixelFormat::BC3RGBAUnormSrgb:
            return PixelFormat::BC3RGBAUnorm;
        case PixelFormat::BGRA8UnormSrgb:
            return PixelFormat::BGRA8Unorm;
        case PixelFormat::RGBA8UnormSrgb:
            return PixelFormat::RGBA8Unorm;
        case PixelFormat::BC7RGBAUnormSrgb:
            return PixelFormat::BC7RGBAUnorm;
        default:
            VGPU_ASSERT(isSrgbFormat(format) == false);
            return format;
        }
    }

    /// Convert an linear format to sRGB. If the format doesn't have a matching sRGB format, will return the original.
    inline PixelFormat linearToSrgbFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::BC1RGBAUnorm:
            return PixelFormat::BC1RGBAUnormSrgb;
        case PixelFormat::BC2RGBAUnorm:
            return PixelFormat::BC2RGBAUnormSrgb;
        case PixelFormat::BC3RGBAUnorm:
            return PixelFormat::BC3RGBAUnormSrgb;
        case PixelFormat::BGRA8Unorm:
            return PixelFormat::BGRA8UnormSrgb;
        case PixelFormat::RGBA8Unorm:
            return PixelFormat::RGBA8UnormSrgb;
        case PixelFormat::BC7RGBAUnorm:
            return PixelFormat::BC7RGBAUnormSrgb;
        default:
            return format;
        }
    }
}
