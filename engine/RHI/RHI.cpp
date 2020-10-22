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

#include "AlimerConfig.h"
#include "RHI.h"

#if defined(ALIMER_D3D11)
#   include "RHI/D3D11/RHI_D3D11.h"
#endif
#if defined(ALIMER_D3D12)
#   include "RHI/D3D12/RHI_D3D12.h"
#endif
#if defined(ALIMER_VULKAN) 
#   include "RHI/Vulkan/RHI_Vulkan.h"
#endif

namespace Alimer
{
    std::shared_ptr<GraphicsDevice> GraphicsDevice::Create(void* windowHandle, GraphicsBackendType backendType, bool fullscreen, bool enableDebugLayer)
    {
        if (backendType == GraphicsBackendType::Count)
        {
#if defined(ALIMER_D3D12) && defined(TODO)
            if (GraphicsDevice_DX12::IsAvailable())
                backendType = GraphicsBackendType::Direct3D12;
#endif

            if (backendType == GraphicsBackendType::Count)
            {

#if defined(ALIMER_D3D11)
                if (GraphicsDevice_DX11::IsAvailable())
                    backendType = GraphicsBackendType::Direct3D11;
#endif
            }
        }

        switch (backendType)
        {
#if defined(ALIMER_D3D12) && defined(TODO)
        case GraphicsBackendType::Direct3D12:
            if (GraphicsDevice_DX12::IsAvailable())
            {
                return std::make_shared<GraphicsDevice_DX12>(windowHandle, fullscreen, enableDebugLayer);
            }
            break;
#endif

#if defined(ALIMER_D3D11)
        case GraphicsBackendType::Direct3D11:
            if (GraphicsDevice_DX11::IsAvailable())
            {
                return std::make_shared<GraphicsDevice_DX11>(windowHandle, fullscreen, enableDebugLayer);
            }
            break;
#endif

#if defined(ALIMER_VULKAN) && defined(TODO)
        case GraphicsBackendType::Vulkan:
            if (IsVulkanBackendAvailable())
            {
                return CreateVulkanGraphicsDevice(windowHandle, fullscreen, enableDebugLayer);
            }
            break;
#endif

        default:
            break;
        }

        return nullptr;
    }

    static RenderPipelineDescriptor RenderPipelineDescriptor_Defaults(const RenderPipelineDescriptor* desc)
    {
        RenderPipelineDescriptor def = *desc;

        uint32_t autoOffset[kMaxVertexBufferBindings] = {};

        bool useAutOffset = true;
        for (uint32_t i = 0; i < kMaxVertexAttributes; ++i)
        {
            // to use computed offsets, *all* attr offsets must be 0.
            if (desc->vertexDescriptor.attributes[i].format != VertexFormat::Invalid && desc->vertexDescriptor.attributes[i].offset != 0)
            {
                useAutOffset = false;
            }
        }

        for (uint32_t i = 0; i < kMaxVertexAttributes; ++i)
        {
            VertexAttributeDescriptor* attrDesc = &def.vertexDescriptor.attributes[i];
            if (attrDesc->format == VertexFormat::Invalid) {
                break;
            }

            ALIMER_ASSERT((attrDesc->bufferIndex >= 0) && (attrDesc->bufferIndex < kMaxVertexBufferBindings));
            if (useAutOffset) {
                attrDesc->offset = autoOffset[attrDesc->bufferIndex];
            }
            autoOffset[attrDesc->bufferIndex] += GetVertexFormatSize(attrDesc->format);
        }

        // Compute vertex strides if needed.
        for (uint32_t bufferIndex = 0; bufferIndex < kMaxVertexBufferBindings; bufferIndex++) {
            VertexBufferLayoutDescriptor* layoutDesc = &def.vertexDescriptor.layouts[bufferIndex];
            if (layoutDesc->stride == 0) {
                layoutDesc->stride = autoOffset[bufferIndex];
            }
        }

        return def;
    }

    bool GraphicsDevice::CreateRenderPipeline(const RenderPipelineDescriptor* descriptor, PipelineState* pipelineState)
    {
        RenderPipelineDescriptor descDef = RenderPipelineDescriptor_Defaults(descriptor);
        return CreateRenderPipelineCore(&descDef, pipelineState);
    }

    bool GraphicsDevice::CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) const
    {
        switch (capability)
        {
        case GRAPHICSDEVICE_CAPABILITY_TESSELLATION:
            return TESSELLATION;
        case GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION:
            return CONSERVATIVE_RASTERIZATION;
        case GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS:
            return RASTERIZER_ORDERED_VIEWS;
        case GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_COMMON:
            return UAV_LOAD_FORMAT_COMMON;
        case GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT:
            return UAV_LOAD_FORMAT_R11G11B10_FLOAT;
        case GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS:
            return RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS;
        case GRAPHICSDEVICE_CAPABILITY_RAYTRACING:
            return RAYTRACING;
        case GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE:
            return RAYTRACING_INLINE;
        case GRAPHICSDEVICE_CAPABILITY_DESCRIPTOR_MANAGEMENT:
            return DESCRIPTOR_MANAGEMENT;
        case GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING:
            return VARIABLE_RATE_SHADING;
        case GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2:
            return VARIABLE_RATE_SHADING_TIER2;
        case GRAPHICSDEVICE_CAPABILITY_MESH_SHADER:
            return MESH_SHADER;
        }
        return false;
    }

    uint32_t GraphicsDevice::GetFormatStride(FORMAT value) const
    {
        switch (value)
        {

        case FORMAT_R32G32B32A32_FLOAT:
        case FORMAT_R32G32B32A32_UINT:
        case FORMAT_R32G32B32A32_SINT:
        case FORMAT_BC1_UNORM:
        case FORMAT_BC1_UNORM_SRGB:
        case FORMAT_BC2_UNORM:
        case FORMAT_BC2_UNORM_SRGB:
        case FORMAT_BC3_UNORM:
        case FORMAT_BC3_UNORM_SRGB:
        case FORMAT_BC4_SNORM:
        case FORMAT_BC4_UNORM:
        case FORMAT_BC5_SNORM:
        case FORMAT_BC5_UNORM:
        case FORMAT_BC6H_UF16:
        case FORMAT_BC6H_SF16:
        case FORMAT_BC7_UNORM:
        case FORMAT_BC7_UNORM_SRGB:
            return 16;

        case FORMAT_R32G32B32_FLOAT:
        case FORMAT_R32G32B32_UINT:
        case FORMAT_R32G32B32_SINT:
            return 12;

        case FORMAT_R16G16B16A16_FLOAT:
        case FORMAT_R16G16B16A16_UNORM:
        case FORMAT_R16G16B16A16_UINT:
        case FORMAT_R16G16B16A16_SNORM:
        case FORMAT_R16G16B16A16_SINT:
            return 8;

        case FORMAT_R32G32_FLOAT:
        case FORMAT_R32G32_UINT:
        case FORMAT_R32G32_SINT:
        case FORMAT_R32G8X24_TYPELESS:
        case FORMAT_D32_FLOAT_S8X24_UINT:
            return 8;

        case FORMAT_R10G10B10A2_UNORM:
        case FORMAT_R10G10B10A2_UINT:
        case FORMAT_R11G11B10_FLOAT:
        case FORMAT_R8G8B8A8_UNORM:
        case FORMAT_R8G8B8A8_UNORM_SRGB:
        case FORMAT_R8G8B8A8_UINT:
        case FORMAT_R8G8B8A8_SNORM:
        case FORMAT_R8G8B8A8_SINT:
        case FORMAT_B8G8R8A8_UNORM:
        case FORMAT_B8G8R8A8_UNORM_SRGB:
        case FORMAT_R16G16_FLOAT:
        case FORMAT_R16G16_UNORM:
        case FORMAT_R16G16_UINT:
        case FORMAT_R16G16_SNORM:
        case FORMAT_R16G16_SINT:
        case FORMAT_R32_TYPELESS:
        case FORMAT_D32_FLOAT:
        case FORMAT_R32_FLOAT:
        case FORMAT_R32_UINT:
        case FORMAT_R32_SINT:
        case FORMAT_R24G8_TYPELESS:
        case FORMAT_D24_UNORM_S8_UINT:
            return 4;

        case FORMAT_R8G8_UNORM:
        case FORMAT_R8G8_UINT:
        case FORMAT_R8G8_SNORM:
        case FORMAT_R8G8_SINT:
        case FORMAT_R16_TYPELESS:
        case FORMAT_R16_FLOAT:
        case FORMAT_D16_UNORM:
        case FORMAT_R16_UNORM:
        case FORMAT_R16_UINT:
        case FORMAT_R16_SNORM:
        case FORMAT_R16_SINT:
            return 2;

        case FORMAT_R8_UNORM:
        case FORMAT_R8_UINT:
        case FORMAT_R8_SNORM:
        case FORMAT_R8_SINT:
            return 1;

        default:
            assert(0); // didn't catch format!
            break;
        }

        return 16;
    }

    bool GraphicsDevice::IsFormatUnorm(FORMAT value) const
    {
        switch (value)
        {
        case FORMAT_R16G16B16A16_UNORM:
        case FORMAT_R10G10B10A2_UNORM:
        case FORMAT_R8G8B8A8_UNORM:
        case FORMAT_R8G8B8A8_UNORM_SRGB:
        case FORMAT_B8G8R8A8_UNORM:
        case FORMAT_B8G8R8A8_UNORM_SRGB:
        case FORMAT_R16G16_UNORM:
        case FORMAT_D24_UNORM_S8_UINT:
        case FORMAT_R8G8_UNORM:
        case FORMAT_D16_UNORM:
        case FORMAT_R16_UNORM:
        case FORMAT_R8_UNORM:
            return true;
        }

        return false;
    }

    bool GraphicsDevice::IsFormatBlockCompressed(FORMAT value) const
    {
        switch (value)
        {
        case FORMAT_BC1_UNORM:
        case FORMAT_BC1_UNORM_SRGB:
        case FORMAT_BC2_UNORM:
        case FORMAT_BC2_UNORM_SRGB:
        case FORMAT_BC3_UNORM:
        case FORMAT_BC3_UNORM_SRGB:
        case FORMAT_BC4_UNORM:
        case FORMAT_BC4_SNORM:
        case FORMAT_BC5_UNORM:
        case FORMAT_BC5_SNORM:
        case FORMAT_BC6H_UF16:
        case FORMAT_BC6H_SF16:
        case FORMAT_BC7_UNORM:
        case FORMAT_BC7_UNORM_SRGB:
            return true;
        }

        return false;
    }

    bool GraphicsDevice::IsFormatStencilSupport(FORMAT value) const
    {
        switch (value)
        {
        case FORMAT_R32G8X24_TYPELESS:
        case FORMAT_D32_FLOAT_S8X24_UINT:
        case FORMAT_R24G8_TYPELESS:
        case FORMAT_D24_UNORM_S8_UINT:
            return true;
        }

        return false;
    }

    float GraphicsDevice::GetScreenWidth() const
    {
        return (float)GetResolutionWidth() / 1.0f; // wiPlatform::GetDPIScaling();
    }

    float GraphicsDevice::GetScreenHeight() const
    {
        return (float)GetResolutionHeight() / 1.0f; //wiPlatform::GetDPIScaling();
    }

    uint32_t GetVertexFormatNumComponents(VertexFormat format)
    {
        switch (format) {
        case VertexFormat::UChar4:
        case VertexFormat::Char4:
        case VertexFormat::UChar4Norm:
        case VertexFormat::Char4Norm:
        case VertexFormat::UShort4:
        case VertexFormat::Short4:
        case VertexFormat::UShort4Norm:
        case VertexFormat::Short4Norm:
        case VertexFormat::Half4:
        case VertexFormat::Float4:
        case VertexFormat::UInt4:
        case VertexFormat::Int4:
            return 4;
        case VertexFormat::Float3:
        case VertexFormat::UInt3:
        case VertexFormat::Int3:
            return 3;
        case VertexFormat::UChar2:
        case VertexFormat::Char2:
        case VertexFormat::UChar2Norm:
        case VertexFormat::Char2Norm:
        case VertexFormat::UShort2:
        case VertexFormat::Short2:
        case VertexFormat::UShort2Norm:
        case VertexFormat::Short2Norm:
        case VertexFormat::Half2:
        case VertexFormat::Float2:
        case VertexFormat::UInt2:
        case VertexFormat::Int2:
            return 2;
        case VertexFormat::Float:
        case VertexFormat::UInt:
        case VertexFormat::Int:
            return 1;
        default:
            ALIMER_UNREACHABLE();
        }
    }

    uint32_t GetVertexFormatComponentSize(VertexFormat format)
    {
        switch (format)
        {
        case VertexFormat::UChar2:
        case VertexFormat::UChar4:
        case VertexFormat::Char2:
        case VertexFormat::Char4:
        case VertexFormat::UChar2Norm:
        case VertexFormat::UChar4Norm:
        case VertexFormat::Char2Norm:
        case VertexFormat::Char4Norm:
            return sizeof(char);
        case VertexFormat::UShort2:
        case VertexFormat::UShort4:
        case VertexFormat::UShort2Norm:
        case VertexFormat::UShort4Norm:
        case VertexFormat::Short2:
        case VertexFormat::Short4:
        case VertexFormat::Short2Norm:
        case VertexFormat::Short4Norm:
        case VertexFormat::Half2:
        case VertexFormat::Half4:
            return sizeof(uint16_t);
        case VertexFormat::Float:
        case VertexFormat::Float2:
        case VertexFormat::Float3:
        case VertexFormat::Float4:
            return sizeof(float);
        case VertexFormat::UInt:
        case VertexFormat::UInt2:
        case VertexFormat::UInt3:
        case VertexFormat::UInt4:
        case VertexFormat::Int:
        case VertexFormat::Int2:
        case VertexFormat::Int3:
        case VertexFormat::Int4:
            return sizeof(int32_t);
        default:
            ALIMER_UNREACHABLE();
        }
    }

    uint32_t GetVertexFormatSize(VertexFormat format)
    {
        return GetVertexFormatNumComponents(format) * GetVertexFormatComponentSize(format);
    }

    bool StencilTestEnabled(const DepthStencilStateDescriptor* descriptor)
    {
        return descriptor->stencilBack.compare != CompareFunction::Always
            || descriptor->stencilBack.failOp != StencilOperation::Keep
            || descriptor->stencilBack.depthFailOp != StencilOperation::Keep
            || descriptor->stencilBack.passOp != StencilOperation::Keep
            || descriptor->stencilFront.compare != CompareFunction::Always
            || descriptor->stencilFront.failOp != StencilOperation::Keep
            || descriptor->stencilFront.depthFailOp != StencilOperation::Keep
            || descriptor->stencilFront.passOp != StencilOperation::Keep;
    }
}
