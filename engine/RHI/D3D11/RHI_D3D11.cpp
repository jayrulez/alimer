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

#include "AlimerConfig.h"

#if defined(ALIMER_D3D11)
#include "RHI_D3D11.h"
#include "Core/String.h"
#include "Core/Log.h"
#include "PlatformIncl.h"

#include <array>
#include <vector>

#if !defined(ALIMER_DISABLE_SHADER_COMPILER)
#   include <d3dcompiler.h>
#endif

#ifdef _WIN32
// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#include <sstream>
#include <algorithm>

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    static PFN_D3D11_CREATE_DEVICE D3D11CreateDevice = nullptr;
#endif

    namespace DX11_Internal
    {
        // Engine -> Native converters

        constexpr uint32_t _ParseBindFlags(uint32_t value)
        {
            uint32_t _flag = 0;

            if (value & BIND_VERTEX_BUFFER)
                _flag |= D3D11_BIND_VERTEX_BUFFER;
            if (value & BIND_INDEX_BUFFER)
                _flag |= D3D11_BIND_INDEX_BUFFER;
            if (value & BIND_CONSTANT_BUFFER)
                _flag |= D3D11_BIND_CONSTANT_BUFFER;
            if (value & BIND_SHADER_RESOURCE)
                _flag |= D3D11_BIND_SHADER_RESOURCE;
            if (value & BIND_STREAM_OUTPUT)
                _flag |= D3D11_BIND_STREAM_OUTPUT;
            if (value & BIND_RENDER_TARGET)
                _flag |= D3D11_BIND_RENDER_TARGET;
            if (value & BIND_DEPTH_STENCIL)
                _flag |= D3D11_BIND_DEPTH_STENCIL;
            if (value & BIND_UNORDERED_ACCESS)
                _flag |= D3D11_BIND_UNORDERED_ACCESS;

            return _flag;
        }
        constexpr uint32_t _ParseCPUAccessFlags(uint32_t value)
        {
            uint32_t _flag = 0;

            if (value & CPU_ACCESS_WRITE)
                _flag |= D3D11_CPU_ACCESS_WRITE;
            if (value & CPU_ACCESS_READ)
                _flag |= D3D11_CPU_ACCESS_READ;

            return _flag;
        }
        constexpr uint32_t _ParseResourceMiscFlags(uint32_t value)
        {
            uint32_t _flag = 0;

            if (value & RESOURCE_MISC_SHARED)
                _flag |= D3D11_RESOURCE_MISC_SHARED;
            if (value & RESOURCE_MISC_TEXTURECUBE)
                _flag |= D3D11_RESOURCE_MISC_TEXTURECUBE;
            if (value & RESOURCE_MISC_INDIRECT_ARGS)
                _flag |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
            if (value & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
                _flag |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            if (value & RESOURCE_MISC_BUFFER_STRUCTURED)
                _flag |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            if (value & RESOURCE_MISC_TILED)
                _flag |= D3D11_RESOURCE_MISC_TILED;

            return _flag;
        }

        /* BlendState */
        constexpr D3D11_BLEND _ConvertBlend(BlendFactor value)
        {
            switch (value)
            {
            case BlendFactor::Zero:
                return D3D11_BLEND_ZERO;
            case BlendFactor::One:
                return D3D11_BLEND_ONE;
            case BlendFactor::SourceColor:
                return D3D11_BLEND_SRC_COLOR;
            case BlendFactor::OneMinusSourceColor:
                return D3D11_BLEND_INV_SRC_COLOR;
            case BlendFactor::SourceAlpha:
                return D3D11_BLEND_SRC_ALPHA;
            case BlendFactor::OneMinusSourceAlpha:
                return D3D11_BLEND_INV_SRC_ALPHA;
            case BlendFactor::DestinationColor:
                return D3D11_BLEND_DEST_COLOR;
            case BlendFactor::OneMinusDestinationColor:
                return D3D11_BLEND_INV_DEST_COLOR;
            case BlendFactor::DestinationAlpha:
                return D3D11_BLEND_DEST_ALPHA;
            case BlendFactor::OneMinusDestinationAlpha:
                return D3D11_BLEND_INV_DEST_ALPHA;
            case BlendFactor::SourceAlphaSaturated:
                return D3D11_BLEND_SRC_ALPHA_SAT;
            case BlendFactor::BlendColor:
                //case BlendFactor::BlendAlpha:
                return D3D11_BLEND_BLEND_FACTOR;
            case BlendFactor::OneMinusBlendColor:
                //case BlendFactor::OneMinusBlendAlpha:
                return D3D11_BLEND_INV_BLEND_FACTOR;
            case BlendFactor::Source1Color:
                return D3D11_BLEND_SRC1_COLOR;
            case BlendFactor::OneMinusSource1Color:
                return D3D11_BLEND_INV_SRC1_COLOR;
            case BlendFactor::Source1Alpha:
                return D3D11_BLEND_SRC1_ALPHA;
            case BlendFactor::OneMinusSource1Alpha:
                return D3D11_BLEND_INV_SRC1_ALPHA;
            default:
                ALIMER_UNREACHABLE();
                return D3D11_BLEND_ZERO;
            }
        }

        constexpr D3D11_BLEND_OP _ConvertBlendOp(BlendOperation value)
        {
            switch (value)
            {
            case BlendOperation::Add:
                return D3D11_BLEND_OP_ADD;
            case BlendOperation::Subtract:
                return D3D11_BLEND_OP_SUBTRACT;
            case BlendOperation::ReverseSubtract:
                return D3D11_BLEND_OP_REV_SUBTRACT;
            case BlendOperation::Min:
                return D3D11_BLEND_OP_MIN;
            case BlendOperation::Max:
                return D3D11_BLEND_OP_MAX;
            default:
                ALIMER_UNREACHABLE();
                return D3D11_BLEND_OP_ADD;
            }
        }

        constexpr uint8_t _ConvertColorWriteMask(ColorWriteMask writeMask)
        {
            static_assert(static_cast<D3D11_COLOR_WRITE_ENABLE>(ColorWriteMask::Red) == D3D11_COLOR_WRITE_ENABLE_RED, "ColorWriteMask values must match");
            static_assert(static_cast<D3D11_COLOR_WRITE_ENABLE>(ColorWriteMask::Green) == D3D11_COLOR_WRITE_ENABLE_GREEN, "ColorWriteMask values must match");
            static_assert(static_cast<D3D11_COLOR_WRITE_ENABLE>(ColorWriteMask::Blue) == D3D11_COLOR_WRITE_ENABLE_BLUE, "ColorWriteMask values must match");
            static_assert(static_cast<D3D11_COLOR_WRITE_ENABLE>(ColorWriteMask::Alpha) == D3D11_COLOR_WRITE_ENABLE_ALPHA, "ColorWriteMask values must match");
            return static_cast<uint8_t>(writeMask);
        }

        D3D11_RENDER_TARGET_BLEND_DESC1 _ConvertColorAttachment(const ColorAttachmentDescriptor* descriptor) {
            D3D11_RENDER_TARGET_BLEND_DESC1 blendDesc;
            blendDesc.BlendEnable = descriptor->blendEnable;
            blendDesc.SrcBlend = _ConvertBlend(descriptor->srcColorBlendFactor);
            blendDesc.DestBlend = _ConvertBlend(descriptor->dstColorBlendFactor);
            blendDesc.BlendOp = _ConvertBlendOp(descriptor->colorBlendOp);
            blendDesc.SrcBlendAlpha = _ConvertBlend(descriptor->srcAlphaBlendFactor);
            blendDesc.DestBlendAlpha = _ConvertBlend(descriptor->dstAlphaBlendFactor);
            blendDesc.BlendOpAlpha = _ConvertBlendOp(descriptor->alphaBlendOp);
            blendDesc.RenderTargetWriteMask = _ConvertColorWriteMask(descriptor->colorWriteMask);
            blendDesc.LogicOpEnable = false;
            blendDesc.LogicOp = D3D11_LOGIC_OP_NOOP;
            return blendDesc;
        }

        constexpr D3D11_FILTER_TYPE _ConvertFilterType(FilterMode filter)
        {
            switch (filter)
            {
            case FilterMode::Nearest:
                return D3D11_FILTER_TYPE_POINT;
            case FilterMode::Linear:
                return D3D11_FILTER_TYPE_LINEAR;
            default:
                ALIMER_UNREACHABLE();
                return (D3D11_FILTER_TYPE)-1;
            }
        }

        inline D3D11_FILTER _ConvertFilter(FilterMode minFilter, FilterMode magFilter, FilterMode mipFilter, bool isComparison, bool isAnisotropic)
        {
            D3D11_FILTER filter;
            D3D11_FILTER_REDUCTION_TYPE reduction = isComparison ? D3D11_FILTER_REDUCTION_TYPE_COMPARISON : D3D11_FILTER_REDUCTION_TYPE_STANDARD;

            if (isAnisotropic)
            {
                filter = D3D11_ENCODE_ANISOTROPIC_FILTER(reduction);
            }
            else
            {
                D3D11_FILTER_TYPE dxMin = _ConvertFilterType(minFilter);
                D3D11_FILTER_TYPE dxMag = _ConvertFilterType(magFilter);
                D3D11_FILTER_TYPE dxMip = _ConvertFilterType(mipFilter);
                filter = D3D11_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, reduction);
            }

            return filter;
        }

        constexpr D3D11_TEXTURE_ADDRESS_MODE _ConvertAddressMode(SamplerAddressMode value)
        {
            switch (value)
            {
            case SamplerAddressMode::Mirror:
                return D3D11_TEXTURE_ADDRESS_MIRROR;
            case SamplerAddressMode::Clamp:
                return D3D11_TEXTURE_ADDRESS_CLAMP;
            case SamplerAddressMode::Border:
                return D3D11_TEXTURE_ADDRESS_BORDER;
                //case SamplerAddressMode::MirrorOnce:
                //    return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;

            case SamplerAddressMode::Wrap:
            default:
                return D3D11_TEXTURE_ADDRESS_WRAP;
            }
        }

        constexpr D3D11_COMPARISON_FUNC _ConvertComparisonFunc(CompareFunction value)
        {
            switch (value)
            {
            case CompareFunction::Never:
                return D3D11_COMPARISON_NEVER;
                break;
            case CompareFunction::Less:
                return D3D11_COMPARISON_LESS;
                break;
            case CompareFunction::Equal:
                return D3D11_COMPARISON_EQUAL;
                break;
            case CompareFunction::LessEqual:
                return D3D11_COMPARISON_LESS_EQUAL;
                break;
            case CompareFunction::Greater:
                return D3D11_COMPARISON_GREATER;
                break;
            case CompareFunction::NotEqual:
                return D3D11_COMPARISON_NOT_EQUAL;
                break;
            case CompareFunction::GreaterEqual:
                return D3D11_COMPARISON_GREATER_EQUAL;
                break;
            case CompareFunction::Always:
                return D3D11_COMPARISON_ALWAYS;
                break;
            default:
                break;
            }
            return D3D11_COMPARISON_NEVER;
        }
        constexpr D3D11_CULL_MODE _ConvertCullMode(CullMode value)
        {
            switch (value)
            {
            case CullMode::None:
                return D3D11_CULL_NONE;
            case CullMode::Front:
                return D3D11_CULL_FRONT;
            case CullMode::Back:
                return D3D11_CULL_BACK;
            default:
                break;
            }
            return D3D11_CULL_NONE;
        }
        constexpr D3D11_STENCIL_OP _ConvertStencilOp(StencilOperation value)
        {
            switch (value)
            {
            case StencilOperation::Keep:
                return D3D11_STENCIL_OP_KEEP;
            case StencilOperation::Zero:
                return D3D11_STENCIL_OP_ZERO;
            case StencilOperation::Replace:
                return D3D11_STENCIL_OP_REPLACE;
            case StencilOperation::IncrementClamp:
                return D3D11_STENCIL_OP_INCR_SAT;
            case StencilOperation::DecrementClamp:
                return D3D11_STENCIL_OP_DECR_SAT;
            case StencilOperation::Invert:
                return D3D11_STENCIL_OP_INVERT;
            case StencilOperation::IncrementWrap:
                return D3D11_STENCIL_OP_INCR;
            case StencilOperation::DecrementWrap:
                return D3D11_STENCIL_OP_DECR;
            default:
                ALIMER_UNREACHABLE();
                break;
            }
            return D3D11_STENCIL_OP_KEEP;
        }

        constexpr D3D11_USAGE _ConvertUsage(USAGE value)
        {
            switch (value)
            {
            case USAGE_DEFAULT:
                return D3D11_USAGE_DEFAULT;
                break;
            case USAGE_IMMUTABLE:
                return D3D11_USAGE_IMMUTABLE;
                break;
            case USAGE_DYNAMIC:
                return D3D11_USAGE_DYNAMIC;
                break;
            case USAGE_STAGING:
                return D3D11_USAGE_STAGING;
                break;
            default:
                break;
            }
            return D3D11_USAGE_DEFAULT;
        }
        constexpr D3D11_INPUT_CLASSIFICATION _ConvertInputClassification(InputStepMode value)
        {
            switch (value)
            {
            case InputStepMode::Vertex:
                return D3D11_INPUT_PER_VERTEX_DATA;
            case InputStepMode::Instance:
                return D3D11_INPUT_PER_INSTANCE_DATA;
            default:
                ALIMER_UNREACHABLE();
                return D3D11_INPUT_PER_VERTEX_DATA;
            }
        }

        inline D3D11_TEXTURE1D_DESC _ConvertTextureDesc1D(const TextureDesc* pDesc)
        {
            D3D11_TEXTURE1D_DESC desc;
            desc.Width = pDesc->Width;
            desc.MipLevels = pDesc->MipLevels;
            desc.ArraySize = pDesc->ArraySize;
            desc.Format = PixelFormatToDXGIFormat(pDesc->format);
            desc.Usage = _ConvertUsage(pDesc->Usage);
            desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
            desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
            desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

            return desc;
        }
        inline D3D11_TEXTURE2D_DESC _ConvertTextureDesc2D(const TextureDesc* pDesc)
        {
            D3D11_TEXTURE2D_DESC desc;
            desc.Width = pDesc->Width;
            desc.Height = pDesc->Height;
            desc.MipLevels = pDesc->MipLevels;
            desc.ArraySize = pDesc->ArraySize;
            desc.Format = PixelFormatToDXGIFormat(pDesc->format);
            desc.SampleDesc.Count = pDesc->SampleCount;
            desc.SampleDesc.Quality = 0;
            desc.Usage = _ConvertUsage(pDesc->Usage);
            desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
            desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
            desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

            return desc;
        }
        inline D3D11_TEXTURE3D_DESC _ConvertTextureDesc3D(const TextureDesc* pDesc)
        {
            D3D11_TEXTURE3D_DESC desc;
            desc.Width = pDesc->Width;
            desc.Height = pDesc->Height;
            desc.Depth = pDesc->Depth;
            desc.MipLevels = pDesc->MipLevels;
            desc.Format = PixelFormatToDXGIFormat(pDesc->format);
            desc.Usage = _ConvertUsage(pDesc->Usage);
            desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
            desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
            desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

            return desc;
        }
        inline D3D11_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
        {
            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = pInitialData.pSysMem;
            data.SysMemPitch = pInitialData.SysMemPitch;
            data.SysMemSlicePitch = pInitialData.SysMemSlicePitch;

            return data;
        }


        // Native -> Engine converters

        constexpr uint32_t _ParseBindFlags_Inv(uint32_t value)
        {
            uint32_t _flag = 0;

            if (value & D3D11_BIND_VERTEX_BUFFER)
                _flag |= BIND_VERTEX_BUFFER;
            if (value & D3D11_BIND_INDEX_BUFFER)
                _flag |= BIND_INDEX_BUFFER;
            if (value & D3D11_BIND_CONSTANT_BUFFER)
                _flag |= BIND_CONSTANT_BUFFER;
            if (value & D3D11_BIND_SHADER_RESOURCE)
                _flag |= BIND_SHADER_RESOURCE;
            if (value & D3D11_BIND_STREAM_OUTPUT)
                _flag |= BIND_STREAM_OUTPUT;
            if (value & D3D11_BIND_RENDER_TARGET)
                _flag |= BIND_RENDER_TARGET;
            if (value & D3D11_BIND_DEPTH_STENCIL)
                _flag |= BIND_DEPTH_STENCIL;
            if (value & D3D11_BIND_UNORDERED_ACCESS)
                _flag |= BIND_UNORDERED_ACCESS;

            return _flag;
        }
        constexpr uint32_t _ParseCPUAccessFlags_Inv(uint32_t value)
        {
            uint32_t _flag = 0;

            if (value & D3D11_CPU_ACCESS_WRITE)
                _flag |= CPU_ACCESS_WRITE;
            if (value & D3D11_CPU_ACCESS_READ)
                _flag |= CPU_ACCESS_READ;

            return _flag;
        }
        constexpr uint32_t _ParseResourceMiscFlags_Inv(uint32_t value)
        {
            uint32_t _flag = 0;

            if (value & D3D11_RESOURCE_MISC_SHARED)
                _flag |= RESOURCE_MISC_SHARED;
            if (value & D3D11_RESOURCE_MISC_TEXTURECUBE)
                _flag |= RESOURCE_MISC_TEXTURECUBE;
            if (value & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
                _flag |= RESOURCE_MISC_INDIRECT_ARGS;
            if (value & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
                _flag |= RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            if (value & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
                _flag |= RESOURCE_MISC_BUFFER_STRUCTURED;
            if (value & D3D11_RESOURCE_MISC_TILED)
                _flag |= RESOURCE_MISC_TILED;

            return _flag;
        }


        constexpr USAGE _ConvertUsage_Inv(D3D11_USAGE value)
        {
            switch (value)
            {
            case D3D11_USAGE_DEFAULT:
                return USAGE_DEFAULT;
                break;
            case D3D11_USAGE_IMMUTABLE:
                return USAGE_IMMUTABLE;
                break;
            case D3D11_USAGE_DYNAMIC:
                return USAGE_DYNAMIC;
                break;
            case D3D11_USAGE_STAGING:
                return USAGE_STAGING;
                break;
            default:
                break;
            }
            return USAGE_DEFAULT;
        }

        inline TextureDesc _ConvertTextureDesc_Inv(const D3D11_TEXTURE2D_DESC* pDesc)
        {
            TextureDesc desc;
            desc.Width = pDesc->Width;
            desc.Height = pDesc->Height;
            desc.MipLevels = pDesc->MipLevels;
            desc.ArraySize = pDesc->ArraySize;
            desc.format = PixelFormatFromDXGIFormat(pDesc->Format);
            desc.SampleCount = pDesc->SampleDesc.Count;
            desc.Usage = _ConvertUsage_Inv(pDesc->Usage);
            desc.BindFlags = _ParseBindFlags_Inv(pDesc->BindFlags);
            desc.CPUAccessFlags = _ParseCPUAccessFlags_Inv(pDesc->CPUAccessFlags);
            desc.MiscFlags = _ParseResourceMiscFlags_Inv(pDesc->MiscFlags);

            return desc;
        }

        inline D3D11_DEPTH_STENCILOP_DESC _ConvertStencilOpDesc(const StencilStateFaceDescriptor descriptor) {
            D3D11_DEPTH_STENCILOP_DESC desc;
            desc.StencilFailOp = _ConvertStencilOp(descriptor.failOp);
            desc.StencilDepthFailOp = _ConvertStencilOp(descriptor.depthFailOp);
            desc.StencilPassOp = _ConvertStencilOp(descriptor.passOp);
            desc.StencilFunc = _ConvertComparisonFunc(descriptor.compare);
            return desc;
        }

        // Local Helpers:
        const void* const __nullBlob[128] = {}; // this is initialized to nullptrs and used to unbind resources!

        struct Buffer_DX11 : public GraphicsBuffer
        {
            Buffer_DX11(const GPUBufferDesc& desc_)
                : GraphicsBuffer(desc_)
            {
            }

            ~Buffer_DX11() override
            {
                Destroy();
            }

            void Destroy() override
            {
                SafeRelease(handle);
            }

#ifdef _DEBUG
            void SetName(const String& newName) override
            {
                GraphicsBuffer::SetName(newName);

                handle->SetPrivateData(g_D3DDebugObjectName, (UINT)newName.length(), newName.c_str());
            }
#endif

            ID3D11Buffer* handle = nullptr;
            ComPtr<ID3D11ShaderResourceView> srv;
            ComPtr<ID3D11UnorderedAccessView> uav;
            std::vector<ComPtr<ID3D11ShaderResourceView>> subresources_srv;
            std::vector<ComPtr<ID3D11UnorderedAccessView>> subresources_uav;
        };

        struct Resource_DX11
        {
            ComPtr<ID3D11Resource> resource;
            ComPtr<ID3D11ShaderResourceView> srv;
            ComPtr<ID3D11UnorderedAccessView> uav;
            std::vector<ComPtr<ID3D11ShaderResourceView>> subresources_srv;
            std::vector<ComPtr<ID3D11UnorderedAccessView>> subresources_uav;
        };
        struct Texture_DX11 : public Resource_DX11
        {
            ComPtr<ID3D11RenderTargetView> rtv;
            ComPtr<ID3D11DepthStencilView> dsv;
            std::vector<ComPtr<ID3D11RenderTargetView>> subresources_rtv;
            std::vector<ComPtr<ID3D11DepthStencilView>> subresources_dsv;
        };
        struct VertexShader_DX11
        {
            ComPtr<ID3D11VertexShader> resource;
        };
        struct HullShader_DX11
        {
            ComPtr<ID3D11HullShader> resource;
        };
        struct DomainShader_DX11
        {
            ComPtr<ID3D11DomainShader> resource;
        };
        struct GeometryShader_DX11
        {
            ComPtr<ID3D11GeometryShader> resource;
        };
        struct PixelShader_DX11
        {
            ComPtr<ID3D11PixelShader> resource;
        };
        struct ComputeShader_DX11
        {
            ComPtr<ID3D11ComputeShader> resource;
        };

        struct Sampler_DX11 : public Sampler
        {
            ID3D11SamplerState* handle;

            ~Sampler_DX11() override
            {
                Destroy();
            }

            void Destroy() override
            {
            }

#ifdef _DEBUG
            void SetName(const String& newName) override
            {
                Sampler::SetName(newName);
                handle->SetPrivateData(g_D3DDebugObjectName, (UINT)newName.length(), newName.c_str());
            }
#endif
        };

        struct Query_DX11
        {
            ComPtr<ID3D11Query> resource;
        };

        struct PipelineState_DX11 : public RenderPipeline
        {
            RenderPipelineDescriptor desc;
            ID3D11RasterizerState* rasterizerState;
            ID3D11DepthStencilState* depthStencilState;
            ID3D11BlendState1* blendState;
            ComPtr<ID3D11InputLayout> inputLayout;
            D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
            uint32_t vertexBufferStrides[kMaxVertexBufferBindings];

            ~PipelineState_DX11() override
            {
                Destroy();
            }

            void Destroy() override
            {
            }

#ifdef _DEBUG
            void SetName(const String& newName) override
            {
                RenderPipeline::SetName(newName);
            }
#endif
        };

        /* New interface*/
        const Buffer_DX11* to_internal(const GraphicsBuffer* param)
        {
            return static_cast<const Buffer_DX11*>(param);
        }

        const Sampler_DX11* to_internal(const Sampler* param)
        {
            return static_cast<const Sampler_DX11*>(param);
        }

        const PipelineState_DX11* to_internal(const RenderPipeline* param)
        {
            return static_cast<const PipelineState_DX11*>(param);
        }

        Resource_DX11* to_internal(const GPUResource* param)
        {
            return static_cast<Resource_DX11*>(param->internal_state.get());
        }


        Texture_DX11* to_internal(const Texture* param)
        {
            return static_cast<Texture_DX11*>(param->internal_state.get());
        }
        Query_DX11* to_internal(const GPUQuery* param)
        {
            return static_cast<Query_DX11*>(param->internal_state.get());
        }


#if !defined(ALIMER_DISABLE_SHADER_COMPILER)
        static HINSTANCE d3dcompiler_dll = nullptr;
        static bool d3dcompiler_dll_load_failed = false;
        static pD3DCompile D3DCompile_func;

        inline bool LoadD3DCompilerDLL(void)
        {
            /* load DLL on demand */
            if (d3dcompiler_dll == nullptr && !d3dcompiler_dll_load_failed)
            {
                d3dcompiler_dll = LoadLibraryW(L"d3dcompiler_47.dll");
                if (d3dcompiler_dll == nullptr)
                {
                    /* don't attempt to load missing DLL in the future */
                    d3dcompiler_dll_load_failed = true;
                    return false;
                }

                /* look up function pointers */
                D3DCompile_func = (pD3DCompile)GetProcAddress(d3dcompiler_dll, "D3DCompile");
                assert(D3DCompile_func != nullptr);
            }

            return d3dcompiler_dll != nullptr;
        }
#endif
    }
    using namespace DX11_Internal;

    namespace
    {
#if defined(_DEBUG)
        // Check for SDK Layer support.
        inline bool SdkLayersAvailable() noexcept
        {
            HRESULT hr = D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
                nullptr,
                D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
                nullptr,                    // Any feature level will do.
                0,
                D3D11_SDK_VERSION,
                nullptr,                    // No need to keep the D3D device reference.
                nullptr,                    // No need to know the feature level.
                nullptr                     // No need to keep the D3D device context reference.
            );

            return SUCCEEDED(hr);
        }
#endif
    }

    class D3D11_CommandList final : public CommandList
    {
    public:
        D3D11_CommandList(GraphicsDevice_DX11* device);
        ~D3D11_CommandList() override;

        void Execute(ID3D11DeviceContext1* immediateContext);
        void Reset();
        void PresentBegin() override;
        void PresentEnd() override;

        void PushDebugGroup(const char* name) override;
        void PopDebugGroup() override;
        void InsertDebugMarker(const char* name) override;

        void RenderPassBegin(const RenderPass* renderpass) override;
        void RenderPassEnd() override;
        void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) override;
        void SetViewport(const Viewport& viewport) override;
        void SetViewports(uint32_t viewportCount, const Viewport* pViewports) override;
        void SetScissorRect(const ScissorRect& rect) override;
        void SetScissorRects(uint32_t scissorCount, const ScissorRect* rects) override;
        void BindResource(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource = -1) override;
        void BindResources(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count) override;
        void BindUAV(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource = -1) override;
        void BindUAVs(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count) override;
        void BindSampler(ShaderStage stage, const Sampler* sampler, uint32_t slot) override;
        void BindConstantBuffer(ShaderStage stage, const GraphicsBuffer* buffer, uint32_t slot) override;
        void BindVertexBuffers(const GraphicsBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets) override;
        void BindIndexBuffer(const GraphicsBuffer* indexBuffer, IndexFormat format, uint32_t offset) override;
        void BindStencilRef(uint32_t value) override;
        void BindBlendFactor(float r, float g, float b, float a) override;

        void SetRenderPipeline(RenderPipeline* pipeline) override;
        void BindComputeShader(const Shader* shader) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) override;
        void DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;
        void DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;

        void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;
        void DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;
        void CopyResource(const GPUResource* pDst, const GPUResource* pSrc) override;

        GPUAllocation AllocateGPU(size_t dataSize) override;
        void UpdateBuffer(GraphicsBuffer* buffer, const void* data, uint64_t size = 0) override;

        void QueryBegin(const GPUQuery* query) override;
        void QueryEnd(const GPUQuery* query) override;
        void Barrier(const GPUBarrier* barriers, uint32_t numBarriers) override;

    private:
        void PrepareDraw();
        void CommitAllocations();

    private:
        GraphicsDevice_DX11* device;
        ID3D11DeviceContext1* handle = nullptr;
        ID3DUserDefinedAnnotation* userDefinedAnnotation = nullptr;
        ComPtr<ID3D11CommandList> commandList;

        D3D11_VIEWPORT viewports[kMaxViewportAndScissorRects];
        D3D11_RECT scissorRects[kMaxViewportAndScissorRects];
        const RenderPass* active_renderpass = nullptr;
        D3D_PRIMITIVE_TOPOLOGY prev_pt = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        bool dirty_pso = false;
        RenderPipeline* active_pso = nullptr;

        uint32_t	stencilRef = 0;
        XMFLOAT4	blendFactor{};

        ID3D11UnorderedAccessView* raster_uavs[8] = {};
        uint8_t raster_uavs_slot = 0;
        uint8_t raster_uavs_count = 0;

        ID3D11ComputeShader* computeShader = nullptr;

        ID3D11VertexShader* prev_vs = {};
        ID3D11PixelShader* prev_ps = {};
        ID3D11HullShader* prev_hs = {};
        ID3D11DomainShader* prev_ds = {};
        ID3D11GeometryShader* prev_gs = {};
        XMFLOAT4 prev_blendfactor = {};
        uint32_t prev_samplemask = {};
        ID3D11BlendState* prev_bs = {};
        ID3D11RasterizerState* prev_rs = {};
        uint32_t prev_stencilRef = {};
        ID3D11DepthStencilState* prev_dss = {};
        ID3D11InputLayout* prev_il = {};

        struct GPUAllocator
        {
            RefPtr<GraphicsBuffer> buffer;
            size_t byteOffset = 0;
            uint64_t residentFrame = 0;
            bool dirty = false;
        } frame_allocator;
    };

    bool GraphicsDevice_DX11::IsAvailable()
    {
        static bool available = false;
        static bool available_initialized = false;

        if (available_initialized) {
            return available;
        }

        available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        static HMODULE dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!dxgiDLL) {
            return false;
        }

        CreateDXGIFactory1 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(dxgiDLL, "CreateDXGIFactory1"));
        if (!CreateDXGIFactory1)
        {
            return false;
        }

        CreateDXGIFactory2 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(dxgiDLL, "CreateDXGIFactory2"));
        DXGIGetDebugInterface1 = reinterpret_cast<PFN_DXGI_GET_DEBUG_INTERFACE1>(GetProcAddress(dxgiDLL, "DXGIGetDebugInterface1"));

        static HMODULE d3d11DLL = LoadLibraryA("d3d11.dll");
        if (!d3d11DLL) {
            return false;
        }

        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11DLL, "D3D11CreateDevice");
        if (!D3D11CreateDevice) {
            return false;
        }
#endif

        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            s_featureLevels,
            _countof(s_featureLevels),
            D3D11_SDK_VERSION,
            nullptr,
            nullptr,
            nullptr
        );

        if (FAILED(hr))
        {
            return false;
        }

        available = true;
        return true;
    }

    GraphicsDevice_DX11::GraphicsDevice_DX11(WindowHandle window, const Desc& desc)
        : GraphicsDevice(window, desc)
    {
        if (!IsAvailable()) {
            // TODO: MessageBox
        }

        CreateFactory();

        // Get adapter and create device
        {
            ComPtr<IDXGIAdapter1> adapter;
            GetAdapter(adapter.GetAddressOf());

            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
            const bool enableDebugLayer = any(desc.flags & GraphicsDeviceFlags::DebugRuntime);
            if (enableDebugLayer)
            {
                if (SdkLayersAvailable())
                {
                    // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
                }
                else
                {
                    OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
                }
            }
#endif /* defined(_DEBUG) */

            static const D3D_FEATURE_LEVEL s_featureLevels[] =
            {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0
            };

            // Create the Direct3D 11 API device object and a corresponding context.
            ID3D11Device* tempDevice;
            ID3D11DeviceContext* tempContext;

            HRESULT hr = E_FAIL;
            if (adapter)
            {
                hr = D3D11CreateDevice(
                    adapter.Get(),
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    creationFlags,
                    s_featureLevels,
                    _countof(s_featureLevels),
                    D3D11_SDK_VERSION,
                    &tempDevice,
                    &featureLevel,
                    &tempContext
                );
            }
            else
            {
                throw std::exception("No Direct3D hardware device found");
            }

            ThrowIfFailed(hr);

#ifndef NDEBUG
            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(tempDevice->QueryInterface(&d3dDebug)))
            {
                ID3D11InfoQueue* d3dInfoQueue;
                if (SUCCEEDED(d3dDebug->QueryInterface(&d3dInfoQueue)))
                {
#ifdef _DEBUG
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
                    D3D11_MESSAGE_ID hide[] =
                    {
                        D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                    };
                    D3D11_INFO_QUEUE_FILTER filter = {};
                    filter.DenyList.NumIDs = _countof(hide);
                    filter.DenyList.pIDList = hide;
                    d3dInfoQueue->AddStorageFilterEntries(&filter);
                    d3dInfoQueue->Release();
                }
                d3dDebug->Release();
            }
#endif

            ThrowIfFailed(tempDevice->QueryInterface(&device));
            ThrowIfFailed(tempContext->QueryInterface(&immediateContext));

            tempContext->Release();
            tempDevice->Release();
        }

        // Create Swapchain
        {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
            swapChainDesc.Width = backbufferWidth;
            swapChainDesc.Height = backbufferHeight;
            swapChainDesc.Format = PixelFormatToDXGIFormat(GetBackBufferFormat());
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
            swapChainDesc.Flags = 0;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

#ifndef PLATFORM_UWP
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = !desc.fullscreen;
            ThrowIfFailed(dxgiFactory2->CreateSwapChainForHwnd(device, window, &swapChainDesc, &fsSwapChainDesc, nullptr, &swapChain));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(dxgiFactory2->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
            sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
            sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

            ThrowIfFailed(dxgiFactory2->CreateSwapChainForCoreWindow(device, reinterpret_cast<IUnknown*>(window.Get()), &sd, nullptr, &swapChain));
#endif

            IDXGIDevice1* dxgiDevice;
            if (SUCCEEDED(device->QueryInterface(&dxgiDevice)))
            {
                // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
                // ensures that the application will only render after each VSync, minimizing power consumption.
                ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));
                dxgiDevice->Release();
            }
        }


        D3D_FEATURE_LEVEL aquiredFeatureLevel = device->GetFeatureLevel();
        TESSELLATION = ((aquiredFeatureLevel >= D3D_FEATURE_LEVEL_11_0) ? true : false);

        //D3D11_FEATURE_DATA_D3D11_OPTIONS features_0;
        //hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &features_0, sizeof(features_0));

        //D3D11_FEATURE_DATA_D3D11_OPTIONS1 features_1;
        //hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &features_1, sizeof(features_1));

        D3D11_FEATURE_DATA_D3D11_OPTIONS2 features_2;
        HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &features_2, sizeof(features_2));
        CONSERVATIVE_RASTERIZATION = features_2.ConservativeRasterizationTier >= D3D11_CONSERVATIVE_RASTERIZATION_TIER_1;
        RASTERIZER_ORDERED_VIEWS = features_2.ROVsSupported == TRUE;

        if (features_2.TypedUAVLoadAdditionalFormats)
        {
            // More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
            UAV_LOAD_FORMAT_COMMON = true;

            D3D11_FEATURE_DATA_FORMAT_SUPPORT2 FormatSupport = {};
            FormatSupport.InFormat = DXGI_FORMAT_R11G11B10_FLOAT;
            hr = device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &FormatSupport, sizeof(FormatSupport));
            if (SUCCEEDED(hr) && (FormatSupport.OutFormatSupport2 & D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
            {
                UAV_LOAD_FORMAT_R11G11B10_FLOAT = true;
            }
        }

        D3D11_FEATURE_DATA_D3D11_OPTIONS3 features_3;
        hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &features_3, sizeof(features_3));
        RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = features_3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer == TRUE;

        CreateBackBufferResources();

        emptyresource = std::make_shared<EmptyResourceHandle>();
        LOGI("Direct3D11 Graphics Device created");
    }

    GraphicsDevice_DX11::~GraphicsDevice_DX11()
    {
        blendStateCache.clear();
        rasterizerStateCache.clear();
        depthStencilStateCache.clear();
        samplerCache.clear();

        // Command Lists
        {
            for (uint32_t i = 0; i < kCommandListCount; i++)
            {
                if (!commandLists[i])
                    break;

                delete commandLists[i];
            }
        }

        // SwapChain
        {
            SafeRelease(renderTargetView);
            SafeRelease(backBufferTexture);
            SafeRelease(swapChain);
        }

        SafeRelease(immediateContext);

#ifdef _DEBUG
        ULONG refCount = device->Release();
        if (refCount)
        {
            LOGD("Direct3D11: There are {} unreleased references left on the D3D device!", refCount);

            ID3D11Debug* d3d11Debug = nullptr;
            if (SUCCEEDED(device->QueryInterface(&d3d11Debug)))
            {
                d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
                d3d11Debug->Release();
            }

        }
        else
        {
            LOGD("Direct3D11: No memory leaks detected");
        }
#else
        device->Release();
#endif

        device = nullptr;

        // DXGI Factory
        {
            SafeRelease(dxgiFactory2);

#ifdef _DEBUG
            IDXGIDebug1* dxgiDebug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1 &&
                SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#endif
            {
                dxgiDebug1->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug1->Release();
            }
#endif
        }
}

    void GraphicsDevice_DX11::CreateFactory()
    {
        SafeRelease(dxgiFactory2);

#if defined(_DEBUG) 
        bool debugDXGI = false;
        {
            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                debugDXGI = true;

                ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory2)));

                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
            }
        }

        if (!debugDXGI)
#endif
        {
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory2)));
        }
    }

    void GraphicsDevice_DX11::GetAdapter(IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6 = nullptr;
        HRESULT hr = dxgiFactory2->QueryInterface(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                    IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif

                break;
            }
        }
        SafeRelease(dxgiFactory6);
#endif

        if (!adapter)
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory2->EnumAdapters1(
                    adapterIndex,
                    adapter.ReleaseAndGetAddressOf()));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif

                break;
            }
        }

        *ppAdapter = adapter.Detach();
    }

    void GraphicsDevice_DX11::CreateBackBufferResources()
    {
        HRESULT hr;

        hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferTexture));
        if (FAILED(hr)) {
            //wiHelper::messageBox("BackBuffer creation Failed!", "Error!");
            //wiPlatform::Exit();
        }

        hr = device->CreateRenderTargetView(backBufferTexture, nullptr, &renderTargetView);
        if (FAILED(hr)) {
            //wiHelper::messageBox("Main Rendertarget creation Failed!", "Error!");
            //wiPlatform::Exit();
        }
    }

    void GraphicsDevice_DX11::Resize(uint32_t width, uint32_t height)
    {
        if ((width != backbufferWidth
            || height != backbufferHeight) && width > 0 && height > 0)
        {
            backbufferWidth = width;
            backbufferHeight = height;

            SafeRelease(renderTargetView);
            SafeRelease(backBufferTexture);

            HRESULT hr = swapChain->ResizeBuffers(GetBackBufferCount(), width, height, PixelFormatToDXGIFormat(GetBackBufferFormat()), 0);
            assert(SUCCEEDED(hr));

            CreateBackBufferResources();
        }
    }

    Texture GraphicsDevice_DX11::GetBackBuffer()
    {
        auto internal_state = std::make_shared<Texture_DX11>();
        internal_state->resource = backBufferTexture;

        Texture result;
        result.internal_state = internal_state;
        result.type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;

        D3D11_TEXTURE2D_DESC desc;
        backBufferTexture->GetDesc(&desc);
        result.desc = _ConvertTextureDesc_Inv(&desc);

        return result;
    }

    RefPtr<GraphicsBuffer> GraphicsDevice_DX11::CreateBuffer(const GPUBufferDesc& desc, const void* initialData)
    {
        D3D11_BUFFER_DESC d3d11Desc = {};
        d3d11Desc.ByteWidth = desc.ByteWidth;
        d3d11Desc.Usage = _ConvertUsage(desc.Usage);
        d3d11Desc.BindFlags = _ParseBindFlags(desc.BindFlags);
        d3d11Desc.CPUAccessFlags = _ParseCPUAccessFlags(desc.CPUAccessFlags);
        d3d11Desc.MiscFlags = _ParseResourceMiscFlags(desc.MiscFlags);
        d3d11Desc.StructureByteStride = desc.StructureByteStride;

        D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        D3D11_SUBRESOURCE_DATA initialResourceData = {};
        if (initialData != nullptr)
        {
            initialResourceData.pSysMem = initialData;
            initialDataPtr = &initialResourceData;
        }

        RefPtr<Buffer_DX11> result(new Buffer_DX11(desc));
        HRESULT hr = device->CreateBuffer(&d3d11Desc, initialDataPtr, &result->handle);
        if (FAILED(hr))
        {
            LOGE("D3D11: Create buffer failed");
            return nullptr;
        }


        if (SUCCEEDED(hr))
        {
            // Create resource views if needed
            if (desc.BindFlags & BIND_SHADER_RESOURCE)
            {
                CreateSubresource(result, SRV, 0);
            }
            if (desc.BindFlags & BIND_UNORDERED_ACCESS)
            {
                CreateSubresource(result, UAV, 0);
            }
        }

        return result;
    }

    bool GraphicsDevice_DX11::CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture)
    {
        auto internal_state = std::make_shared<Texture_DX11>();
        pTexture->internal_state = internal_state;
        pTexture->type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;

        pTexture->desc = *pDesc;

        std::vector<D3D11_SUBRESOURCE_DATA> data;
        if (pInitialData != nullptr)
        {
            uint32_t dataCount = pDesc->ArraySize * Alimer::Max(1u, pDesc->MipLevels);
            data.resize(dataCount);
            for (uint32_t slice = 0; slice < dataCount; ++slice)
            {
                data[slice] = _ConvertSubresourceData(pInitialData[slice]);
            }
        }

        HRESULT hr = S_OK;

        switch (pTexture->desc.type)
        {
        case TextureDesc::TEXTURE_1D:
        {
            D3D11_TEXTURE1D_DESC desc = _ConvertTextureDesc1D(&pTexture->desc);
            hr = device->CreateTexture1D(&desc, data.data(), (ID3D11Texture1D**)internal_state->resource.ReleaseAndGetAddressOf());
        }
        break;
        case TextureDesc::TEXTURE_2D:
        {
            D3D11_TEXTURE2D_DESC desc = _ConvertTextureDesc2D(&pTexture->desc);
            hr = device->CreateTexture2D(&desc, data.data(), (ID3D11Texture2D**)internal_state->resource.ReleaseAndGetAddressOf());
        }
        break;
        case TextureDesc::TEXTURE_3D:
        {
            D3D11_TEXTURE3D_DESC desc = _ConvertTextureDesc3D(&pTexture->desc);
            hr = device->CreateTexture3D(&desc, data.data(), (ID3D11Texture3D**)internal_state->resource.ReleaseAndGetAddressOf());
        }
        break;
        default:
            assert(0);
            break;
        }

        assert(SUCCEEDED(hr));
        if (FAILED(hr))
            return SUCCEEDED(hr);

        if (pTexture->desc.MipLevels == 0)
        {
            pTexture->desc.MipLevels = (uint32_t)log2(Alimer::Max(pTexture->desc.Width, pTexture->desc.Height)) + 1;
        }

        if (pTexture->desc.BindFlags & BIND_RENDER_TARGET)
        {
            CreateSubresource(pTexture, RTV, 0, -1, 0, -1);
        }
        if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
        {
            CreateSubresource(pTexture, DSV, 0, -1, 0, -1);
        }
        if (pTexture->desc.BindFlags & BIND_SHADER_RESOURCE)
        {
            CreateSubresource(pTexture, SRV, 0, -1, 0, -1);
        }
        if (pTexture->desc.BindFlags & BIND_UNORDERED_ACCESS)
        {
            CreateSubresource(pTexture, UAV, 0, -1, 0, -1);
        }

        return SUCCEEDED(hr);
    }

    bool GraphicsDevice_DX11::CreateShader(ShaderStage stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader)
    {
        pShader->code.resize(BytecodeLength);
        std::memcpy(pShader->code.data(), pShaderBytecode, BytecodeLength);
        pShader->stage = stage;

        HRESULT hr = E_FAIL;

        switch (stage)
        {
        case ShaderStage::Vertex:
        {
            auto internal_state = std::make_shared<VertexShader_DX11>();
            pShader->internal_state = internal_state;
            hr = device->CreateVertexShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
        }
        break;
        case ShaderStage::Hull:
        {
            auto internal_state = std::make_shared<HullShader_DX11>();
            pShader->internal_state = internal_state;
            hr = device->CreateHullShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
        }
        break;
        case ShaderStage::Domain:
        {
            auto internal_state = std::make_shared<DomainShader_DX11>();
            pShader->internal_state = internal_state;
            hr = device->CreateDomainShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
        }
        break;
        case ShaderStage::Geometry:
        {
            auto internal_state = std::make_shared<GeometryShader_DX11>();
            pShader->internal_state = internal_state;
            hr = device->CreateGeometryShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
        }
        break;
        case ShaderStage::Fragment:
        {
            auto internal_state = std::make_shared<PixelShader_DX11>();
            pShader->internal_state = internal_state;
            hr = device->CreatePixelShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
        }
        break;
        case ShaderStage::Compute:
        {
            auto internal_state = std::make_shared<ComputeShader_DX11>();
            pShader->internal_state = internal_state;
            hr = device->CreateComputeShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
        }
        break;
        }

        assert(SUCCEEDED(hr));

        return SUCCEEDED(hr);
    }

    bool GraphicsDevice_DX11::CreateShader(ShaderStage stage, const char* source, const char* entryPoint, Shader* pShader)
    {
#if defined(ALIMER_DISABLE_SHADER_COMPILER)
        return false;
#else
        if (!LoadD3DCompilerDLL())
        {
            return false;
        }

        ComPtr<ID3DBlob> output = nullptr;
        ID3DBlob* errors_or_warnings = nullptr;

        const char* target = "vs_5_0";
        switch (stage)
        {
        case ShaderStage::Hull:
            target = "hs_5_0";
            break;
        case ShaderStage::Domain:
            target = "ds_5_0";
            break;
        case ShaderStage::Geometry:
            target = "gs_5_0";
            break;
        case ShaderStage::Fragment:
            target = "ps_5_0";
            break;
        case ShaderStage::Compute:
            target = "cs_5_0";
            break;
        }

        UINT compileFlags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG;
        compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        HRESULT hr = D3DCompile_func(
            source,
            strlen(source),
            nullptr,
            NULL,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint,
            target,
            compileFlags,
            0,
            &output,
            &errors_or_warnings);

        if (FAILED(hr)) {
            const char* errorMsg = (LPCSTR)errors_or_warnings->GetBufferPointer();
            //errors->Reset(errors_or_warnings->GetBufferPointer(), static_cast<uint32_t>(errors_or_warnings->GetBufferSize()));
            return false;
        }

        return CreateShader(stage, output->GetBufferPointer(), output->GetBufferSize(), pShader);
#endif
    }

    ID3D11DepthStencilState* GraphicsDevice_DX11::GetDepthStencilState(const DepthStencilStateDescriptor& descriptor)
    {
        std::size_t hash = std::hash<DepthStencilStateDescriptor>{}(descriptor);

        auto it = depthStencilStateCache.find(hash);
        if (it == depthStencilStateCache.end())
        {
            D3D11_DEPTH_STENCIL_DESC d3dDesc;
            d3dDesc.DepthEnable = descriptor.depthCompare != CompareFunction::Always || descriptor.depthWriteEnabled;
            d3dDesc.DepthWriteMask = descriptor.depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
            d3dDesc.DepthFunc = _ConvertComparisonFunc(descriptor.depthCompare);
            d3dDesc.StencilEnable = StencilTestEnabled(&descriptor) ? TRUE : FALSE;
            d3dDesc.StencilReadMask = descriptor.stencilReadMask;
            d3dDesc.StencilWriteMask = descriptor.stencilWriteMask;
            d3dDesc.FrontFace = _ConvertStencilOpDesc(descriptor.stencilFront);
            d3dDesc.BackFace = _ConvertStencilOpDesc(descriptor.stencilBack);

            ComPtr<ID3D11DepthStencilState> depthStencilState;
            ThrowIfFailed(device->CreateDepthStencilState(&d3dDesc, depthStencilState.GetAddressOf()));
            it = depthStencilStateCache.insert({ hash, std::move(depthStencilState) }).first;
        }

        return it->second.Get();
    }

    ID3D11RasterizerState* GraphicsDevice_DX11::GetRasterizerState(const RasterizationStateDescriptor& descriptor, uint32_t sampleCount)
    {
        std::size_t hash = std::hash<RasterizationStateDescriptor>{}(descriptor);

        auto it = rasterizerStateCache.find(hash);
        if (it == rasterizerStateCache.end())
        {
            D3D11_RASTERIZER_DESC desc;
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = _ConvertCullMode(descriptor.cullMode);
            desc.FrontCounterClockwise = (descriptor.frontFace == FrontFace::CCW) ? TRUE : FALSE;
            desc.DepthBias = descriptor.depthBias;
            desc.DepthBiasClamp = descriptor.depthBiasClamp;
            desc.SlopeScaledDepthBias = descriptor.depthBiasSlopeScale;
            desc.DepthClipEnable = descriptor.depthClipEnable;
            desc.ScissorEnable = TRUE;
            desc.MultisampleEnable = sampleCount > 1 ? TRUE : FALSE;
            desc.AntialiasedLineEnable = FALSE;

            if (CONSERVATIVE_RASTERIZATION && descriptor.conservativeRasterizationEnable)
            {
                ID3D11Device3* device3;
                if (SUCCEEDED(device->QueryInterface(&device3)))
                {
                    D3D11_RASTERIZER_DESC2 desc2;
                    desc2.FillMode = desc.FillMode;
                    desc2.CullMode = desc.CullMode;
                    desc2.FrontCounterClockwise = desc.FrontCounterClockwise;
                    desc2.DepthBias = desc.DepthBias;
                    desc2.DepthBiasClamp = desc.DepthBiasClamp;
                    desc2.SlopeScaledDepthBias = desc.SlopeScaledDepthBias;
                    desc2.DepthClipEnable = desc.DepthClipEnable;
                    desc2.ScissorEnable = desc.ScissorEnable;
                    desc2.MultisampleEnable = desc.MultisampleEnable;
                    desc2.AntialiasedLineEnable = desc.AntialiasedLineEnable;
                    desc2.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON;
                    desc2.ForcedSampleCount = (RASTERIZER_ORDERED_VIEWS ? descriptor.forcedSampleCount : 0);

                    ComPtr<ID3D11RasterizerState2> rasterizerState2;
                    ThrowIfFailed(device3->CreateRasterizerState2(&desc2, &rasterizerState2));
                    device3->Release();
                    it = rasterizerStateCache.insert({ hash, std::move(rasterizerState2) }).first;
                    return it->second.Get();
                }
            }
            else if (RASTERIZER_ORDERED_VIEWS && descriptor.forcedSampleCount > 0)
            {
                D3D11_RASTERIZER_DESC1 desc1;
                desc1.FillMode = desc.FillMode;
                desc1.CullMode = desc.CullMode;
                desc1.FrontCounterClockwise = desc.FrontCounterClockwise;
                desc1.DepthBias = desc.DepthBias;
                desc1.DepthBiasClamp = desc.DepthBiasClamp;
                desc1.SlopeScaledDepthBias = desc.SlopeScaledDepthBias;
                desc1.DepthClipEnable = desc.DepthClipEnable;
                desc1.ScissorEnable = desc.ScissorEnable;
                desc1.MultisampleEnable = desc.MultisampleEnable;
                desc1.AntialiasedLineEnable = desc.AntialiasedLineEnable;
                desc1.ForcedSampleCount = descriptor.forcedSampleCount;

                ComPtr<ID3D11RasterizerState1> rasterizerState1;
                ThrowIfFailed(device->CreateRasterizerState1(&desc1, &rasterizerState1));

                it = rasterizerStateCache.insert({ hash, std::move(rasterizerState1) }).first;
                return it->second.Get();
            }

            ComPtr<ID3D11RasterizerState> rasterizerState;
            ThrowIfFailed(device->CreateRasterizerState(&desc, &rasterizerState));
            it = rasterizerStateCache.insert({ hash, std::move(rasterizerState) }).first;
        }

        return it->second.Get();
    }

    ID3D11BlendState1* GraphicsDevice_DX11::GetBlendState(const RenderPipelineDescriptor* descriptor)
    {
        size_t hash = 0;
        CombineHash(hash, descriptor->alphaToCoverageEnable);
        for (uint32_t i = 0; i < kMaxColorAttachments; ++i)
        {
            CombineHash(hash, descriptor->colorAttachments[i]);
        }

        auto it = blendStateCache.find(hash);
        if (it == blendStateCache.end())
        {
            D3D11_BLEND_DESC1 d3d11Desc = {};
            d3d11Desc.AlphaToCoverageEnable = descriptor->alphaToCoverageEnable;
            d3d11Desc.IndependentBlendEnable = TRUE;

            for (uint32_t i = 0; i < kMaxColorAttachments; ++i)
            {
                d3d11Desc.RenderTarget[i] = _ConvertColorAttachment(&descriptor->colorAttachments[i]);
            }

            ComPtr<ID3D11BlendState1> state;
            ThrowIfFailed(device->CreateBlendState1(&d3d11Desc, &state));

            it = blendStateCache.insert({ hash, std::move(state) }).first;
            return it->second.Get();
        }

        return it->second.Get();
    }

    RefPtr<Sampler> GraphicsDevice_DX11::CreateSampler(const SamplerDescriptor* descriptor)
    {
        size_t hash = std::hash<SamplerDescriptor>{}(*descriptor);

        RefPtr<Sampler_DX11> result(new Sampler_DX11());
        auto it = samplerCache.find(hash);
        if (it != samplerCache.end())
        {
            result->handle = it->second.Get();
            return result;
        }

        D3D11_SAMPLER_DESC desc;
        desc.Filter = _ConvertFilter(
            descriptor->minFilter,
            descriptor->magFilter,
            descriptor->mipmapFilter,
            descriptor->compareFunction != CompareFunction::Undefined,
            descriptor->maxAnisotropy > 1
        );

        desc.AddressU = _ConvertAddressMode(descriptor->addressModeU);
        desc.AddressV = _ConvertAddressMode(descriptor->addressModeV);
        desc.AddressW = _ConvertAddressMode(descriptor->addressModeW);
        desc.MipLODBias = descriptor->mipLodBias;
        desc.MaxAnisotropy = descriptor->maxAnisotropy;
        if (descriptor->compareFunction != CompareFunction::Undefined) {
            desc.ComparisonFunc = _ConvertComparisonFunc(descriptor->compareFunction);
        }
        else {
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        }
        switch (descriptor->borderColor)
        {
        case SamplerBorderColor::OpaqueBlack:
            desc.BorderColor[0] = 0.0f;
            desc.BorderColor[1] = 0.0f;
            desc.BorderColor[2] = 0.0f;
            desc.BorderColor[3] = 1.0f;
            break;

        case SamplerBorderColor::OpaqueWhite:
            desc.BorderColor[0] = 1.0f;
            desc.BorderColor[1] = 1.0f;
            desc.BorderColor[2] = 1.0f;
            desc.BorderColor[3] = 1.0f;
            break;

        default:
            desc.BorderColor[0] = 0.0f;
            desc.BorderColor[1] = 0.0f;
            desc.BorderColor[2] = 0.0f;
            desc.BorderColor[3] = 0.0f;
            break;
        }

        desc.MinLOD = descriptor->lodMinClamp;
        desc.MaxLOD = descriptor->lodMaxClamp;

        ComPtr<ID3D11SamplerState> state;
        HRESULT hr = device->CreateSamplerState(&desc, state.GetAddressOf());
        if (FAILED(hr))
        {
            return nullptr;
        }

        it = samplerCache.insert({ hash, std::move(state) }).first;
        result->handle = it->second.Get();
        return result;
    }

    bool GraphicsDevice_DX11::CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery)
    {
        auto internal_state = std::make_shared<Query_DX11>();
        pQuery->internal_state = internal_state;

        pQuery->desc = *pDesc;

        D3D11_QUERY_DESC desc;
        desc.MiscFlags = 0;
        desc.Query = D3D11_QUERY_EVENT;
        if (pDesc->Type == GPU_QUERY_TYPE_EVENT)
        {
            desc.Query = D3D11_QUERY_EVENT;
        }
        else if (pDesc->Type == GPU_QUERY_TYPE_OCCLUSION)
        {
            desc.Query = D3D11_QUERY_OCCLUSION;
        }
        else if (pDesc->Type == GPU_QUERY_TYPE_OCCLUSION_PREDICATE)
        {
            desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
        }
        else if (pDesc->Type == GPU_QUERY_TYPE_TIMESTAMP)
        {
            desc.Query = D3D11_QUERY_TIMESTAMP;
        }
        else if (pDesc->Type == GPU_QUERY_TYPE_TIMESTAMP_DISJOINT)
        {
            desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        }

        HRESULT hr = device->CreateQuery(&desc, &internal_state->resource);
        assert(SUCCEEDED(hr));

        return SUCCEEDED(hr);
    }

    bool GraphicsDevice_DX11::CreateRenderPipelineCore(const RenderPipelineDescriptor* descriptor, RenderPipeline** pipeline)
    {
        RefPtr<PipelineState_DX11> internal_state(new PipelineState_DX11());
        internal_state->desc = *descriptor;
        internal_state->rasterizerState = GetRasterizerState(descriptor->rasterizationState, descriptor->sampleCount);
        internal_state->depthStencilState = GetDepthStencilState(descriptor->depthStencilState);
        internal_state->blendState = GetBlendState(descriptor);

        // TODO: Cache
        uint32_t inputElementsCount = 0;
        std::array<D3D11_INPUT_ELEMENT_DESC, kMaxVertexAttributes> inputElements;
        for (uint32_t i = 0; i < kMaxVertexAttributes; ++i)
        {
            const VertexAttributeDescriptor* attrDesc = &descriptor->vertexDescriptor.attributes[i];
            if (attrDesc->format == VertexFormat::Invalid) {
                break;
            }

            const VertexBufferLayoutDescriptor* layoutDesc = &descriptor->vertexDescriptor.layouts[i];

            D3D11_INPUT_ELEMENT_DESC* inputElementDesc = &inputElements[inputElementsCount++];
            inputElementDesc->SemanticName = "ATTRIBUTE";
            inputElementDesc->SemanticIndex = i;
            inputElementDesc->Format = D3DConvertVertexFormat(attrDesc->format);
            inputElementDesc->InputSlot = attrDesc->bufferIndex;
            inputElementDesc->AlignedByteOffset = attrDesc->offset; // D3D11_APPEND_ALIGNED_ELEMENT; // attrDesc->offset;
            if (layoutDesc->stepMode == InputStepMode::Vertex)
            {
                inputElementDesc->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                inputElementDesc->InstanceDataStepRate = 0;
            }
            else
            {
                inputElementDesc->InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                inputElementDesc->InstanceDataStepRate = 1;
            }
        }

        HRESULT hr = device->CreateInputLayout(
            inputElements.data(),
            inputElementsCount,
            descriptor->vs->code.data(),
            descriptor->vs->code.size(),
            internal_state->inputLayout.ReleaseAndGetAddressOf());

        internal_state->primitiveTopology = D3DPrimitiveTopology(descriptor->primitiveTopology);

        *pipeline = internal_state.Detach();
        return true;
    }

    bool GraphicsDevice_DX11::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
    {
        renderpass->internal_state = emptyresource;

        renderpass->desc = *pDesc;

        return true;
    }

    int GraphicsDevice_DX11::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
    {
        auto internal_state = to_internal(texture);

        switch (type)
        {
        case Alimer::SRV:
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

            // Try to resolve resource format:
            switch (texture->desc.format)
            {
            case PixelFormat::FORMAT_R16_TYPELESS:
                srv_desc.Format = DXGI_FORMAT_R16_UNORM;
                break;
            case PixelFormat::FORMAT_R32_TYPELESS:
                srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case PixelFormat::FORMAT_R24G8_TYPELESS:
                srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;
            case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                srv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                break;
            default:
                srv_desc.Format = PixelFormatToDXGIFormat(texture->desc.format);
                break;
            }

            if (texture->desc.type == TextureDesc::TEXTURE_1D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                    srv_desc.Texture1DArray.FirstArraySlice = firstSlice;
                    srv_desc.Texture1DArray.ArraySize = sliceCount;
                    srv_desc.Texture1DArray.MostDetailedMip = firstMip;
                    srv_desc.Texture1DArray.MipLevels = mipCount;
                }
                else
                {
                    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                    srv_desc.Texture1D.MostDetailedMip = firstMip;
                    srv_desc.Texture1D.MipLevels = mipCount;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_2D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    if (texture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
                    {
                        if (texture->desc.ArraySize > 6)
                        {
                            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                            srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
                            srv_desc.TextureCubeArray.NumCubes = Alimer::Min(texture->desc.ArraySize, sliceCount) / 6;
                            srv_desc.TextureCubeArray.MostDetailedMip = firstMip;
                            srv_desc.TextureCubeArray.MipLevels = mipCount;
                        }
                        else
                        {
                            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                            srv_desc.TextureCube.MostDetailedMip = firstMip;
                            srv_desc.TextureCube.MipLevels = mipCount;
                        }
                    }
                    else
                    {
                        if (texture->desc.SampleCount > 1)
                        {
                            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                            srv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
                            srv_desc.Texture2DMSArray.ArraySize = sliceCount;
                        }
                        else
                        {
                            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                            srv_desc.Texture2DArray.FirstArraySlice = firstSlice;
                            srv_desc.Texture2DArray.ArraySize = sliceCount;
                            srv_desc.Texture2DArray.MostDetailedMip = firstMip;
                            srv_desc.Texture2DArray.MipLevels = mipCount;
                        }
                    }
                }
                else
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        srv_desc.Texture2D.MostDetailedMip = firstMip;
                        srv_desc.Texture2D.MipLevels = mipCount;
                    }
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_3D)
            {
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                srv_desc.Texture3D.MostDetailedMip = firstMip;
                srv_desc.Texture3D.MipLevels = mipCount;
            }

            ComPtr<ID3D11ShaderResourceView> srv;
            HRESULT hr = device->CreateShaderResourceView(internal_state->resource.Get(), &srv_desc, &srv);
            if (SUCCEEDED(hr))
            {
                if (!internal_state->srv)
                {
                    internal_state->srv = srv;
                    return -1;
                }
                internal_state->subresources_srv.push_back(srv);
                return int(internal_state->subresources_srv.size() - 1);
            }
            else
            {
                assert(0);
            }
        }
        break;
        case Alimer::UAV:
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};

            // Try to resolve resource format:
            switch (texture->desc.format)
            {
            case PixelFormat::FORMAT_R16_TYPELESS:
                uav_desc.Format = DXGI_FORMAT_R16_UNORM;
                break;
            case PixelFormat::FORMAT_R32_TYPELESS:
                uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case PixelFormat::FORMAT_R24G8_TYPELESS:
                uav_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;
            case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                uav_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                break;
            default:
                uav_desc.Format = PixelFormatToDXGIFormat(texture->desc.format);
                break;
            }

            if (texture->desc.type == TextureDesc::TEXTURE_1D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                    uav_desc.Texture1DArray.FirstArraySlice = firstSlice;
                    uav_desc.Texture1DArray.ArraySize = sliceCount;
                    uav_desc.Texture1DArray.MipSlice = firstMip;
                }
                else
                {
                    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                    uav_desc.Texture1D.MipSlice = firstMip;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_2D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                    uav_desc.Texture2DArray.FirstArraySlice = firstSlice;
                    uav_desc.Texture2DArray.ArraySize = sliceCount;
                    uav_desc.Texture2DArray.MipSlice = firstMip;
                }
                else
                {
                    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    uav_desc.Texture2D.MipSlice = firstMip;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_3D)
            {
                uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                uav_desc.Texture3D.MipSlice = firstMip;
                uav_desc.Texture3D.FirstWSlice = 0;
                uav_desc.Texture3D.WSize = -1;
            }

            ComPtr<ID3D11UnorderedAccessView> uav;
            HRESULT hr = device->CreateUnorderedAccessView(internal_state->resource.Get(), &uav_desc, &uav);
            if (SUCCEEDED(hr))
            {
                if (!internal_state->uav)
                {
                    internal_state->uav = uav;
                    return -1;
                }
                internal_state->subresources_uav.push_back(uav);
                return int(internal_state->subresources_uav.size() - 1);
            }
            else
            {
                assert(0);
            }
        }
        break;
        case Alimer::RTV:
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};

            // Try to resolve resource format:
            switch (texture->desc.format)
            {
            case PixelFormat::FORMAT_R16_TYPELESS:
                rtv_desc.Format = DXGI_FORMAT_R16_UNORM;
                break;
            case PixelFormat::FORMAT_R32_TYPELESS:
                rtv_desc.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case PixelFormat::FORMAT_R24G8_TYPELESS:
                rtv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;
            case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                rtv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                break;
            default:
                rtv_desc.Format = PixelFormatToDXGIFormat(texture->desc.format);
                break;
            }

            if (texture->desc.type == TextureDesc::TEXTURE_1D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                    rtv_desc.Texture1DArray.FirstArraySlice = firstSlice;
                    rtv_desc.Texture1DArray.ArraySize = sliceCount;
                    rtv_desc.Texture1DArray.MipSlice = firstMip;
                }
                else
                {
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                    rtv_desc.Texture1D.MipSlice = firstMip;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_2D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        rtv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
                        rtv_desc.Texture2DMSArray.ArraySize = sliceCount;
                    }
                    else
                    {
                        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                        rtv_desc.Texture2DArray.FirstArraySlice = firstSlice;
                        rtv_desc.Texture2DArray.ArraySize = sliceCount;
                        rtv_desc.Texture2DArray.MipSlice = firstMip;
                    }
                }
                else
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                        rtv_desc.Texture2D.MipSlice = firstMip;
                    }
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_3D)
            {
                rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                rtv_desc.Texture3D.MipSlice = firstMip;
                rtv_desc.Texture3D.FirstWSlice = 0;
                rtv_desc.Texture3D.WSize = -1;
            }

            ComPtr<ID3D11RenderTargetView> rtv;
            HRESULT hr = device->CreateRenderTargetView(internal_state->resource.Get(), &rtv_desc, &rtv);
            if (SUCCEEDED(hr))
            {
                if (!internal_state->rtv)
                {
                    internal_state->rtv = rtv;
                    return -1;
                }
                internal_state->subresources_rtv.push_back(rtv);
                return int(internal_state->subresources_rtv.size() - 1);
            }
            else
            {
                assert(0);
            }
        }
        break;
        case Alimer::DSV:
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};

            // Try to resolve resource format:
            switch (texture->desc.format)
            {
            case PixelFormat::FORMAT_R16_TYPELESS:
                dsv_desc.Format = DXGI_FORMAT_D16_UNORM;
                break;
            case PixelFormat::FORMAT_R32_TYPELESS:
                dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
                break;
            case PixelFormat::FORMAT_R24G8_TYPELESS:
                dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                break;
            case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
                break;
            default:
                dsv_desc.Format = PixelFormatToDXGIFormat(texture->desc.format);
                break;
            }

            if (texture->desc.type == TextureDesc::TEXTURE_1D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                    dsv_desc.Texture1DArray.FirstArraySlice = firstSlice;
                    dsv_desc.Texture1DArray.ArraySize = sliceCount;
                    dsv_desc.Texture1DArray.MipSlice = firstMip;
                }
                else
                {
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                    dsv_desc.Texture1D.MipSlice = firstMip;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_2D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                        dsv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
                        dsv_desc.Texture2DMSArray.ArraySize = sliceCount;
                    }
                    else
                    {
                        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                        dsv_desc.Texture2DArray.FirstArraySlice = firstSlice;
                        dsv_desc.Texture2DArray.ArraySize = sliceCount;
                        dsv_desc.Texture2DArray.MipSlice = firstMip;
                    }
                }
                else
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                        dsv_desc.Texture2D.MipSlice = firstMip;
                    }
                }
            }

            ComPtr<ID3D11DepthStencilView> dsv;
            HRESULT hr = device->CreateDepthStencilView(internal_state->resource.Get(), &dsv_desc, &dsv);
            if (SUCCEEDED(hr))
            {
                if (!internal_state->dsv)
                {
                    internal_state->dsv = dsv;
                    return -1;
                }
                internal_state->subresources_dsv.push_back(dsv);
                return int(internal_state->subresources_dsv.size() - 1);
            }
            else
            {
                assert(0);
            }
        }
        break;
        default:
            break;
        }
        return -1;
    }

    int GraphicsDevice_DX11::CreateSubresource(GraphicsBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size)
    {
        Buffer_DX11* buffer_d3d11 = static_cast<Buffer_DX11*>(buffer);
        const GPUBufferDesc& desc = buffer->GetDesc();
        HRESULT hr = E_FAIL;

        switch (type)
        {
        case Alimer::SRV:
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

            if (desc.MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
            {
                // This is a Raw Buffer
                srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
                srv_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
                srv_desc.BufferEx.FirstElement = (UINT)offset / sizeof(uint32_t);
                srv_desc.BufferEx.NumElements = Alimer::Min((UINT)size, desc.ByteWidth - (UINT)offset) / sizeof(uint32_t);
            }
            else if (desc.MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
            {
                // This is a Structured Buffer
                srv_desc.Format = DXGI_FORMAT_UNKNOWN;
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
                srv_desc.BufferEx.FirstElement = (UINT)offset / desc.StructureByteStride;
                srv_desc.BufferEx.NumElements = Alimer::Min((UINT)size, desc.ByteWidth - (UINT)offset) / desc.StructureByteStride;
            }
            else
            {
                // This is a Typed Buffer
                uint32_t stride = GetPixelFormatSize(desc.format);
                srv_desc.Format = PixelFormatToDXGIFormat(desc.format);
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                srv_desc.Buffer.FirstElement = (UINT)offset / stride;
                srv_desc.Buffer.NumElements = Alimer::Min((UINT)size, desc.ByteWidth - (UINT)offset) / stride;
            }

            ComPtr<ID3D11ShaderResourceView> srv;
            hr = device->CreateShaderResourceView(buffer_d3d11->handle, &srv_desc, &srv);

            if (SUCCEEDED(hr))
            {
                if (buffer_d3d11->srv == nullptr)
                {
                    buffer_d3d11->srv = srv;
                    return -1;
                }
                else
                {
                    buffer_d3d11->subresources_srv.push_back(srv);
                    return int(buffer_d3d11->subresources_srv.size() - 1);
                }
            }
            else
            {
                assert(0);
            }
        }
        break;
        case Alimer::UAV:
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

            if (desc.MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
            {
                // This is a Raw Buffer
                uav_desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
                uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
                uav_desc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
                uav_desc.Buffer.NumElements = Alimer::Min((UINT)size, desc.ByteWidth - (UINT)offset) / sizeof(uint32_t);
            }
            else if (desc.MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
            {
                // This is a Structured Buffer
                uav_desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
                uav_desc.Buffer.FirstElement = (UINT)offset / desc.StructureByteStride;
                uav_desc.Buffer.NumElements = Alimer::Min((UINT)size, desc.ByteWidth - (UINT)offset) / desc.StructureByteStride;
            }
            else
            {
                // This is a Typed Buffer
                uint32_t stride = GetPixelFormatSize(desc.format);
                uav_desc.Format = PixelFormatToDXGIFormat(desc.format);
                uav_desc.Buffer.FirstElement = (UINT)offset / stride;
                uav_desc.Buffer.NumElements = Alimer::Min((UINT)size, desc.ByteWidth - (UINT)offset) / stride;
            }

            ComPtr<ID3D11UnorderedAccessView> uav;
            hr = device->CreateUnorderedAccessView(buffer_d3d11->handle, &uav_desc, &uav);

            if (SUCCEEDED(hr))
            {
                if (buffer_d3d11->uav == nullptr)
                {
                    buffer_d3d11->uav = uav;
                    return -1;
                }
                else
                {
                    buffer_d3d11->subresources_uav.push_back(uav);
                    return int(buffer_d3d11->subresources_uav.size() - 1);
                }
            }
            else
            {
                assert(0);
            }
        }
        break;
        default:
            assert(0);
            break;
        }
        return -1;
    }

    void GraphicsDevice_DX11::Map(const GPUResource* resource, Mapping* mapping)
    {
        auto internal_state = to_internal(resource);

        D3D11_MAPPED_SUBRESOURCE map_result = {};
        D3D11_MAP map_type = D3D11_MAP_READ_WRITE;
        if (mapping->_flags & Mapping::FLAG_READ)
        {
            if (mapping->_flags & Mapping::FLAG_WRITE)
            {
                map_type = D3D11_MAP_READ_WRITE;
            }
            else
            {
                map_type = D3D11_MAP_READ;
            }
        }
        else if (mapping->_flags & Mapping::FLAG_WRITE)
        {
            map_type = D3D11_MAP_WRITE_NO_OVERWRITE;
        }
        HRESULT hr = immediateContext->Map(internal_state->resource.Get(), 0, map_type, D3D11_MAP_FLAG_DO_NOT_WAIT, &map_result);
        if (SUCCEEDED(hr))
        {
            mapping->data = map_result.pData;
            mapping->rowpitch = map_result.RowPitch;
        }
        else
        {
            assert(0);
            mapping->data = nullptr;
            mapping->rowpitch = 0;
        }
    }
    void GraphicsDevice_DX11::Unmap(const GPUResource* resource)
    {
        auto internal_state = to_internal(resource);
        immediateContext->Unmap(internal_state->resource.Get(), 0);
    }
    bool GraphicsDevice_DX11::QueryRead(const GPUQuery* query, GPUQueryResult* result)
    {
        const uint32_t _flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;

        auto internal_state = to_internal(query);
        ID3D11Query* QUERY = internal_state->resource.Get();

        HRESULT hr = S_OK;
        switch (query->desc.Type)
        {
        case GPU_QUERY_TYPE_TIMESTAMP:
            hr = immediateContext->GetData(QUERY, &result->result_timestamp, sizeof(uint64_t), _flags);
            break;
        case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
        {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT _temp;
            hr = immediateContext->GetData(QUERY, &_temp, sizeof(_temp), _flags);
            result->result_timestamp_frequency = _temp.Frequency;
        }
        break;
        case GPU_QUERY_TYPE_EVENT:
        case GPU_QUERY_TYPE_OCCLUSION:
            hr = immediateContext->GetData(QUERY, &result->result_passed_sample_count, sizeof(uint64_t), _flags);
            break;
        case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
        {
            BOOL passed = FALSE;
            hr = immediateContext->GetData(QUERY, &passed, sizeof(BOOL), _flags);
            result->result_passed_sample_count = (uint64_t)passed;
            break;
        }
        }

        return hr != S_FALSE;
    }

    void GraphicsDevice_DX11::SetName(GPUResource* pResource, const char* name)
    {
#ifdef _DEBUG
        auto internal_state = to_internal(pResource);
        internal_state->resource->SetPrivateData(g_D3DDebugObjectName, (UINT)strlen(name), name);
#else
        ALIMER_UNUSED(pResource);
        ALIMER_UNUSED(name);
#endif
    }

    void D3D11_CommandList::PresentEnd()
    {
        device->PresentEnd();
    }

    CommandList& GraphicsDevice_DX11::BeginCommandList()
    {
        std::atomic_uint32_t cmd = commandListsCount.fetch_add(1);
        ALIMER_ASSERT(cmd < kCommandListCount);

        if (commandLists[cmd] == nullptr)
        {
            commandLists[cmd] = new D3D11_CommandList(this);
        }

        commandLists[cmd]->Reset();
        commandLists[cmd]->SetRenderPipeline(nullptr);
        commandLists[cmd]->BindComputeShader(nullptr);
        commandLists[cmd]->SetViewport(0.0f, 0.0f, (float)backbufferWidth, (float)backbufferHeight, 0.0f, 1.0f);

        return *commandLists[cmd];
    }

    void GraphicsDevice_DX11::SubmitCommandLists()
    {
        // Execute deferred command lists:
        {
            uint32_t cmd_last = commandListsCount.load();
            commandListsCount.store(0);

            for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
            {
                commandLists[cmd]->Execute(immediateContext);
            }
        }

        immediateContext->ClearState();

        FRAMECOUNT++;
    }

    void GraphicsDevice_DX11::PresentEnd()
    {
        SubmitCommandLists();

        HRESULT hr;
        if (!verticalSync)
        {
            hr = swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
        }
        else
        {
            hr = swapChain->Present(1, 0);
        }

        // If the device was reset we must completely reinitialize the renderer.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? device->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            //HandleDeviceLost();
            return;
        }
    }

    void GraphicsDevice_DX11::WaitForGPU()
    {
        immediateContext->Flush();

        GPUQuery query;
        GPUQueryDesc desc;
        desc.Type = GPU_QUERY_TYPE_EVENT;
        bool success = CreateQuery(&desc, &query);
        assert(success);
        auto internal_state = to_internal(&query);
        immediateContext->End(internal_state->resource.Get());
        BOOL result;
        while (immediateContext->GetData(internal_state->resource.Get(), &result, sizeof(result), 0) == S_FALSE);
        assert(result == TRUE);
    }

    /* D3D11_CommandList */
    D3D11_CommandList::D3D11_CommandList(GraphicsDevice_DX11* device_)
        : device(device_)
    {
        ThrowIfFailed(device->GetD3DDevice()->CreateDeferredContext1(0, &handle));
        ThrowIfFailed(handle->QueryInterface(&userDefinedAnnotation));

        // Temporary allocations will use the following buffer type:
        GPUBufferDesc frameAllocatorDesc;
        frameAllocatorDesc.ByteWidth = 1024 * 1024; // 1 MB starting size
        frameAllocatorDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_INDEX_BUFFER | BIND_VERTEX_BUFFER;
        frameAllocatorDesc.Usage = USAGE_DYNAMIC;
        frameAllocatorDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        frameAllocatorDesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        frame_allocator.buffer = device->CreateBuffer(frameAllocatorDesc, nullptr);
        ALIMER_ASSERT(frame_allocator.buffer.IsNotNull());
        frame_allocator.buffer->SetName("frame_allocator");
    }

    D3D11_CommandList::~D3D11_CommandList()
    {
        SafeRelease(userDefinedAnnotation);
        SafeRelease(handle);
        frame_allocator.buffer.Reset();
    }

    void D3D11_CommandList::Execute(ID3D11DeviceContext1* immediateContext)
    {
        ThrowIfFailed(handle->FinishCommandList(false, commandList.ReleaseAndGetAddressOf()));
        immediateContext->ExecuteCommandList(commandList.Get(), false);
    }

    void D3D11_CommandList::Reset()
    {
        prev_pt = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        active_pso = nullptr;
        dirty_pso = false;
        active_renderpass = nullptr;
        memset(raster_uavs, 0, sizeof(raster_uavs));
        raster_uavs_slot = 0;
        raster_uavs_count = 0;
        stencilRef = 0;
        blendFactor = XMFLOAT4(1, 1, 1, 1);
        computeShader = nullptr;
        prev_vs = nullptr;
        prev_ps = nullptr;
        prev_hs = nullptr;
        prev_ds = nullptr;
        prev_gs = nullptr;
        prev_blendfactor = {};
        prev_samplemask = {};
        prev_bs = nullptr;
        prev_rs = nullptr;
        prev_stencilRef = {};
        prev_dss = nullptr;
        prev_il = nullptr;

        for (uint32_t i = 0; i < kMaxViewportAndScissorRects; ++i)
        {
            scissorRects[i].bottom = INT32_MAX;
            scissorRects[i].left = INT32_MIN;
            scissorRects[i].right = INT32_MAX;
            scissorRects[i].top = INT32_MIN;
        }

        handle->RSSetScissorRects(kMaxViewportAndScissorRects, scissorRects);
    }

    void D3D11_CommandList::PresentBegin()
    {
        handle->OMSetRenderTargets(1, &device->renderTargetView, 0);
        float ClearColor[4] = { 0, 0, 0, 1.0f }; // red,green,blue,alpha
        handle->ClearRenderTargetView(device->renderTargetView, ClearColor);
    }

    void D3D11_CommandList::RenderPassBegin(const RenderPass* renderpass)
    {
        active_renderpass = renderpass;
        const RenderPassDesc& desc = renderpass->GetDesc();

        uint32_t rt_count = 0;
        ID3D11RenderTargetView* RTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        ID3D11DepthStencilView* DSV = nullptr;
        for (auto& attachment : desc.attachments)
        {
            const Texture* texture = attachment.texture;
            int subresource = attachment.subresource;
            auto internal_state = to_internal(texture);

            if (attachment.type == RenderPassAttachment::RENDERTARGET)
            {
                if (subresource < 0 || internal_state->subresources_rtv.empty())
                {
                    RTVs[rt_count] = internal_state->rtv.Get();
                }
                else
                {
                    assert(internal_state->subresources_rtv.size() > size_t(subresource) && "Invalid RTV subresource!");
                    RTVs[rt_count] = internal_state->subresources_rtv[subresource].Get();
                }

                if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
                {
                    handle->ClearRenderTargetView(RTVs[rt_count], texture->desc.clear.color);
                }

                rt_count++;
            }
            else if (attachment.type == RenderPassAttachment::DEPTH_STENCIL)
            {
                if (subresource < 0 || internal_state->subresources_dsv.empty())
                {
                    DSV = internal_state->dsv.Get();
                }
                else
                {
                    assert(internal_state->subresources_dsv.size() > size_t(subresource) && "Invalid DSV subresource!");
                    DSV = internal_state->subresources_dsv[subresource].Get();
                }

                if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
                {
                    uint32_t _flags = D3D11_CLEAR_DEPTH;
                    if (device->IsFormatStencilSupport(texture->desc.format))
                    {
                        _flags |= D3D11_CLEAR_STENCIL;
                    }

                    handle->ClearDepthStencilView(DSV, _flags, texture->desc.clear.depthstencil.depth, texture->desc.clear.depthstencil.stencil);
                }
            }
        }

        // UAVs:
        if (raster_uavs_count > 0)
        {
            const uint32_t count = raster_uavs_count;
            const uint32_t slot = raster_uavs_slot;
            handle->OMSetRenderTargetsAndUnorderedAccessViews(rt_count, RTVs, DSV, slot, count, &raster_uavs[slot], nullptr);
            raster_uavs_count = 0;
            raster_uavs_slot = 8;
        }
        else
        {
            handle->OMSetRenderTargets(rt_count, RTVs, DSV);
        }
    }

    void D3D11_CommandList::RenderPassEnd()
    {
        handle->OMSetRenderTargets(0, nullptr, nullptr);

        // Perform resolves:
        int dst_counter = 0;
        for (auto& attachment : active_renderpass->desc.attachments)
        {
            if (attachment.type == RenderPassAttachment::RESOLVE)
            {
                if (attachment.texture != nullptr)
                {
                    auto dst_internal = to_internal(attachment.texture);

                    int src_counter = 0;
                    for (auto& src : active_renderpass->desc.attachments)
                    {
                        if (src.type == RenderPassAttachment::RENDERTARGET && src.texture != nullptr)
                        {
                            if (src_counter == dst_counter)
                            {
                                auto src_internal = to_internal(src.texture);
                                handle->ResolveSubresource(dst_internal->resource.Get(), 0, src_internal->resource.Get(), 0, PixelFormatToDXGIFormat(attachment.texture->desc.format));
                                break;
                            }
                            src_counter++;
                        }
                    }
                }

                dst_counter++;
            }
        }
        active_renderpass = nullptr;
    }

    void D3D11_CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        viewports[0].TopLeftX = x;
        viewports[0].TopLeftY = y;
        viewports[0].Width = width;
        viewports[0].Height = height;
        viewports[0].MinDepth = minDepth;
        viewports[0].MaxDepth = maxDepth;
        handle->RSSetViewports(1, &viewports[0]);
    }

    void D3D11_CommandList::SetViewport(const Viewport& viewport)
    {
        viewports[0].TopLeftX = viewport.x;
        viewports[0].TopLeftY = viewport.y;
        viewports[0].Width = viewport.width;
        viewports[0].Height = viewport.height;
        viewports[0].MinDepth = viewport.minDepth;
        viewports[0].MaxDepth = viewport.maxDepth;
        handle->RSSetViewports(1, &viewports[0]);
    }

    void D3D11_CommandList::SetViewports(uint32_t viewportCount, const Viewport* pViewports)
    {
        ALIMER_ASSERT(viewportCount <= kMaxViewportAndScissorRects);

        for (uint32_t i = 0; i < viewportCount; ++i)
        {
            viewports[i].TopLeftX = pViewports[i].x;
            viewports[i].TopLeftY = pViewports[i].y;
            viewports[i].Width = pViewports[i].width;
            viewports[i].Height = pViewports[i].height;
            viewports[i].MinDepth = pViewports[i].minDepth;
            viewports[i].MaxDepth = pViewports[i].maxDepth;
        }

        handle->RSSetViewports(viewportCount, viewports);
    }

    void D3D11_CommandList::SetScissorRect(const ScissorRect& rect)
    {
        scissorRects[0].left = LONG(rect.x);
        scissorRects[0].top = LONG(rect.y);
        scissorRects[0].right = LONG(rect.x + rect.width);
        scissorRects[0].bottom = LONG(rect.y + rect.height);
        handle->RSSetScissorRects(1, &scissorRects[0]);
    }

    void D3D11_CommandList::SetScissorRects(uint32_t scissorCount, const ScissorRect* rects)
    {
        ALIMER_ASSERT(rects != nullptr);
        ALIMER_ASSERT(scissorCount <= kMaxViewportAndScissorRects);

        for (uint32_t i = 0; i < scissorCount; ++i)
        {
            scissorRects[i].left = LONG(rects[i].x);
            scissorRects[i].top = LONG(rects[i].y);
            scissorRects[i].right = LONG(rects[i].x + rects[i].width);
            scissorRects[i].bottom = LONG(rects[i].y + rects[i].height);
        }

        handle->RSSetScissorRects(scissorCount, scissorRects);
    }

    void D3D11_CommandList::BindResource(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource)
    {
        if (resource != nullptr && resource->IsValid())
        {
            auto internal_state = to_internal(resource);
            ID3D11ShaderResourceView* SRV;

            if (subresource < 0)
            {
                SRV = internal_state->srv.Get();
            }
            else
            {
                assert(internal_state->subresources_srv.size() > static_cast<size_t>(subresource) && "Invalid subresource!");
                SRV = internal_state->subresources_srv[subresource].Get();
            }

            switch (stage)
            {
            case ShaderStage::Vertex:
                handle->VSSetShaderResources(slot, 1, &SRV);
                break;
            case ShaderStage::Hull:
                handle->HSSetShaderResources(slot, 1, &SRV);
                break;
            case ShaderStage::Domain:
                handle->DSSetShaderResources(slot, 1, &SRV);
                break;
            case ShaderStage::Geometry:
                handle->GSSetShaderResources(slot, 1, &SRV);
                break;
            case ShaderStage::Fragment:
                handle->PSSetShaderResources(slot, 1, &SRV);
                break;
            case ShaderStage::Compute:
                handle->CSSetShaderResources(slot, 1, &SRV);
                break;
            default:
                ALIMER_UNREACHABLE();
                break;
            }
        }
    }

    void D3D11_CommandList::BindResources(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count)
    {
        ALIMER_ASSERT(count <= 16); /* D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT */
        ID3D11ShaderResourceView* srvs[16];
        for (uint32_t i = 0; i < count; ++i)
        {
            srvs[i] = resources[i] != nullptr && resources[i]->IsValid() ? to_internal(resources[i])->srv.Get() : nullptr;
        }

        switch (stage)
        {
        case ShaderStage::Vertex:
            handle->VSSetShaderResources(slot, count, srvs);
            break;
        case ShaderStage::Hull:
            handle->HSSetShaderResources(slot, count, srvs);
            break;
        case ShaderStage::Domain:
            handle->DSSetShaderResources(slot, count, srvs);
            break;
        case ShaderStage::Geometry:
            handle->GSSetShaderResources(slot, count, srvs);
            break;
        case ShaderStage::Fragment:
            handle->PSSetShaderResources(slot, count, srvs);
            break;
        case ShaderStage::Compute:
            handle->CSSetShaderResources(slot, count, srvs);
            break;
        default:
            ALIMER_UNREACHABLE();
            break;
        }
    }

    void D3D11_CommandList::BindUAV(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource)
    {
        if (resource != nullptr && resource->IsValid())
        {
            auto internal_state = to_internal(resource);
            ID3D11UnorderedAccessView* UAV;

            if (subresource < 0)
            {
                UAV = internal_state->uav.Get();
            }
            else
            {
                assert(internal_state->subresources_uav.size() > static_cast<size_t>(subresource) && "Invalid subresource!");
                UAV = internal_state->subresources_uav[subresource].Get();
            }

            if (stage == ShaderStage::Compute)
            {
                handle->CSSetUnorderedAccessViews(slot, 1, &UAV, nullptr);
            }
            else
            {
                raster_uavs[slot] = UAV;
                raster_uavs_slot = Alimer::Min(raster_uavs_slot, uint8_t(slot));
                raster_uavs_count = Alimer::Max(raster_uavs_count, uint8_t(1));
            }
        }
    }

    void D3D11_CommandList::BindUAVs(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count)
    {
        assert(slot + count <= 8);
        ID3D11UnorderedAccessView* uavs[8];
        for (uint32_t i = 0; i < count; ++i)
        {
            uavs[i] = resources[i] != nullptr && resources[i]->IsValid() ? to_internal(resources[i])->uav.Get() : nullptr;

            if (stage != ShaderStage::Compute)
            {
                raster_uavs[slot + i] = uavs[i];
            }
        }

        if (stage == ShaderStage::Compute)
        {
            handle->CSSetUnorderedAccessViews(static_cast<uint32_t>(slot), static_cast<uint32_t>(count), uavs, nullptr);
        }
        else
        {
            raster_uavs_slot = Alimer::Min(raster_uavs_slot, uint8_t(slot));
            raster_uavs_count = Alimer::Max(raster_uavs_count, uint8_t(count));
        }
    }

    void D3D11_CommandList::BindSampler(ShaderStage stage, const Sampler* sampler, uint32_t slot)
    {
        if (sampler != nullptr)
        {
            auto internal_state = to_internal(sampler);
            ID3D11SamplerState* state = internal_state->handle;

            switch (stage)
            {
            case ShaderStage::Vertex:
                handle->VSSetSamplers(slot, 1, &state);
                break;
            case ShaderStage::Hull:
                handle->HSSetSamplers(slot, 1, &state);
                break;
            case ShaderStage::Domain:
                handle->DSSetSamplers(slot, 1, &state);
                break;
            case ShaderStage::Geometry:
                handle->GSSetSamplers(slot, 1, &state);
                break;
            case ShaderStage::Fragment:
                handle->PSSetSamplers(slot, 1, &state);
                break;
            case ShaderStage::Compute:
                handle->CSSetSamplers(slot, 1, &state);
                break;
            case ShaderStage::Mesh:
            case ShaderStage::Amplification:
                break;
            default:
                ALIMER_UNREACHABLE();
                break;
            }
        }
    }

    void D3D11_CommandList::BindConstantBuffer(ShaderStage stage, const GraphicsBuffer* buffer, uint32_t slot)
    {
        ALIMER_ASSERT(slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

        ID3D11Buffer* res = buffer != nullptr ? to_internal(buffer)->handle : nullptr;
        switch (stage)
        {
        case ShaderStage::Vertex:
            handle->VSSetConstantBuffers(slot, 1, &res);
            break;
        case ShaderStage::Hull:
            handle->HSSetConstantBuffers(slot, 1, &res);
            break;
        case ShaderStage::Domain:
            handle->DSSetConstantBuffers(slot, 1, &res);
            break;
        case ShaderStage::Geometry:
            handle->GSSetConstantBuffers(slot, 1, &res);
            break;
        case ShaderStage::Fragment:
            handle->PSSetConstantBuffers(slot, 1, &res);
            break;
        case ShaderStage::Compute:
            handle->CSSetConstantBuffers(slot, 1, &res);
            break;
        case ShaderStage::Mesh:
        case ShaderStage::Amplification:
            break;
        default:
            ALIMER_UNREACHABLE();
            break;
        }
    }

    void D3D11_CommandList::BindVertexBuffers(const GraphicsBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets)
    {
        assert(count <= 8);
        ID3D11Buffer* res[8] = { 0 };
        for (uint32_t i = 0; i < count; ++i)
        {
            res[i] = vertexBuffers[i] != nullptr ? to_internal(vertexBuffers[i])->handle : nullptr;
        }

        handle->IASetVertexBuffers(slot, count, res, strides, (offsets != nullptr ? offsets : reinterpret_cast<const uint32_t*>(__nullBlob)));
    }

    void D3D11_CommandList::BindIndexBuffer(const GraphicsBuffer* indexBuffer, IndexFormat format, uint32_t offset)
    {
        ID3D11Buffer* res = indexBuffer != nullptr ? to_internal(indexBuffer)->handle : nullptr;
        handle->IASetIndexBuffer(res, (format == IndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT), offset);
    }

    void D3D11_CommandList::BindStencilRef(uint32_t value)
    {
        stencilRef = value;
    }

    void D3D11_CommandList::BindBlendFactor(float r, float g, float b, float a)
    {
        blendFactor.x = r;
        blendFactor.y = g;
        blendFactor.z = b;
        blendFactor.w = a;
    }

    void D3D11_CommandList::SetRenderPipeline(RenderPipeline* pipeline)
    {
        if (active_pso == pipeline)
            return;

        active_pso = pipeline;
        dirty_pso = true;
    }

    void D3D11_CommandList::BindComputeShader(const Shader* shader)
    {
        //ALIMER_ASSERT(shader->stage == ShaderStage::Compute);

        ID3D11ComputeShader* newShader = shader == nullptr ? nullptr : static_cast<ComputeShader_DX11*>(shader->internal_state.get())->resource.Get();
        if (newShader != computeShader)
        {
            handle->CSSetShader(newShader, nullptr, 0);
            computeShader = newShader;
        }
    }

    void D3D11_CommandList::PrepareDraw()
    {
        if (!dirty_pso)
            return;

        const RenderPipeline* pipeline = active_pso;
        auto internal_state = to_internal(pipeline);
        const RenderPipelineDescriptor& desc = internal_state->desc;

        ID3D11VertexShader* vs = desc.vs == nullptr ? nullptr : static_cast<VertexShader_DX11*>(desc.vs->internal_state.get())->resource.Get();
        if (vs != prev_vs)
        {
            handle->VSSetShader(vs, nullptr, 0);
            prev_vs = vs;
        }
        ID3D11PixelShader* ps = desc.ps == nullptr ? nullptr : static_cast<PixelShader_DX11*>(desc.ps->internal_state.get())->resource.Get();
        if (ps != prev_ps)
        {
            handle->PSSetShader(ps, nullptr, 0);
            prev_ps = ps;
        }
        ID3D11HullShader* hs = desc.hs == nullptr ? nullptr : static_cast<HullShader_DX11*>(desc.hs->internal_state.get())->resource.Get();
        if (hs != prev_hs)
        {
            handle->HSSetShader(hs, nullptr, 0);
            prev_hs = hs;
        }
        ID3D11DomainShader* ds = desc.ds == nullptr ? nullptr : static_cast<DomainShader_DX11*>(desc.ds->internal_state.get())->resource.Get();
        if (ds != prev_ds)
        {
            handle->DSSetShader(ds, nullptr, 0);
            prev_ds = ds;
        }
        ID3D11GeometryShader* gs = desc.gs == nullptr ? nullptr : static_cast<GeometryShader_DX11*>(desc.gs->internal_state.get())->resource.Get();
        if (gs != prev_gs)
        {
            handle->GSSetShader(gs, nullptr, 0);
            prev_gs = gs;
        }

        if (internal_state->blendState != prev_bs
            || desc.sampleMask != prev_samplemask
            || blendFactor.x != prev_blendfactor.x
            || blendFactor.y != prev_blendfactor.y
            || blendFactor.z != prev_blendfactor.z
            || blendFactor.w != prev_blendfactor.w
            )
        {
            handle->OMSetBlendState(internal_state->blendState, &blendFactor.x, desc.sampleMask);
            prev_bs = internal_state->blendState;
            prev_blendfactor = blendFactor;
            prev_samplemask = desc.sampleMask;
        }

        if (internal_state->rasterizerState != prev_rs)
        {
            handle->RSSetState(internal_state->rasterizerState);
            prev_rs = internal_state->rasterizerState;
        }

        ID3D11DepthStencilState* dss = internal_state->depthStencilState;
        if (dss != prev_dss || stencilRef != prev_stencilRef)
        {
            handle->OMSetDepthStencilState(dss, stencilRef);
            prev_dss = dss;
            prev_stencilRef = stencilRef;
        }

        ID3D11InputLayout* il = internal_state->inputLayout.Get();
        if (il != prev_il)
        {
            handle->IASetInputLayout(il);
            prev_il = il;
        }

        if (prev_pt != internal_state->primitiveTopology)
        {
            handle->IASetPrimitiveTopology(internal_state->primitiveTopology);
            prev_pt = internal_state->primitiveTopology;
        }
    }

    void D3D11_CommandList::CommitAllocations()
    {
        // DX11 needs to unmap allocations before it can execute safely
        if (frame_allocator.dirty)
        {
            auto buffer_d3d11 = StaticCast<Buffer_DX11>(frame_allocator.buffer);
            handle->Unmap(buffer_d3d11->handle, 0);
            frame_allocator.dirty = false;
        }
    }

    void D3D11_CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        PrepareDraw();
        CommitAllocations();

        if (instanceCount <= 1)
        {
            handle->Draw(vertexCount, firstVertex);
        }
        else
        {
            handle->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
        }
    }

    void D3D11_CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
    {
        PrepareDraw();
        CommitAllocations();

        if (instanceCount <= 1)
        {
            handle->DrawIndexed(indexCount, firstIndex, baseVertex);
        }
        else
        {
            handle->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
        }
    }

    void D3D11_CommandList::DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
        PrepareDraw();
        CommitAllocations();

        auto internal_state = to_internal(args);
        handle->DrawInstancedIndirect(to_internal(args)->handle, args_offset);
    }

    void D3D11_CommandList::DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
        PrepareDraw();
        CommitAllocations();

        auto internal_state = to_internal(args);
        handle->DrawIndexedInstancedIndirect(to_internal(args)->handle, args_offset);
    }

    void D3D11_CommandList::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
    {
        CommitAllocations();
        handle->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }

    void D3D11_CommandList::DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
        CommitAllocations();
        handle->DispatchIndirect(to_internal(args)->handle, args_offset);
    }

    void D3D11_CommandList::CopyResource(const GPUResource* pDst, const GPUResource* pSrc)
    {
        assert(pDst != nullptr && pSrc != nullptr);
        handle->CopyResource(to_internal(pDst)->resource.Get(), to_internal(pSrc)->resource.Get());
    }

    GPUAllocation D3D11_CommandList::AllocateGPU(size_t dataSize)
    {
        GPUAllocation result;
        if (dataSize == 0)
        {
            return result;
        }

        GPUBufferDesc bufferDesc = frame_allocator.buffer->GetDesc();
        if (bufferDesc.ByteWidth <= dataSize)
        {
            // If allocation too large, grow the allocator:
            bufferDesc.ByteWidth = uint32_t((dataSize + 1) * 2);
            frame_allocator.buffer = device->CreateBuffer(bufferDesc, nullptr);
            assert(frame_allocator.buffer.IsNotNull());
            frame_allocator.buffer->SetName("frame_allocator");
            frame_allocator.byteOffset = 0;
        }

        auto buffer_d3d11 = StaticCast<Buffer_DX11>(frame_allocator.buffer);
        frame_allocator.dirty = true;

        size_t position = frame_allocator.byteOffset;
        bool wrap = position == 0 || position + dataSize > frame_allocator.buffer->GetDesc().ByteWidth || frame_allocator.residentFrame != device->GetFrameCount();
        position = wrap ? 0 : position;

        // Issue buffer rename (realloc) on wrap, otherwise just append data:
        D3D11_MAP mapping = wrap ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = handle->Map(buffer_d3d11->handle, 0, mapping, 0, &mappedResource);
        assert(SUCCEEDED(hr) && "GPUBuffer mapping failed!");

        frame_allocator.byteOffset = position + dataSize;
        frame_allocator.residentFrame = device->GetFrameCount();

        result.buffer = frame_allocator.buffer.Get();
        result.offset = (uint32_t)position;
        result.data = (void*)((size_t)mappedResource.pData + position);
        return result;
    }

    void D3D11_CommandList::UpdateBuffer(GraphicsBuffer* buffer, const void* data, uint64_t size)
    {
        const GPUBufferDesc& bufferDesc = buffer->GetDesc();

        assert(bufferDesc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
        assert((int)bufferDesc.ByteWidth >= size && "Data size is too big!");

        auto internal_state = static_cast<const Buffer_DX11*>(buffer);
        if (size == 0)
        {
            size = bufferDesc.ByteWidth;
        }
        else
        {
            size = Alimer::Min<uint64_t>(bufferDesc.ByteWidth, size);
        }

        if (bufferDesc.Usage == USAGE_DYNAMIC)
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            HRESULT hr = handle->Map(internal_state->handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            assert(SUCCEEDED(hr) && "GPUBuffer mapping failed!");
            memcpy(mappedResource.pData, data, size);
            handle->Unmap(internal_state->handle, 0);
        }
        else if (bufferDesc.BindFlags & BIND_CONSTANT_BUFFER)
        {
            handle->UpdateSubresource(internal_state->handle, 0, nullptr, data, 0, 0);
        }
        else
        {
            D3D11_BOX box = {};
            box.left = 0;
            box.right = static_cast<uint32_t>(size);
            box.top = 0;
            box.bottom = 1;
            box.front = 0;
            box.back = 1;
            handle->UpdateSubresource(internal_state->handle, 0, &box, data, 0, 0);
        }
    }

    void D3D11_CommandList::PushDebugGroup(const char* name)
    {
        auto wName = ToUtf16(name, strlen(name));
        userDefinedAnnotation->BeginEvent(wName.c_str());
    }

    void D3D11_CommandList::PopDebugGroup()
    {
        userDefinedAnnotation->EndEvent();
    }

    void D3D11_CommandList::InsertDebugMarker(const char* name)
    {
        auto wName = ToUtf16(name, strlen(name));
        userDefinedAnnotation->SetMarker(wName.c_str());
    }

    void D3D11_CommandList::QueryBegin(const GPUQuery* query)
    {
        auto internal_state = to_internal(query);
        handle->Begin(internal_state->resource.Get());
    }

    void D3D11_CommandList::QueryEnd(const GPUQuery* query)
    {
        auto internal_state = to_internal(query);
        handle->End(internal_state->resource.Get());
    }

    void D3D11_CommandList::Barrier(const GPUBarrier* barriers, uint32_t numBarriers)
    {

    }
}

#endif // defined(ALIMER_D3D11)
