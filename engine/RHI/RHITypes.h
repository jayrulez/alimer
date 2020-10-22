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

#pragma once

#include "Core/Assert.h"
#include "Core/Hash.h"
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <string>

using namespace DirectX;

namespace Alimer
{
	struct Shader;
	struct GPUResource;
	class GraphicsBuffer;
    class Sampler;
	struct Texture;
	struct RootSignature;
    using CommandList = uint8_t;

    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;
    static constexpr uint32_t kMaxViewportAndScissorRects = 8u;
    static constexpr CommandList kCommanstListCount = 16;

    static constexpr uint32_t KnownVendorId_AMD = 0x1002;
    static constexpr uint32_t KnownVendorId_Intel = 0x8086;
    static constexpr uint32_t KnownVendorId_Nvidia = 0x10DE;
    static constexpr uint32_t KnownVendorId_Microsoft = 0x1414;
    static constexpr uint32_t KnownVendorId_ARM = 0x13B5;
    static constexpr uint32_t KnownVendorId_ImgTec = 0x1010;
    static constexpr uint32_t KnownVendorId_Qualcomm = 0x5143;

    /// Enum describing the rendering backend.
    enum class GraphicsBackendType : uint32
    {
        /// Direct3D 12 backend.
        Direct3D12,
        /// Direct3D 11 backend.
        Direct3D11,
        /// Vulkan backend.
        Vulkan,
        /// Default best platform supported backend.
        Count
    };

	enum class ShaderStage : uint32_t
	{
		Vertex,
		Hull,
		Domain,
		Geometry,
		Fragment,
		Compute,
        Amplification,
        Mesh,
		Count,
	};

	enum class PrimitiveTopology : uint32_t
	{
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
		PatchList,
	};

	enum class CompareFunction : uint32_t
	{
        Undefined = 0,
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
	};

    enum class StencilOperation : uint32_t
    {
        Keep,
        Zero,
        Replace,
        IncrementClamp,
        DecrementClamp,
		Invert,
        IncrementWrap,
        DecrementWrap,
	};

	enum class BlendFactor : uint32_t
	{
        Zero,
        One,
        SourceColor,
        OneMinusSourceColor,
		SourceAlpha,
        OneMinusSourceAlpha,
        DestinationColor,
        OneMinusDestinationColor,
        DestinationAlpha,
        OneMinusDestinationAlpha,
        SourceAlphaSaturated,
		BlendColor,
		OneMinusBlendColor,
		Source1Color,
        OneMinusSource1Color,
        Source1Alpha,
        OneMinusSource1Alpha,
	};

    enum class BlendOperation : uint32_t
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };

	enum class ColorWriteMask : uint32_t
	{
        None = 0x00000000,
        Red = 0x00000001,
        Green = 0x00000002,
        Blue = 0x00000004,
        Alpha = 0x00000008,
        All = 0x0000000F,
	};
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(ColorWriteMask, uint32_t);

    enum class FrontFace : uint32_t
    {
        CCW,
        CW,
    };

    enum class CullMode : uint32_t
    {
        None,
        Front,
        Back,
    };

	enum class InputStepMode : uint32_t
	{
		Vertex,
		Instance,
	};

	enum USAGE
	{
		USAGE_DEFAULT,
		USAGE_IMMUTABLE,
		USAGE_DYNAMIC,
		USAGE_STAGING,
	};

    enum class SamplerAddressMode : uint32_t
    {
        Wrap,
        Mirror,
        Clamp,
        Border
    };

	enum class FilterMode : uint32_t
	{
        Nearest,
        Linear
	};

    enum class SamplerBorderColor : uint32_t
    {
        TransparentBlack,
        OpaqueBlack,
        OpaqueWhite,
    };

	enum class PixelFormat : uint32_t
	{
        Invalid = 0,

		FORMAT_R32G32B32A32_FLOAT,
		FORMAT_R32G32B32A32_UINT,
		FORMAT_R32G32B32A32_SINT,

		FORMAT_R32G32B32_FLOAT,
		FORMAT_R32G32B32_UINT,
		FORMAT_R32G32B32_SINT,

		FORMAT_R16G16B16A16_FLOAT,
		FORMAT_R16G16B16A16_UNORM,
		FORMAT_R16G16B16A16_UINT,
		FORMAT_R16G16B16A16_SNORM,
		FORMAT_R16G16B16A16_SINT,

		FORMAT_R32G32_FLOAT,
		FORMAT_R32G32_UINT,
		FORMAT_R32G32_SINT,
		FORMAT_R32G8X24_TYPELESS,		// depth + stencil (alias)
		FORMAT_D32_FLOAT_S8X24_UINT,	// depth + stencil

		FORMAT_R10G10B10A2_UNORM,
		FORMAT_R10G10B10A2_UINT,
		FORMAT_R11G11B10_FLOAT,
		FORMAT_R8G8B8A8_UNORM,
		FORMAT_R8G8B8A8_UNORM_SRGB,
		FORMAT_R8G8B8A8_UINT,
		FORMAT_R8G8B8A8_SNORM,
		FORMAT_R8G8B8A8_SINT, 
		FORMAT_B8G8R8A8_UNORM,
		FORMAT_B8G8R8A8_UNORM_SRGB,
		FORMAT_R16G16_FLOAT,
		FORMAT_R16G16_UNORM,
		FORMAT_R16G16_UINT,
		FORMAT_R16G16_SNORM,
		FORMAT_R16G16_SINT,
		FORMAT_R32_TYPELESS,			// depth (alias)
		FORMAT_D32_FLOAT,				// depth
		FORMAT_R32_FLOAT,
		FORMAT_R32_UINT,
		FORMAT_R32_SINT, 
		FORMAT_R24G8_TYPELESS,			// depth + stencil (alias)
		FORMAT_D24_UNORM_S8_UINT,		// depth + stencil

		FORMAT_R8G8_UNORM,
		FORMAT_R8G8_UINT,
		FORMAT_R8G8_SNORM,
		FORMAT_R8G8_SINT,
		FORMAT_R16_TYPELESS,			// depth (alias)
		FORMAT_R16_FLOAT,
		FORMAT_D16_UNORM,				// depth
		FORMAT_R16_UNORM,
		FORMAT_R16_UINT,
		FORMAT_R16_SNORM,
		FORMAT_R16_SINT,

		FORMAT_R8_UNORM,
		FORMAT_R8_UINT,
		FORMAT_R8_SNORM,
		FORMAT_R8_SINT,

		FORMAT_BC1_UNORM,
		FORMAT_BC1_UNORM_SRGB,
		FORMAT_BC2_UNORM,
		FORMAT_BC2_UNORM_SRGB,
		FORMAT_BC3_UNORM,
		FORMAT_BC3_UNORM_SRGB,
		FORMAT_BC4_UNORM,
		FORMAT_BC4_SNORM,
		FORMAT_BC5_UNORM,
		FORMAT_BC5_SNORM,
		FORMAT_BC6H_UF16,
		FORMAT_BC6H_SF16,
		FORMAT_BC7_UNORM,
		FORMAT_BC7_UNORM_SRGB
	};

	enum GPU_QUERY_TYPE
	{
		GPU_QUERY_TYPE_INVALID,				// do not use! Indicates if query was not created.
		GPU_QUERY_TYPE_EVENT,				// has the GPU reached this point?
		GPU_QUERY_TYPE_OCCLUSION,			// how many samples passed depthstencil test?
		GPU_QUERY_TYPE_OCCLUSION_PREDICATE, // are there any samples that passed depthstencil test
		GPU_QUERY_TYPE_TIMESTAMP,			// retrieve time point of gpu execution
		GPU_QUERY_TYPE_TIMESTAMP_DISJOINT,	// timestamp frequency information
	};

    enum class VertexFormat : uint32_t
    {
        Invalid,
        UChar2,
        UChar4,
        Char2,
        Char4,
        UChar2Norm,
        UChar4Norm,
        Char2Norm,
        Char4Norm,
        UShort2,
        UShort4,
        Short2,
        Short4,
        UShort2Norm,
        UShort4Norm,
        Short2Norm,
        Short4Norm,
        Half2,
        Half4,
        Float,
        Float2,
        Float3,
        Float4,
        UInt,
        UInt2,
        UInt3,
        UInt4,
        Int,
        Int2,
        Int3,
        Int4,
    };

	enum class IndexFormat : uint32_t
    {
        UInt16 = 0x00000000,
        UInt32 = 0x00000001,
	};

	enum SUBRESOURCE_TYPE
	{
		SRV,
		UAV,
		RTV,
		DSV,
	};
	enum IMAGE_LAYOUT
	{
		IMAGE_LAYOUT_UNDEFINED,					// discard contents
		IMAGE_LAYOUT_GENERAL,					// supports everything
		IMAGE_LAYOUT_RENDERTARGET,				// render target, write enabled
		IMAGE_LAYOUT_DEPTHSTENCIL,				// depth stencil, write enabled
		IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,		// depth stencil, read only
		IMAGE_LAYOUT_SHADER_RESOURCE,			// shader resource, read only
		IMAGE_LAYOUT_UNORDERED_ACCESS,			// shader resource, write enabled
		IMAGE_LAYOUT_COPY_SRC,					// copy from
		IMAGE_LAYOUT_COPY_DST,					// copy to
		IMAGE_LAYOUT_SHADING_RATE_SOURCE,		// shading rate control per tile
	};
	enum BUFFER_STATE
	{
		BUFFER_STATE_GENERAL,					// supports everything
		BUFFER_STATE_VERTEX_BUFFER,				// vertex buffer, read only
		BUFFER_STATE_INDEX_BUFFER,				// index buffer, read only
		BUFFER_STATE_CONSTANT_BUFFER,			// constant buffer, read only
		BUFFER_STATE_INDIRECT_ARGUMENT,			// argument buffer to DrawIndirect() or DispatchIndirect()
		BUFFER_STATE_SHADER_RESOURCE,			// shader resource, read only
		BUFFER_STATE_UNORDERED_ACCESS,			// shader resource, write enabled
		BUFFER_STATE_COPY_SRC,					// copy from
		BUFFER_STATE_COPY_DST,					// copy to
		BUFFER_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
	};

	enum class ShadingRate : uint32_t
	{
		Rate_1X1,
		Rate_1X2,
		Rate_2X1,
		Rate_2X2,
		Rate_2X4,
		Rate_4X2,
		Rate_4X4
	};

	enum GRAPHICSDEVICE_CAPABILITY
	{
		GRAPHICSDEVICE_CAPABILITY_TESSELLATION,
		GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION,
		GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS,
		GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_COMMON, // eg: R16G16B16A16_FLOAT, R8G8B8A8_UNORM and more common ones
		GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT,
		GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS,
		GRAPHICSDEVICE_CAPABILITY_RAYTRACING,
		GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE,
		GRAPHICSDEVICE_CAPABILITY_DESCRIPTOR_MANAGEMENT,
		GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING,
		GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2,
		GRAPHICSDEVICE_CAPABILITY_MESH_SHADER,
		GRAPHICSDEVICE_CAPABILITY_COUNT,
	};

	// Flags ////////////////////////////////////////////
	enum BIND_FLAG
	{
		BIND_VERTEX_BUFFER = 1 << 0,
		BIND_INDEX_BUFFER = 1 << 1,
		BIND_CONSTANT_BUFFER = 1 << 2,
		BIND_SHADER_RESOURCE = 1 << 3,
		BIND_STREAM_OUTPUT = 1 << 4,
		BIND_RENDER_TARGET = 1 << 5,
		BIND_DEPTH_STENCIL = 1 << 6,
		BIND_UNORDERED_ACCESS = 1 << 7,
	};
	enum CPU_ACCESS
	{
		CPU_ACCESS_WRITE = 1 << 0,
		CPU_ACCESS_READ = 1 << 1,
	};
	enum RESOURCE_MISC_FLAG
	{
		RESOURCE_MISC_SHARED = 1 << 0,
		RESOURCE_MISC_TEXTURECUBE = 1 << 1,
		RESOURCE_MISC_INDIRECT_ARGS = 1 << 2,
		RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS = 1 << 3,
		RESOURCE_MISC_BUFFER_STRUCTURED = 1 << 4,
		RESOURCE_MISC_TILED = 1 << 5,
		RESOURCE_MISC_RAY_TRACING = 1 << 0,
	};

	// Descriptor structs:

	struct Viewport
	{
		float TopLeftX = 0.0f;
		float TopLeftY = 0.0f;
		float Width = 0.0f;
		float Height = 0.0f;
		float MinDepth = 0.0f;
		float MaxDepth = 1.0f;
	};

	union ClearValue
	{
		float color[4];
		struct ClearDepthStencil
		{
			float depth;
			uint32_t stencil;
		} depthstencil;
	};

	struct TextureDesc
	{
		enum TEXTURE_TYPE
		{
			TEXTURE_1D,
			TEXTURE_2D,
			TEXTURE_3D,
		} type = TEXTURE_2D;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Depth = 1;
		uint32_t ArraySize = 1;
		uint32_t MipLevels = 1;
        PixelFormat format = PixelFormat::FORMAT_R8G8B8A8_UNORM;
		uint32_t SampleCount = 1;
		USAGE Usage = USAGE_DEFAULT;
		uint32_t BindFlags = 0;
		uint32_t CPUAccessFlags = 0;
		uint32_t MiscFlags = 0;
		ClearValue clear = {};
		IMAGE_LAYOUT layout = IMAGE_LAYOUT_GENERAL;
	};

	struct SamplerDescriptor
	{
        FilterMode magFilter = FilterMode::Linear;
        FilterMode minFilter = FilterMode::Linear;
        FilterMode mipmapFilter = FilterMode::Linear;
        SamplerAddressMode addressModeU = SamplerAddressMode::Wrap;
        SamplerAddressMode addressModeV = SamplerAddressMode::Wrap;
        SamplerAddressMode addressModeW = SamplerAddressMode::Wrap;
        float mipLodBias;
		uint32_t maxAnisotropy = 1u;
        CompareFunction compareFunction = CompareFunction::Undefined;
        float lodMinClamp = 0.0f;
        float lodMaxClamp = FLT_MAX;
        SamplerBorderColor borderColor = SamplerBorderColor::TransparentBlack;
        const char* label;
	};

	struct GPUBufferDesc
	{
		uint32_t ByteWidth = 0;
		USAGE Usage = USAGE_DEFAULT;
		uint32_t BindFlags = 0;
		uint32_t CPUAccessFlags = 0;
		uint32_t MiscFlags = 0;
		uint32_t StructureByteStride = 0; // needed for typed and structured buffer types!
        PixelFormat format = PixelFormat::Invalid; // only needed for typed buffer!
	};

	struct GPUQueryDesc
	{
		GPU_QUERY_TYPE Type = GPU_QUERY_TYPE_INVALID;
	};

	struct GPUQueryResult
	{
		uint64_t	result_passed_sample_count = 0;
		uint64_t	result_timestamp = 0;
		uint64_t	result_timestamp_frequency = 0;
	};

    struct VertexAttributeDescriptor
    {
        VertexFormat format;
        uint32_t offset;
        uint32_t bufferIndex;
    };

    struct VertexBufferLayoutDescriptor
    {
        uint32_t        stride = 0;
        InputStepMode   stepMode = InputStepMode::Vertex;
    };

    struct VertexDescriptor
    {
        VertexAttributeDescriptor       attributes[kMaxVertexAttributes];
        VertexBufferLayoutDescriptor    layouts[kMaxVertexBufferBindings];
    };

    struct RasterizationStateDescriptor
    {
        FrontFace frontFace = FrontFace::CCW;
        CullMode cullMode = CullMode::Back;
        int32_t depthBias = 0;
        float depthBiasSlopeScale = 0.0f;
        float depthBiasClamp = 0.0f;
        bool depthClipEnable = true;
        bool conservativeRasterizationEnable = false;
        uint32_t forcedSampleCount = 0;
    };

    struct StencilStateFaceDescriptor
    {
        StencilOperation failOp = StencilOperation::Keep;
        StencilOperation depthFailOp = StencilOperation::Keep;
        StencilOperation passOp = StencilOperation::Keep;
        CompareFunction compare = CompareFunction::Always;
    };

    struct DepthStencilStateDescriptor
    {
        bool                        depthWriteEnabled = true;
        CompareFunction             depthCompare = CompareFunction::Less;
        StencilStateFaceDescriptor  stencilFront;
        StencilStateFaceDescriptor  stencilBack;
        uint8_t                     stencilReadMask = 0xff;
        uint8_t                     stencilWriteMask = 0xff;
    };

    struct ColorAttachmentDescriptor
    {
        PixelFormat format;
        bool blendEnable = false;
        BlendFactor srcColorBlendFactor = BlendFactor::One;
        BlendFactor dstColorBlendFactor = BlendFactor::Zero;
        BlendOperation colorBlendOp = BlendOperation::Add;
        BlendFactor srcAlphaBlendFactor = BlendFactor::One;
        BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;
        BlendOperation alphaBlendOp = BlendOperation::Add;
        ColorWriteMask colorWriteMask = ColorWriteMask::All;
    };

	struct RenderPipelineDescriptor
	{
		const RootSignature*            rootSignature = nullptr;
		const Shader*				    vs = nullptr;
		const Shader*				    ps = nullptr;
		const Shader*				    hs = nullptr;
		const Shader*				    ds = nullptr;
		const Shader*				    gs = nullptr;
		const Shader*				    ms = nullptr;
		const Shader*				    as = nullptr;
        VertexDescriptor                vertexDescriptor;
        PrimitiveTopology			    primitiveTopology = PrimitiveTopology::TriangleList;
        RasterizationStateDescriptor    rasterizationState;
		DepthStencilStateDescriptor	    depthStencilState;
        ColorAttachmentDescriptor       colorAttachments[kMaxColorAttachments];
        uint32_t                        sampleCount = 1;
        uint32_t                        sampleMask = 0xFFFFFFFF;
        bool                            alphaToCoverageEnable = false;
	};

	struct GPUBarrier
	{
		enum TYPE
		{
			MEMORY_BARRIER,		// UAV accesses
			IMAGE_BARRIER,		// image layout transition
			BUFFER_BARRIER,		// buffer state transition
		} type = MEMORY_BARRIER;

		struct Memory
		{
			const GPUResource* resource;
		};
		struct Image
		{
			const Texture* texture;
			IMAGE_LAYOUT layout_before;
			IMAGE_LAYOUT layout_after;
		};
		struct Buffer
		{
			const GraphicsBuffer* buffer;
			BUFFER_STATE state_before;
			BUFFER_STATE state_after;
		};
		union
		{
			Memory memory;
			Image image;
			Buffer buffer;
		};

		static GPUBarrier Memory(const GPUResource* resource = nullptr)
		{
			GPUBarrier barrier;
			barrier.type = MEMORY_BARRIER;
			barrier.memory.resource = resource;
			return barrier;
		}
		static inline GPUBarrier Image(const Texture* texture, IMAGE_LAYOUT before, IMAGE_LAYOUT after)
		{
			GPUBarrier barrier;
			barrier.type = IMAGE_BARRIER;
			barrier.image.texture = texture;
			barrier.image.layout_before = before;
			barrier.image.layout_after = after;
			return barrier;
		}

		static inline GPUBarrier Buffer(const GraphicsBuffer* buffer, BUFFER_STATE before, BUFFER_STATE after)
		{
			GPUBarrier barrier;
			barrier.type = BUFFER_BARRIER;
			barrier.buffer.buffer = buffer;
			barrier.buffer.state_before = before;
			barrier.buffer.state_after = after;
			return barrier;
		}
	};
	struct RenderPassAttachment
	{
		enum TYPE
		{
			RENDERTARGET,
			DEPTH_STENCIL,
			RESOLVE,
		} type = RENDERTARGET;
		enum LOAD_OPERATION
		{
			LOADOP_LOAD,
			LOADOP_CLEAR,
			LOADOP_DONTCARE,
		} loadop = LOADOP_LOAD;
		const Texture* texture = nullptr;
		int subresource = -1;
		enum STORE_OPERATION
		{
			STOREOP_STORE,
			STOREOP_DONTCARE,
		} storeop = STOREOP_STORE;
		IMAGE_LAYOUT initial_layout = IMAGE_LAYOUT_GENERAL;		// layout before the render pass
		IMAGE_LAYOUT final_layout = IMAGE_LAYOUT_GENERAL;		// layout after the render pass
		IMAGE_LAYOUT subpass_layout = IMAGE_LAYOUT_RENDERTARGET;// layout within the render pass

		static RenderPassAttachment RenderTarget(
			const Texture* resource = nullptr,
			LOAD_OPERATION load_op = LOADOP_LOAD,
			STORE_OPERATION store_op = STOREOP_STORE,
			IMAGE_LAYOUT initial_layout = IMAGE_LAYOUT_GENERAL,
			IMAGE_LAYOUT subpass_layout = IMAGE_LAYOUT_RENDERTARGET,
			IMAGE_LAYOUT final_layout = IMAGE_LAYOUT_GENERAL
		)
		{
			RenderPassAttachment attachment;
			attachment.type = RENDERTARGET;
			attachment.texture = resource;
			attachment.loadop = load_op;
			attachment.storeop = store_op;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = subpass_layout;
			attachment.final_layout = final_layout;
			return attachment;
		}

		static RenderPassAttachment DepthStencil(
			const Texture* resource = nullptr,
			LOAD_OPERATION load_op = LOADOP_LOAD,
			STORE_OPERATION store_op = STOREOP_STORE,
			IMAGE_LAYOUT initial_layout = IMAGE_LAYOUT_DEPTHSTENCIL,
			IMAGE_LAYOUT subpass_layout = IMAGE_LAYOUT_DEPTHSTENCIL,
			IMAGE_LAYOUT final_layout = IMAGE_LAYOUT_DEPTHSTENCIL
		)
		{
			RenderPassAttachment attachment;
			attachment.type = DEPTH_STENCIL;
			attachment.texture = resource;
			attachment.loadop = load_op;
			attachment.storeop = store_op;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = subpass_layout;
			attachment.final_layout = final_layout;
			return attachment;
		}

		static RenderPassAttachment Resolve(
			const Texture* resource = nullptr,
			IMAGE_LAYOUT initial_layout = IMAGE_LAYOUT_GENERAL,
			IMAGE_LAYOUT final_layout = IMAGE_LAYOUT_GENERAL
		)
		{
			RenderPassAttachment attachment;
			attachment.type = RESOLVE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.final_layout = final_layout;
			return attachment;
		}
	};
	struct RenderPassDesc
	{
		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_ALLOW_UAV_WRITES = 1 << 0,
		};
		uint32_t _flags = FLAG_EMPTY;
		std::vector<RenderPassAttachment> attachments;
	};
	struct IndirectDrawArgsInstanced
	{
		uint32_t VertexCountPerInstance = 0;
		uint32_t InstanceCount = 0;
		uint32_t StartVertexLocation = 0;
		uint32_t StartInstanceLocation = 0;
	};
	struct IndirectDrawArgsIndexedInstanced
	{
		uint32_t IndexCountPerInstance = 0;
		uint32_t InstanceCount = 0;
		uint32_t StartIndexLocation = 0;
		int32_t BaseVertexLocation = 0;
		uint32_t StartInstanceLocation = 0;
	};
	struct IndirectDispatchArgs
	{
		uint32_t ThreadGroupCountX = 0;
		uint32_t ThreadGroupCountY = 0;
		uint32_t ThreadGroupCountZ = 0;
	};
	struct SubresourceData
	{
		const void *pSysMem = nullptr;
		uint32_t SysMemPitch = 0;
		uint32_t SysMemSlicePitch = 0;
	};
	struct Rect
	{
		int32_t left = 0;
		int32_t top = 0;
		int32_t right = 0;
		int32_t bottom = 0;
	};
	struct Mapping
	{
		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_READ = 1 << 0,
			FLAG_WRITE = 1 << 1,
		};
		uint32_t _flags = FLAG_EMPTY;
		size_t offset = 0;
		size_t size = 0;
		uint32_t rowpitch = 0;	// output
		void* data = nullptr;	// output
	};


	// Resources:

	struct GraphicsDeviceChild
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }
	};

	struct Shader : public GraphicsDeviceChild
	{
        ShaderStage stage = ShaderStage::Count;
		std::vector<uint8_t> code;
		const RootSignature* rootSignature = nullptr;
	};

	struct GPUResource : public GraphicsDeviceChild
	{
		enum class GPU_RESOURCE_TYPE
		{
			BUFFER,
			TEXTURE,
			RAYTRACING_ACCELERATION_STRUCTURE,
			UNKNOWN_TYPE,
		} type = GPU_RESOURCE_TYPE::UNKNOWN_TYPE;
		inline bool IsTexture() const { return type == GPU_RESOURCE_TYPE::TEXTURE; }
		inline bool IsBuffer() const { return type == GPU_RESOURCE_TYPE::BUFFER; }
		inline bool IsAccelerationStructure() const { return type == GPU_RESOURCE_TYPE::RAYTRACING_ACCELERATION_STRUCTURE; }
	};

	struct Texture : public GPUResource
	{
		TextureDesc	desc;

		const TextureDesc& GetDesc() const { return desc; }
	};

	struct GPUQuery : public GraphicsDeviceChild
	{
		GPUQueryDesc desc;

		const GPUQueryDesc& GetDesc() const { return desc; }
	};

	struct PipelineState : public GraphicsDeviceChild
	{
		size_t hash = 0;
		RenderPipelineDescriptor desc;

		const RenderPipelineDescriptor& GetDesc() const { return desc; }
	};

	struct RenderPass : public GraphicsDeviceChild
	{
		size_t hash = 0;
		RenderPassDesc desc;

		const RenderPassDesc& GetDesc() const { return desc; }
	};


	struct RaytracingAccelerationStructureDesc
	{
		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_ALLOW_UPDATE = 1 << 0,
			FLAG_ALLOW_COMPACTION = 1 << 1,
			FLAG_PREFER_FAST_TRACE = 1 << 2,
			FLAG_PREFER_FAST_BUILD = 1 << 3,
			FLAG_MINIMIZE_MEMORY = 1 << 4,
		};
		uint32_t _flags = FLAG_EMPTY;

		enum TYPE
		{
			BOTTOMLEVEL,
			TOPLEVEL,
		} type = BOTTOMLEVEL;

		struct BottomLevel
		{
			struct Geometry
			{
				enum FLAGS
				{
					FLAG_EMPTY = 0,
					FLAG_OPAQUE = 1 << 0,
					FLAG_NO_DUPLICATE_ANYHIT_INVOCATION = 1 << 1,
					FLAG_USE_TRANSFORM = 1 << 2,
				};
				uint32_t _flags = FLAG_EMPTY;

				enum TYPE
				{
					TRIANGLES,
					PROCEDURAL_AABBS,
				} type = TRIANGLES;

				struct Triangles
				{
					GraphicsBuffer* vertexBuffer;
					GraphicsBuffer* indexBuffer;
					uint32_t indexCount = 0;
					uint32_t indexOffset = 0;
					uint32_t vertexCount = 0;
					uint32_t vertexByteOffset = 0;
					uint32_t vertexStride = 0;
					IndexFormat indexFormat = IndexFormat::UInt32;
                    VertexFormat vertexFormat = VertexFormat::Float3;
                    GraphicsBuffer* transform3x4Buffer;
					uint32_t transform3x4BufferOffset = 0;
				} triangles;
				struct Procedural_AABBs
				{
                    GraphicsBuffer* aabbBuffer;
					uint32_t offset = 0;
					uint32_t count = 0;
					uint32_t stride = 0;
				} aabbs;

			};
			std::vector<Geometry> geometries;
		} bottomlevel;

		struct TopLevel
		{
			struct Instance
			{
				XMFLOAT3X4 transform;
				uint32_t InstanceID : 24;
				uint32_t InstanceMask : 8;
				uint32_t InstanceContributionToHitGroupIndex : 24;
				uint32_t Flags : 8;
				GPUResource bottomlevel;
			};
            GraphicsBuffer* instanceBuffer;
			uint32_t offset = 0;
			uint32_t count = 0;
		} toplevel;
	};
	struct RaytracingAccelerationStructure : public GPUResource
	{
		RaytracingAccelerationStructureDesc desc;

		const RaytracingAccelerationStructureDesc& GetDesc() const { return desc; }
	};

	struct ShaderLibrary
	{
		enum TYPE
		{
			RAYGENERATION,
			MISS,
			CLOSESTHIT,
			ANYHIT,
			INTERSECTION,
		} type = RAYGENERATION;
		const Shader* shader = nullptr;
		std::string function_name;
	};
	struct ShaderHitGroup
	{
		enum TYPE
		{
			GENERAL, // raygen or miss
			TRIANGLES,
			PROCEDURAL,
		} type = TRIANGLES;
		std::string name;
		uint32_t general_shader = ~0;
		uint32_t closesthit_shader = ~0;
		uint32_t anyhit_shader = ~0;
		uint32_t intersection_shader = ~0;
	};
	struct RaytracingPipelineStateDesc
	{
		const RootSignature* rootSignature = nullptr;
		std::vector<ShaderLibrary> shaderlibraries;
		std::vector<ShaderHitGroup> hitgroups;
		uint32_t max_trace_recursion_depth = 1;
		uint32_t max_attribute_size_in_bytes = 0;
		uint32_t max_payload_size_in_bytes = 0;
	};
	struct RaytracingPipelineState : public GraphicsDeviceChild
	{
		RaytracingPipelineStateDesc desc;

		const RaytracingPipelineStateDesc& GetDesc() const { return desc; }
	};

	struct ShaderTable
	{
		const GraphicsBuffer* buffer = nullptr;
		uint64_t offset = 0;
		uint64_t size = 0;
		uint64_t stride = 0;
	};
	struct DispatchRaysDesc
	{
		ShaderTable raygeneration;
		ShaderTable miss;
		ShaderTable hitgroup;
		ShaderTable callable;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Depth = 1;
	};

	enum BINDPOINT
	{
		GRAPHICS,
		COMPUTE,
		RAYTRACING,
	};

	enum RESOURCEBINDING
	{
		ROOT_CONSTANTBUFFER,
		ROOT_RAWBUFFER,
		ROOT_STRUCTUREDBUFFER,
		ROOT_RWRAWBUFFER,
		ROOT_RWSTRUCTUREDBUFFER,

		CONSTANTBUFFER,
		RAWBUFFER,
		STRUCTUREDBUFFER,
		TYPEDBUFFER,
		TEXTURE1D,
		TEXTURE1DARRAY,
		TEXTURE2D,
		TEXTURE2DARRAY,
		TEXTURECUBE,
		TEXTURECUBEARRAY,
		TEXTURE3D,
		ACCELERATIONSTRUCTURE,
		RWRAWBUFFER,
		RWSTRUCTUREDBUFFER,
		RWTYPEDBUFFER,
		RWTEXTURE1D,
		RWTEXTURE1DARRAY,
		RWTEXTURE2D,
		RWTEXTURE2DARRAY,
		RWTEXTURE3D,

		RESOURCEBINDING_COUNT
	};
	struct ResourceRange
	{
		RESOURCEBINDING binding = CONSTANTBUFFER;
		uint32_t slot = 0;
		uint32_t count = 1;
	};
	struct SamplerRange
	{
		uint32_t slot = 0;
		uint32_t count = 1;
	};
	struct StaticSampler
	{
		Sampler* sampler;
		uint32_t slot = 0;
	};
	struct DescriptorTable : public GraphicsDeviceChild
	{
        ShaderStage stage = ShaderStage::Count;
		std::vector<ResourceRange> resources;
		std::vector<SamplerRange> samplers;
		std::vector<StaticSampler> staticsamplers;
	};
	struct RootConstantRange
	{
        ShaderStage stage = ShaderStage::Count;
		uint32_t slot = 0;
		uint32_t size = 0;
		uint32_t offset = 0;
	};
	struct RootSignature : public GraphicsDeviceChild
	{
		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT = 1 << 0,
		};
		uint32_t _flags = FLAG_EMPTY;
		std::vector<DescriptorTable> tables;
		std::vector<RootConstantRange> rootconstants;
	};

}

namespace std
{
    template<>
    struct hash<Alimer::RasterizationStateDescriptor>
    {
        std::size_t operator()(const Alimer::RasterizationStateDescriptor& desc) const noexcept
        {
            std::size_t hash = 0;
            Alimer::hash_combine(hash, (uint32_t)desc.frontFace);
            Alimer::hash_combine(hash, (uint32_t)desc.cullMode);
            Alimer::hash_combine(hash, desc.depthBias);
            Alimer::hash_combine(hash, desc.depthBiasSlopeScale);
            Alimer::hash_combine(hash, desc.depthBiasClamp);
            Alimer::hash_combine(hash, desc.depthClipEnable);
            Alimer::hash_combine(hash, desc.conservativeRasterizationEnable);
            Alimer::hash_combine(hash, desc.forcedSampleCount);
            return hash;
        }
    };

    template<>
    struct hash<Alimer::StencilStateFaceDescriptor>
    {
        std::size_t operator()(const Alimer::StencilStateFaceDescriptor& desc) const noexcept
        {
            std::size_t hash = 0;
            Alimer::hash_combine(hash, (uint32_t)desc.compare);
            Alimer::hash_combine(hash, (uint32_t)desc.failOp);
            Alimer::hash_combine(hash, (uint32_t)desc.depthFailOp);
            Alimer::hash_combine(hash, (uint32_t)desc.passOp);
            return hash;
        }
    };

    template<>
    struct hash<Alimer::DepthStencilStateDescriptor>
    {
        std::size_t operator()(const Alimer::DepthStencilStateDescriptor& desc) const noexcept
        {
            std::size_t hash = 0;
            Alimer::hash_combine(hash, desc.depthWriteEnabled);
            Alimer::hash_combine(hash, (uint32_t)desc.depthCompare);
            Alimer::hash_combine(hash, desc.stencilFront);
            Alimer::hash_combine(hash, desc.stencilBack);
            Alimer::hash_combine(hash, desc.stencilReadMask);
            Alimer::hash_combine(hash, desc.stencilWriteMask);
            return hash;
        }
    };

    template<>
    struct hash<Alimer::VertexBufferLayoutDescriptor>
    {
        std::size_t operator()(const Alimer::VertexBufferLayoutDescriptor& desc) const noexcept
        {
            std::size_t hash = 0;
            Alimer::hash_combine(hash, (uint32_t)desc.stride);
            Alimer::hash_combine(hash, (uint32_t)desc.stepMode);
            return hash;
        }
    };

    template<>
    struct hash<Alimer::VertexAttributeDescriptor>
    {
        std::size_t operator()(const Alimer::VertexAttributeDescriptor& desc) const noexcept
        {
            std::size_t hash = 0;
            Alimer::hash_combine(hash, (uint32_t)desc.format);
            Alimer::hash_combine(hash, desc.offset);
            Alimer::hash_combine(hash, desc.bufferIndex);
            return hash;
        }
    };

    template<>
    struct hash<Alimer::VertexDescriptor>
    {
        std::size_t operator()(const Alimer::VertexDescriptor& desc) const noexcept
        {
            std::size_t hash = 0;

            for (uint32_t i = 0; i < Alimer::kMaxVertexAttributes; ++i)
            {
                if (desc.attributes[i].format != Alimer::VertexFormat::Invalid)
                    break;

                Alimer::hash_combine(hash, desc.attributes[i]);
            }

            return hash;
        }
    };

    template<>
    struct hash<Alimer::ColorAttachmentDescriptor>
    {
        std::size_t operator()(const Alimer::ColorAttachmentDescriptor& desc) const noexcept
        {
            std::size_t hash = 0;
            Alimer::hash_combine(hash, (uint32_t)desc.format);
            Alimer::hash_combine(hash, desc.blendEnable);
            Alimer::hash_combine(hash, (uint32_t)desc.srcColorBlendFactor);
            Alimer::hash_combine(hash, (uint32_t)desc.dstColorBlendFactor);
            Alimer::hash_combine(hash, (uint32_t)desc.colorBlendOp);
            Alimer::hash_combine(hash, (uint32_t)desc.srcAlphaBlendFactor);
            Alimer::hash_combine(hash, (uint32_t)desc.dstAlphaBlendFactor);
            Alimer::hash_combine(hash, (uint32_t)desc.alphaBlendOp);
            Alimer::hash_combine(hash, (uint32_t)desc.colorWriteMask);
            return hash;
        }
    };

    template<>
    struct hash<Alimer::SamplerDescriptor>
    {
        std::size_t operator()(const Alimer::SamplerDescriptor& desc) const noexcept
        {
            std::size_t hash = 0;
            Alimer::hash_combine(hash, (uint32_t)desc.magFilter);
            Alimer::hash_combine(hash, (uint32_t)desc.minFilter);
            Alimer::hash_combine(hash, (uint32_t)desc.mipmapFilter);
            Alimer::hash_combine(hash, (uint32_t)desc.addressModeU);
            Alimer::hash_combine(hash, (uint32_t)desc.addressModeV);
            Alimer::hash_combine(hash, (uint32_t)desc.addressModeW);
            Alimer::hash_combine(hash, desc.mipLodBias);
            Alimer::hash_combine(hash, desc.maxAnisotropy);
            Alimer::hash_combine(hash, (uint32_t)desc.compareFunction);
            Alimer::hash_combine(hash, desc.lodMinClamp);
            Alimer::hash_combine(hash, desc.lodMaxClamp);
            Alimer::hash_combine(hash, (uint32_t)desc.borderColor);
            return hash;
        }
    };
    
}
