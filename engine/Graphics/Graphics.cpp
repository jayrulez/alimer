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
// The implementation is based on WickedEngine graphics code, MIT license (https://github.com/turanszkij/WickedEngine/blob/master/LICENSE.md)

#include "Core/Log.h"
#include "Graphics/Graphics.h"
#include "AlimerConfig.h"
#include "PlatformIncl.h"

#if defined(ALIMER_D3D12)
#    include "Graphics/D3D12/Graphics_D3D12.h"
#endif
#if defined(ALIMER_VULKAN)
#    include "Graphics/Vulkan/Graphics_Vulkan.h"
#endif

namespace alimer
{
    std::set<GraphicsBackendType> Graphics::GetAvailableBackends()
    {
        static std::set<GraphicsBackendType> availableDrivers;

        if (availableDrivers.empty())
        {
#if defined(ALIMER_D3D12)
            if (D3D12Graphics::IsAvailable())
                availableDrivers.insert(GraphicsBackendType::Direct3D12);
#endif

#if defined(ALIMER_VULKAN)
            if (IsVulkanBackendAvailable())
                availableDrivers.insert(GraphicsBackendType::Vulkan);
#endif
        }

        return availableDrivers;
    }

    RefPtr<Graphics> Graphics::Create(WindowHandle windowHandle, const GraphicsSettings& desc)
    {
        GraphicsBackendType backendType = desc.backendType;

        if (backendType == GraphicsBackendType::Count)
        {
            const auto availableDrivers = GetAvailableBackends();

            if (availableDrivers.find(GraphicsBackendType::Direct3D12) != availableDrivers.end())
                backendType = GraphicsBackendType::Direct3D12;
            //else if (availableDrivers.find(GraphicsBackendType::Metal) != availableDrivers.end())
            //   backendType = GraphicsBackendType::Metal;
            else
                backendType = GraphicsBackendType::Vulkan;
        }

        switch (backendType)
        {
#if defined(ALIMER_D3D12)
            case GraphicsBackendType::Direct3D12:
                if (D3D12Graphics::IsAvailable())
                {
                    return MakeRefPtr<D3D12Graphics>(windowHandle, desc);
                }
                break;
#endif

#if defined(ALIMER_D3D11)
            case GraphicsBackendType::Direct3D11:
                if (GraphicsDevice_DX11::IsAvailable())
                {
                    return std::make_shared<GraphicsDevice_DX11>(windowHandle, desc);
                }
                break;
#endif

#if defined(ALIMER_VULKAN)
            case GraphicsBackendType::Vulkan:
                if (IsVulkanBackendAvailable())
                {
                    return CreateVulkanGraphics(windowHandle, desc);
                }
                break;
#endif

            default:
                LOGE("Graphics backend is not supported");
                break;
        }

        return nullptr;
    }

    Graphics::Graphics(WindowHandle window, const GraphicsSettings& desc)
    {
#if ALIMER_PLATFORM_WINDOWS
        RECT rect = RECT();
        GetClientRect(window, &rect);
        backbufferWidth = static_cast<uint32_t>(rect.right - rect.left);
        backbufferHeight = static_cast<uint32_t>(rect.bottom - rect.top);
#elif ALIMER_PLATFORM_UWP
        float dpiscale = wiPlatform::GetDPIScaling();
        backbufferWidth = int(window->Bounds.Width * dpiscale);
        backbufferHeight = int(window->Bounds.Height * dpiscale);
#elif SDL2
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        RESOLUTIONWIDTH = width;
        RESOLUTIONHEIGHT = height;
#endif
        verticalSync = desc.verticalSync;
        BACKBUFFER_FORMAT = desc.backbufferFormat;
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
            if (attrDesc->format == VertexFormat::Invalid)
            {
                break;
            }

            ALIMER_ASSERT((attrDesc->bufferIndex >= 0) && (attrDesc->bufferIndex < kMaxVertexBufferBindings));
            if (useAutOffset)
            {
                attrDesc->offset = autoOffset[attrDesc->bufferIndex];
            }
            autoOffset[attrDesc->bufferIndex] += GetVertexFormatSize(attrDesc->format);
        }

        // Compute vertex strides if needed.
        for (uint32_t bufferIndex = 0; bufferIndex < kMaxVertexBufferBindings; bufferIndex++)
        {
            VertexBufferLayoutDescriptor* layoutDesc = &def.vertexDescriptor.layouts[bufferIndex];
            if (layoutDesc->stride == 0)
            {
                layoutDesc->stride = autoOffset[bufferIndex];
            }
        }

        return def;
    }

    RefPtr<RenderPipeline> Graphics::CreateRenderPipeline(const RenderPipelineDescriptor* descriptor)
    {
        RenderPipelineDescriptor descDef = RenderPipelineDescriptor_Defaults(descriptor);
        RefPtr<RenderPipeline> pipeline;
        if (!CreateRenderPipelineCore(&descDef, pipeline.GetAddressOf()))
        {
            return nullptr;
        }

        return pipeline;
    }

    bool Graphics::CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) const
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

    float Graphics::GetScreenWidth() const
    {
        return (float) GetResolutionWidth() / 1.0f; // wiPlatform::GetDPIScaling();
    }

    float Graphics::GetScreenHeight() const
    {
        return (float) GetResolutionHeight() / 1.0f; //wiPlatform::GetDPIScaling();
    }

    uint32_t GetVertexFormatNumComponents(VertexFormat format)
    {
        switch (format)
        {
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
        return descriptor->stencilBack.compare != CompareFunction::Always || descriptor->stencilBack.failOp != StencilOperation::Keep || descriptor->stencilBack.depthFailOp != StencilOperation::Keep || descriptor->stencilBack.passOp != StencilOperation::Keep || descriptor->stencilFront.compare != CompareFunction::Always || descriptor->stencilFront.failOp != StencilOperation::Keep || descriptor->stencilFront.depthFailOp != StencilOperation::Keep || descriptor->stencilFront.passOp != StencilOperation::Keep;
    }
}
