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

#include "Graphics_D3D12.h"
#include "Core/Hash.h"
#include "Core/Log.h"
#include "Math/MathHelper.h"

#include "d3dx12.h"
#include "D3D12MemAlloc.h"

#include <pix.h>

#if !defined(ALIMER_DISABLE_SHADER_COMPILER)
#   include "dxcapi.h"
#   include "d3d12shader.h"
#endif

#include <array>
#include <sstream>
#include <algorithm>

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
    PFN_DXC_CREATE_INSTANCE DxcCreateInstance;
#endif

    namespace DX12_Internal
    {
        // Engine -> Native converters

        constexpr D3D12_FILTER_TYPE _ConvertFilterType(FilterMode filter)
        {
            switch (filter)
            {
            case FilterMode::Nearest:
                return D3D12_FILTER_TYPE_POINT;
            case FilterMode::Linear:
                return D3D12_FILTER_TYPE_LINEAR;
            default:
                ALIMER_UNREACHABLE();
                return (D3D12_FILTER_TYPE)-1;
            }
        }

        inline D3D12_FILTER _ConvertFilter(FilterMode minFilter, FilterMode magFilter, FilterMode mipFilter, bool isComparison, bool isAnisotropic)
        {
            D3D12_FILTER filter;
            D3D12_FILTER_REDUCTION_TYPE reduction = isComparison ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD;

            if (isAnisotropic)
            {
                filter = D3D12_ENCODE_ANISOTROPIC_FILTER(reduction);
            }
            else
            {
                D3D12_FILTER_TYPE dxMin = _ConvertFilterType(minFilter);
                D3D12_FILTER_TYPE dxMag = _ConvertFilterType(magFilter);
                D3D12_FILTER_TYPE dxMip = _ConvertFilterType(mipFilter);
                filter = D3D12_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, reduction);
            }

            return filter;
        }

        constexpr D3D12_TEXTURE_ADDRESS_MODE _ConvertAddressMode(SamplerAddressMode value)
        {
            switch (value)
            {
            case SamplerAddressMode::Mirror:
                return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

            case SamplerAddressMode::Clamp:
                return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case SamplerAddressMode::Border:
                return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                //case SamplerAddressMode::MirrorOnce:
                //    return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;

            case SamplerAddressMode::Wrap:
            default:
                return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            }
        }

        constexpr D3D12_COMPARISON_FUNC _ConvertComparisonFunc(CompareFunction value)
        {
            switch (value)
            {
            case CompareFunction::Never:
                return D3D12_COMPARISON_FUNC_NEVER;
                break;
            case CompareFunction::Less:
                return D3D12_COMPARISON_FUNC_LESS;
                break;
            case CompareFunction::Equal:
                return D3D12_COMPARISON_FUNC_EQUAL;
                break;
            case CompareFunction::LessEqual:
                return D3D12_COMPARISON_FUNC_LESS_EQUAL;
                break;
            case CompareFunction::Greater:
                return D3D12_COMPARISON_FUNC_GREATER;
                break;
            case CompareFunction::NotEqual:
                return D3D12_COMPARISON_FUNC_NOT_EQUAL;
                break;
            case CompareFunction::GreaterEqual:
                return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
                break;
            case CompareFunction::Always:
                return D3D12_COMPARISON_FUNC_ALWAYS;
                break;
            default:
                break;
            }
            return D3D12_COMPARISON_FUNC_NEVER;
        }
        constexpr D3D12_CULL_MODE _ConvertCullMode(CullMode value)
        {
            switch (value)
            {
            case CullMode::None:
                return D3D12_CULL_MODE_NONE;
            case CullMode::Front:
                return D3D12_CULL_MODE_FRONT;
            case CullMode::Back:
                return D3D12_CULL_MODE_BACK;
            default:
                ALIMER_UNREACHABLE();
                break;
            }
            return D3D12_CULL_MODE_NONE;
        }
        constexpr D3D12_STENCIL_OP _ConvertStencilOp(StencilOperation value)
        {
            switch (value)
            {
            case StencilOperation::Keep:
                return D3D12_STENCIL_OP_KEEP;
            case StencilOperation::Zero:
                return D3D12_STENCIL_OP_ZERO;
            case StencilOperation::Replace:
                return D3D12_STENCIL_OP_REPLACE;
            case StencilOperation::IncrementClamp:
                return D3D12_STENCIL_OP_INCR_SAT;
            case StencilOperation::DecrementClamp:
                return D3D12_STENCIL_OP_DECR_SAT;
            case StencilOperation::Invert:
                return D3D12_STENCIL_OP_INVERT;
            case StencilOperation::IncrementWrap:
                return D3D12_STENCIL_OP_INCR;
            case StencilOperation::DecrementWrap:
                return D3D12_STENCIL_OP_DECR;
            default:
                ALIMER_UNREACHABLE();
                break;
            }
            return D3D12_STENCIL_OP_KEEP;
        }
        constexpr D3D12_BLEND _ConvertBlend(BlendFactor value)
        {
            switch (value)
            {
            case BlendFactor::Zero:
                return D3D12_BLEND_ZERO;
            case BlendFactor::One:
                return D3D12_BLEND_ONE;
            case BlendFactor::SourceColor:
                return D3D12_BLEND_SRC_COLOR;
            case BlendFactor::OneMinusSourceColor:
                return D3D12_BLEND_INV_SRC_COLOR;
            case BlendFactor::SourceAlpha:
                return D3D12_BLEND_SRC_ALPHA;
            case BlendFactor::OneMinusSourceAlpha:
                return D3D12_BLEND_INV_SRC_ALPHA;
            case BlendFactor::DestinationColor:
                return D3D12_BLEND_DEST_COLOR;
            case BlendFactor::OneMinusDestinationColor:
                return D3D12_BLEND_INV_DEST_COLOR;
            case BlendFactor::DestinationAlpha:
                return D3D12_BLEND_DEST_ALPHA;
            case BlendFactor::OneMinusDestinationAlpha:
                return D3D12_BLEND_INV_DEST_ALPHA;
            case BlendFactor::SourceAlphaSaturated:
                return D3D12_BLEND_SRC_ALPHA_SAT;
            case BlendFactor::BlendColor:
                //case BlendFactor::BlendAlpha:
                return D3D12_BLEND_BLEND_FACTOR;
            case BlendFactor::OneMinusBlendColor:
                //case BlendFactor::OneMinusBlendAlpha:
                return D3D12_BLEND_INV_BLEND_FACTOR;
            case BlendFactor::Source1Color:
                return D3D12_BLEND_SRC1_COLOR;
            case BlendFactor::OneMinusSource1Color:
                return D3D12_BLEND_INV_SRC1_COLOR;
            case BlendFactor::Source1Alpha:
                return D3D12_BLEND_SRC1_ALPHA;
            case BlendFactor::OneMinusSource1Alpha:
                return D3D12_BLEND_INV_SRC1_ALPHA;
            default:
                ALIMER_UNREACHABLE();
                return D3D12_BLEND_ZERO;
            }
        }
        constexpr D3D12_BLEND_OP _ConvertBlendOp(BlendOperation value)
        {
            switch (value)
            {
            case BlendOperation::Add:
                return D3D12_BLEND_OP_ADD;
            case BlendOperation::Subtract:
                return D3D12_BLEND_OP_SUBTRACT;
            case BlendOperation::ReverseSubtract:
                return D3D12_BLEND_OP_REV_SUBTRACT;
            case BlendOperation::Min:
                return D3D12_BLEND_OP_MIN;
            case BlendOperation::Max:
                return D3D12_BLEND_OP_MAX;
            default:
                ALIMER_UNREACHABLE();
                return D3D12_BLEND_OP_ADD;
            }
        }
        constexpr uint8_t _ConvertColorWriteMask(ColorWriteMask writeMask)
        {
            static_assert(static_cast<D3D12_COLOR_WRITE_ENABLE>(ColorWriteMask::Red) == D3D12_COLOR_WRITE_ENABLE_RED, "ColorWriteMask values must match");
            static_assert(static_cast<D3D12_COLOR_WRITE_ENABLE>(ColorWriteMask::Green) == D3D12_COLOR_WRITE_ENABLE_GREEN, "ColorWriteMask values must match");
            static_assert(static_cast<D3D12_COLOR_WRITE_ENABLE>(ColorWriteMask::Blue) == D3D12_COLOR_WRITE_ENABLE_BLUE, "ColorWriteMask values must match");
            static_assert(static_cast<D3D12_COLOR_WRITE_ENABLE>(ColorWriteMask::Alpha) == D3D12_COLOR_WRITE_ENABLE_ALPHA, "ColorWriteMask values must match");
            return static_cast<uint8_t>(writeMask);
        }

        constexpr D3D12_INPUT_CLASSIFICATION _ConvertInputClassification(InputStepMode value)
        {
            switch (value)
            {
            case InputStepMode::Vertex:
                return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            case InputStepMode::Instance:
                return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            default:
                ALIMER_UNREACHABLE();
                return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            }
        }
        inline D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
        {
            D3D12_SUBRESOURCE_DATA data;
            data.pData = pInitialData.pSysMem;
            data.RowPitch = pInitialData.SysMemPitch;
            data.SlicePitch = pInitialData.SysMemSlicePitch;

            return data;
        }
        constexpr D3D12_RESOURCE_STATES _ConvertImageLayout(IMAGE_LAYOUT value)
        {
            switch (value)
            {
            case IMAGE_LAYOUT_UNDEFINED:
            case IMAGE_LAYOUT_GENERAL:
                return D3D12_RESOURCE_STATE_COMMON;
            case IMAGE_LAYOUT_RENDERTARGET:
                return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case IMAGE_LAYOUT_DEPTHSTENCIL:
                return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case IMAGE_LAYOUT_DEPTHSTENCIL_READONLY:
                return D3D12_RESOURCE_STATE_DEPTH_READ;
            case IMAGE_LAYOUT_SHADER_RESOURCE:
                return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            case IMAGE_LAYOUT_UNORDERED_ACCESS:
                return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            case IMAGE_LAYOUT_COPY_SRC:
                return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case IMAGE_LAYOUT_COPY_DST:
                return D3D12_RESOURCE_STATE_COPY_DEST;
            case IMAGE_LAYOUT_SHADING_RATE_SOURCE:
                return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
            }

            return D3D12_RESOURCE_STATE_COMMON;
        }
        constexpr D3D12_RESOURCE_STATES _ConvertBufferState(BUFFER_STATE value)
        {
            switch (value)
            {
            case BUFFER_STATE_GENERAL:
                return D3D12_RESOURCE_STATE_COMMON;
            case BUFFER_STATE_VERTEX_BUFFER:
                return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            case BUFFER_STATE_INDEX_BUFFER:
                return D3D12_RESOURCE_STATE_INDEX_BUFFER;
            case BUFFER_STATE_CONSTANT_BUFFER:
                return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            case BUFFER_STATE_INDIRECT_ARGUMENT:
                return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            case BUFFER_STATE_SHADER_RESOURCE:
                return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            case BUFFER_STATE_UNORDERED_ACCESS:
                return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            case BUFFER_STATE_COPY_SRC:
                return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case BUFFER_STATE_COPY_DST:
                return D3D12_RESOURCE_STATE_COPY_DEST;
            case BUFFER_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
                return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
            }

            return D3D12_RESOURCE_STATE_COMMON;
        }
        constexpr D3D12_SHADER_VISIBILITY _ConvertShaderVisibility(ShaderStage value)
        {
            switch (value)
            {
            case ShaderStage::Mesh:
                return D3D12_SHADER_VISIBILITY_MESH;
            case ShaderStage::Amplification:
                return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
            case ShaderStage::Vertex:
                return D3D12_SHADER_VISIBILITY_VERTEX;
            case ShaderStage::Hull:
                return D3D12_SHADER_VISIBILITY_HULL;
            case ShaderStage::Domain:
                return D3D12_SHADER_VISIBILITY_DOMAIN;
            case ShaderStage::Geometry:
                return D3D12_SHADER_VISIBILITY_GEOMETRY;
            case ShaderStage::Fragment:
                return D3D12_SHADER_VISIBILITY_PIXEL;
            default:
                return D3D12_SHADER_VISIBILITY_ALL;
            }
            return D3D12_SHADER_VISIBILITY_ALL;
        }

        constexpr D3D12_SHADING_RATE _ConvertShadingRate(ShadingRate value)
        {
            switch (value)
            {
            case ShadingRate::Rate_1X1:
                return D3D12_SHADING_RATE_1X1;
            case ShadingRate::Rate_1X2:
                return D3D12_SHADING_RATE_1X2;
            case ShadingRate::Rate_2X1:
                return D3D12_SHADING_RATE_2X1;
            case ShadingRate::Rate_2X2:
                return D3D12_SHADING_RATE_2X2;
            case ShadingRate::Rate_2X4:
                return D3D12_SHADING_RATE_2X4;
            case ShadingRate::Rate_4X2:
                return D3D12_SHADING_RATE_4X2;
            case ShadingRate::Rate_4X4:
                return D3D12_SHADING_RATE_4X4;
            default:
                return D3D12_SHADING_RATE_1X1;
            }
            return D3D12_SHADING_RATE_1X1;
        }

        // Native -> Engine converters
        constexpr TextureDesc _ConvertTextureDesc_Inv(const D3D12_RESOURCE_DESC& desc)
        {
            TextureDesc retVal;

            switch (desc.Dimension)
            {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                retVal.type = TextureDesc::TEXTURE_1D;
                retVal.ArraySize = desc.DepthOrArraySize;
                break;
            default:
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                retVal.type = TextureDesc::TEXTURE_2D;
                retVal.ArraySize = desc.DepthOrArraySize;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                retVal.type = TextureDesc::TEXTURE_3D;
                retVal.Depth = desc.DepthOrArraySize;
                break;
            }
            retVal.format = PixelFormatFromDXGIFormat(desc.Format);
            retVal.Width = (uint32_t)desc.Width;
            retVal.Height = desc.Height;
            retVal.MipLevels = desc.MipLevels;

            return retVal;
        }

        inline D3D12_DEPTH_STENCILOP_DESC _ConvertStencilOpDesc(const StencilStateFaceDescriptor descriptor) {
            D3D12_DEPTH_STENCILOP_DESC desc;
            desc.StencilFailOp = _ConvertStencilOp(descriptor.failOp);
            desc.StencilDepthFailOp = _ConvertStencilOp(descriptor.depthFailOp);
            desc.StencilPassOp = _ConvertStencilOp(descriptor.passOp);
            desc.StencilFunc = _ConvertComparisonFunc(descriptor.compare);
            return desc;
        }

        // Local Helpers:

        inline size_t Align(size_t uLocation, size_t uAlign)
        {
            if ((0 == uAlign) || (uAlign & (uAlign - 1)))
            {
                assert(0);
            }

            return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
        }

        struct Buffer_DX12 : public GraphicsBuffer
        {
            Buffer_DX12(const GPUBufferDesc& desc_)
                : GraphicsBuffer(desc_)
            {
            }

            ~Buffer_DX12() override
            {
                Destroy();
            }

            void Destroy() override
            {
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (allocation) allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
                if (resource) allocationhandler->destroyer_resources.push_back(std::make_pair(resource, framecount));
                allocationhandler->destroylocker.unlock();
            }

#ifdef _DEBUG
            void SetName(const String& newName) override
            {
                GraphicsBuffer::SetName(newName);

                auto wName = ToUtf16(newName);
                resource->SetName(wName.c_str());
            }
#endif

            std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
            D3D12MA::Allocation* allocation = nullptr;
            ComPtr<ID3D12Resource> resource;
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
            D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
            std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> subresources_srv;
            std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> subresources_uav;

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;

            GPUAllocation dynamic[kCommandListCount];
        };

        struct Resource_DX12
        {
            std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
            D3D12MA::Allocation* allocation = nullptr;
            ComPtr<ID3D12Resource> resource;
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
            D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
            std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> subresources_srv;
            std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> subresources_uav;

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;

            GPUAllocation dynamic[kCommandListCount];

            virtual ~Resource_DX12()
            {
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (allocation) allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
                if (resource) allocationhandler->destroyer_resources.push_back(std::make_pair(resource, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };
        struct Texture_DX12 : public Resource_DX12
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
            std::vector<D3D12_RENDER_TARGET_VIEW_DESC> subresources_rtv;
            std::vector<D3D12_DEPTH_STENCIL_VIEW_DESC> subresources_dsv;

            ~Texture_DX12() override
            {
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                allocationhandler->destroylocker.unlock();
            }
        };
        struct Sampler_DX12 : public Sampler
        {
            std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
            D3D12_SAMPLER_DESC descriptor;

            ~Sampler_DX12()
            {
                Destroy();
            }

            void Destroy() override
            {
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                allocationhandler->destroylocker.unlock();
            }
        };

        struct Query_DX12
        {
            std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
            GPU_QUERY_TYPE query_type = GPU_QUERY_TYPE_INVALID;
            uint32_t query_index = ~0;

            ~Query_DX12()
            {
                if (query_index != ~0)
                {
                    allocationhandler->destroylocker.lock();
                    uint64_t framecount = allocationhandler->framecount;
                    switch (query_type)
                    {
                    case GPU_QUERY_TYPE_OCCLUSION:
                    case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
                        allocationhandler->destroyer_queries_occlusion.push_back(std::make_pair(query_index, framecount));
                        break;
                    case GPU_QUERY_TYPE_TIMESTAMP:
                        allocationhandler->destroyer_queries_timestamp.push_back(std::make_pair(query_index, framecount));
                        break;
                    }
                    allocationhandler->destroylocker.unlock();
                }
            }
        };
        struct PipelineState_DX12 : public RenderPipeline
        {
            RenderPipelineDescriptor desc;
            std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
            ComPtr<ID3D12PipelineState> handle;
            ID3D12RootSignature* rootSignature = nullptr;
            D3D_PRIMITIVE_TOPOLOGY primitiveTopology;

            std::vector<D3D12_DESCRIPTOR_RANGE> resources;
            std::vector<D3D12_DESCRIPTOR_RANGE> samplers;

            ~PipelineState_DX12() override
            {
                Destroy();
            }

            void Destroy() override
            {
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (handle) {
                    allocationhandler->destroyer_pipelines.push_back(std::make_pair(handle, framecount));
                }

                if (rootSignature) {
                    allocationhandler->destroyer_rootSignatures.push_back(std::make_pair(rootSignature, framecount));
                }

                allocationhandler->destroylocker.unlock();
            }
        };

        struct BVH_DX12 : public Resource_DX12
        {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS desc = {};
            std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
            GraphicsBuffer* scratch;
        };
        struct RTPipelineState_DX12
        {
            std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
            ComPtr<ID3D12StateObject> resource;

            Vector<WString> export_strings;
            Vector<D3D12_EXPORT_DESC> exports;
            Vector<D3D12_DXIL_LIBRARY_DESC> library_descs;
            Vector<WString> group_strings;
            Vector<D3D12_HIT_GROUP_DESC> hitgroup_descs;

            ~RTPipelineState_DX12()
            {
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (resource) allocationhandler->destroyer_stateobjects.push_back(std::make_pair(resource, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };
        struct RenderPass_DX12
        {
            D3D12_RESOURCE_BARRIER barrierdescs_begin[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
            uint32_t num_barriers_begin = 0;
            D3D12_RESOURCE_BARRIER barrierdescs_end[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
            uint32_t num_barriers_end = 0;
        };
        struct DescriptorTable_DX12
        {
            std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;

            struct Heap
            {
                ComPtr<ID3D12DescriptorHeap> heap;
                D3D12_DESCRIPTOR_HEAP_DESC desc = {};
                D3D12_CPU_DESCRIPTOR_HANDLE address = {};
                std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
                std::vector<size_t> write_remap;
            };
            Heap sampler_heap;
            Heap resource_heap;
            std::vector<D3D12_STATIC_SAMPLER_DESC> staticsamplers;

            ~DescriptorTable_DX12()
            {
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (sampler_heap.heap) allocationhandler->destroyer_descriptorHeaps.push_back(std::make_pair(sampler_heap.heap, framecount));
                if (resource_heap.heap) allocationhandler->destroyer_descriptorHeaps.push_back(std::make_pair(resource_heap.heap, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };
        struct RootSignature_DX12
        {
            std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
            ID3D12RootSignature* resource = nullptr;
            std::vector<D3D12_ROOT_PARAMETER> params;

            std::vector<uint32_t> table_bind_point_remap;
            uint32_t root_constant_bind_remap = 0;

            struct RootRemap
            {
                uint32_t space = 0;
                uint32_t rangeIndex = 0;
            };
            std::vector<RootRemap> root_remap;

            ~RootSignature_DX12()
            {
                allocationhandler->destroylocker.lock();
                uint64_t framecount = allocationhandler->framecount;
                if (resource) allocationhandler->destroyer_rootSignatures.push_back(std::make_pair(resource, framecount));
                allocationhandler->destroylocker.unlock();
            }
        };



        /* New Interface */
        Buffer_DX12* to_internal(GraphicsBuffer* param)
        {
            return static_cast<Buffer_DX12*>(param);
        }

        const Buffer_DX12* to_internal(const GraphicsBuffer* param)
        {
            return static_cast<const Buffer_DX12*>(param);
        }

        const PipelineState_DX12* to_internal(const RenderPipeline* param)
        {
            return static_cast<const PipelineState_DX12*>(param);
        }

        Resource_DX12* to_internal(const GPUResource* param)
        {
            return static_cast<Resource_DX12*>(param->internal_state.get());
        }


        Texture_DX12* to_internal(const Texture* param)
        {
            return static_cast<Texture_DX12*>(param->internal_state.get());
        }
        const Sampler_DX12* to_internal(const Sampler* param)
        {
            return static_cast<const Sampler_DX12*>(param);
        }
        Query_DX12* to_internal(const GPUQuery* param)
        {
            return static_cast<Query_DX12*>(param->internal_state.get());
        }
        PipelineState_DX12* to_internal(const Shader* param)
        {
            return static_cast<PipelineState_DX12*>(param->internal_state.get());
        }
        BVH_DX12* to_internal(const RaytracingAccelerationStructure* param)
        {
            return static_cast<BVH_DX12*>(param->internal_state.get());
        }
        RTPipelineState_DX12* to_internal(const RaytracingPipelineState* param)
        {
            return static_cast<RTPipelineState_DX12*>(param->internal_state.get());
        }
        RenderPass_DX12* to_internal(const RenderPass* param)
        {
            return static_cast<RenderPass_DX12*>(param->internal_state.get());
        }
        DescriptorTable_DX12* to_internal(const DescriptorTable* param)
        {
            return static_cast<DescriptorTable_DX12*>(param->internal_state.get());
        }
        RootSignature_DX12* to_internal(const RootSignature* param)
        {
            return static_cast<RootSignature_DX12*>(param->internal_state.get());
        }

#if !defined(ALIMER_DISABLE_SHADER_COMPILER)
        ComPtr<IDxcLibrary> dxcLibrary;
        ComPtr<IDxcCompiler> dxcCompiler;

        inline IDxcLibrary* GetOrCreateDxcLibrary() {
            if (dxcLibrary == nullptr) {
                ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&dxcLibrary)));
                ALIMER_ASSERT(dxcLibrary != nullptr);
            }

            return dxcLibrary.Get();
        }

        inline IDxcCompiler* GetOrCreateDxcCompiler() {
            if (dxcCompiler == nullptr) {
                ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler)));
                ALIMER_ASSERT(dxcCompiler != nullptr);
            }

            return dxcCompiler.Get();
        }
#endif
    }

    using namespace DX12_Internal;

    class D3D12_CommandList final : public CommandList
    {
    public:
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
        void BindShadingRate(ShadingRate rate) override;
        void BindShadingRateImage(const Texture* texture) override;

        void SetRenderPipeline(RenderPipeline* pipeline) override;
        void BindComputeShader(const Shader* shader) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) override;
        void DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;
        void DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;

        void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;
        void DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;
        void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;
        void DispatchMeshIndirect(const GraphicsBuffer* args, uint32_t args_offset) override;

        GPUAllocation AllocateGPU(const uint32_t size) override;
        void UpdateBuffer(GraphicsBuffer* buffer, const void* data, uint64_t size = 0) override;
        void CopyResource(const GPUResource* pDst, const GPUResource* pSrc) override;

        void ResolveQueryData();
        void QueryBegin(const GPUQuery* query) override;
        void QueryEnd(const GPUQuery* query) override;
        void Barrier(const GPUBarrier* barriers, uint32_t numBarriers) override;
        void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, const RaytracingAccelerationStructure* src = nullptr) override;
        void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso) override;
        void DispatchRays(const DispatchRaysDesc* desc) override;

        void BindDescriptorTable(PipelineBindPoint bindPoint, uint32_t space, const DescriptorTable* table) override;
        void BindRootDescriptor(PipelineBindPoint bindPoint, uint32_t index, const GraphicsBuffer* buffer, uint32_t offset) override;
        void BindRootConstants(PipelineBindPoint bindPoint, uint32_t index, const void* srcData) override;

    private:
        void PrepareDraw();
        void PrepareDispatch();
        void PrepareRaytrace();

    public: // TODO: Make private when we fix
        GraphicsDevice_DX12* device;
        uint32_t index = 0;
        ID3D12GraphicsCommandList6* handle = nullptr;
        ID3D12CommandAllocator* commandAllocators[kMaxInflightFrames] = {};

        D3D12_VIEWPORT viewports[kMaxViewportAndScissorRects];
        D3D12_RECT scissorRects[kMaxViewportAndScissorRects];
        const RenderPass* active_renderpass = nullptr;
        D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS resolve_subresources[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

        D3D12_SHADING_RATE prev_shadingrate = {};

        D3D_PRIMITIVE_TOPOLOGY prev_pt = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        bool dirty_pso = false;
        RenderPipeline* active_pso = nullptr;
        const Shader* active_cs = nullptr;

    private:
        const RootSignature* active_rootsig_graphics = nullptr;
        const RootSignature* active_rootsig_compute = nullptr;
        const RaytracingPipelineState* active_rt = nullptr;

        struct Query_Resolve
        {
            GPU_QUERY_TYPE type;
            uint32_t index;
        };
        std::vector<Query_Resolve> query_resolves;
    };


    // Allocators:
    void GraphicsDevice_DX12::AllocationHandler::Update(uint64_t FRAMECOUNT, uint32_t BACKBUFFER_COUNT)
    {
        destroylocker.lock();
        framecount = FRAMECOUNT;
        while (!destroyer_allocations.empty())
        {
            if (destroyer_allocations.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
            {
                auto item = destroyer_allocations.front();
                destroyer_allocations.pop_front();
                item.first->Release();
            }
            else
            {
                break;
            }
        }
        while (!destroyer_resources.empty())
        {
            if (destroyer_resources.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
            {
                destroyer_resources.pop_front();
                // comptr auto delete
            }
            else
            {
                break;
            }
        }
        while (!destroyer_queries_occlusion.empty())
        {
            if (destroyer_queries_occlusion.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
            {
                auto item = destroyer_queries_occlusion.front();
                destroyer_queries_occlusion.pop_front();
                free_occlusionqueries.push_back(item.first);
            }
            else
            {
                break;
            }
        }
        while (!destroyer_queries_timestamp.empty())
        {
            if (destroyer_queries_timestamp.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
            {
                auto item = destroyer_queries_timestamp.front();
                destroyer_queries_timestamp.pop_front();
                free_timestampqueries.push_back(item.first);
            }
            else
            {
                break;
            }
        }
        while (!destroyer_pipelines.empty())
        {
            if (destroyer_pipelines.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
            {
                destroyer_pipelines.pop_front();
                // comptr auto delete
            }
            else
            {
                break;
            }
        }
        while (!destroyer_rootSignatures.empty())
        {
            if (destroyer_rootSignatures.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
            {
                auto item = destroyer_rootSignatures.front();
                destroyer_rootSignatures.pop_front();
                item.first->Release();
            }
            else
            {
                break;
            }
        }
        while (!destroyer_stateobjects.empty())
        {
            if (destroyer_stateobjects.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
            {
                destroyer_stateobjects.pop_front();
                // comptr auto delete
            }
            else
            {
                break;
            }
        }
        while (!destroyer_descriptorHeaps.empty())
        {
            if (destroyer_descriptorHeaps.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
            {
                destroyer_descriptorHeaps.pop_front();
                // comptr auto delete
            }
            else
            {
                break;
            }
        }
        destroylocker.unlock();
    }

    void GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::init(GraphicsDevice_DX12* device, size_t size)
    {
        this->device = device;

        GPUBufferDesc bufferDesc = {};
        bufferDesc.ByteWidth = (uint32_t)(size);
        bufferDesc.Usage = USAGE_DYNAMIC;
        bufferDesc.BindFlags = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
        bufferDesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

        Buffer_DX12* newBuffer = new Buffer_DX12(bufferDesc);
        newBuffer->allocationhandler = device->allocationhandler;

        HRESULT hr;

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        CD3DX12_RESOURCE_DESC resdesc = CD3DX12_RESOURCE_DESC::Buffer(size);

        hr = device->allocationhandler->allocator->CreateResource(
            &allocationDesc,
            &resdesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &newBuffer->allocation,
            IID_PPV_ARGS(&newBuffer->resource)
        );
        assert(SUCCEEDED(hr));

        void* pData;
        CD3DX12_RANGE readRange(0, 0);
        newBuffer->resource->Map(0, &readRange, &pData);
        dataCur = dataBegin = reinterpret_cast<uint8_t*>(pData);
        dataEnd = dataBegin + size;

        ALIMER_ASSERT(bufferDesc.ByteWidth == (uint32_t)((size_t)dataEnd - (size_t)dataBegin));
        newBuffer->srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        newBuffer->srv.Format = DXGI_FORMAT_R32_TYPELESS;
        newBuffer->srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        newBuffer->srv.Buffer.NumElements = bufferDesc.ByteWidth / sizeof(uint32_t);
        newBuffer->srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // Because the "buffer" is created by hand in this, fill the desc to indicate how it can be used:
        buffer.Reset(newBuffer);
    }

    uint8_t* GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::allocate(size_t dataSize, size_t alignment)
    {
        dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

        if (dataCur + dataSize > dataEnd)
        {
            init(device, ((size_t)dataEnd + dataSize - (size_t)dataBegin) * 2);
        }

        uint8_t* retVal = dataCur;

        dataCur += dataSize;

        return retVal;
    }

    void GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::clear()
    {
        dataCur = dataBegin;
    }

    uint64_t GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::calculateOffset(uint8_t* address)
    {
        assert(address >= dataBegin && address < dataEnd);
        return static_cast<uint64_t>(address - dataBegin);
    }

    void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::init(GraphicsDevice_DX12* device)
    {
        this->device = device;

        // Reset state to empty:
        reset();

        heaps_resource.resize(1);
        heaps_sampler.resize(1);
    }

    void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::shutdown()
    {
        for (auto& x : heaps_resource)
        {
            SafeRelease(x.heap_GPU);
        }
        for (auto& x : heaps_sampler)
        {
            SafeRelease(x.heap_GPU);
        }
    }

    void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::reset()
    {
        dirty = true;
        heaps_bound = false;
        for (auto& x : heaps_resource)
        {
            x.ringOffset = 0;
        }
        for (auto& x : heaps_sampler)
        {
            x.ringOffset = 0;
        }
        current_resource_heap = 0;
        current_sampler_heap = 0;

        memset(CBV, 0, sizeof(CBV));
        memset(SRV, 0, sizeof(SRV));
        memset(SRV_index, -1, sizeof(SRV_index));
        memset(UAV, 0, sizeof(UAV));
        memset(UAV_index, -1, sizeof(UAV_index));
        memset(SAM, 0, sizeof(SAM));
    }

    void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::request_heaps(uint32_t resources, uint32_t samplers, D3D12_CommandList* cmd)
    {
        // This function allocatesGPU visible descriptor heaps that can fit the requested table sizes.
        //	First, they grow the heaps until the size fits the dx12 resource limits (tier 1 resource limit = 1 million, sampler limit is 2048)
        //	When the limits are reached, and there is still a need to allocate, then completely new heap blocks are started
        //	
        //	The function will automatically bind descriptor heaps when there was a new (growing or block allocation)

        DescriptorHeap& heap_resource = heaps_resource[current_resource_heap];
        uint32_t allocation = heap_resource.ringOffset + resources;
        if (heap_resource.heapDesc.NumDescriptors <= allocation)
        {
            if (allocation > 1000000) // tier 1 limit
            {
                // need new block
                allocation -= heap_resource.ringOffset;
                current_resource_heap++;
                if (heaps_resource.size() <= current_resource_heap)
                {
                    heaps_resource.resize(current_resource_heap + 1);
                }
            }
            DescriptorHeap& heap = heaps_resource[current_resource_heap];

            // Need to re-check if growing is necessary (maybe step into new block is enough):
            if (heap.heapDesc.NumDescriptors <= allocation)
            {
                // grow rate is controlled here:
                allocation = Max(512u, allocation);
                allocation = NextPowerOfTwo(allocation);
                allocation = Min(1000000u, allocation);

                // Issue destruction of the old heap:
                device->allocationhandler->destroylocker.lock();
                uint64_t framecount = device->allocationhandler->framecount;
                device->allocationhandler->destroyer_descriptorHeaps.push_back(std::make_pair(heap.heap_GPU, framecount));
                device->allocationhandler->destroylocker.unlock();

                heap.heapDesc.NodeMask = 0;
                heap.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                heap.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                heap.heapDesc.NumDescriptors = allocation;
                HRESULT hr = device->device->CreateDescriptorHeap(&heap.heapDesc, IID_PPV_ARGS(&heap.heap_GPU));
                assert(SUCCEEDED(hr));

                // Save heap properties:
                heap.start_cpu = heap.heap_GPU->GetCPUDescriptorHandleForHeapStart();
                heap.start_gpu = heap.heap_GPU->GetGPUDescriptorHandleForHeapStart();
            }

            heaps_bound = false;
        }

        DescriptorHeap& heap_sampler = heaps_sampler[current_sampler_heap];
        allocation = heap_sampler.ringOffset + samplers;
        if (heap_sampler.heapDesc.NumDescriptors <= allocation)
        {
            if (allocation > 2048) // sampler limit
            {
                // need new block
                allocation -= heap_sampler.ringOffset;
                current_sampler_heap++;
                if (heaps_sampler.size() <= current_sampler_heap)
                {
                    heaps_sampler.resize(current_sampler_heap + 1);
                }
            }
            DescriptorHeap& heap = heaps_sampler[current_sampler_heap];

            // Need to re-check if growing is necessary (maybe step into new block is enough):
            if (heap.heapDesc.NumDescriptors <= allocation)
            {
                // grow rate is controlled here:
                allocation = Max(512u, allocation);
                allocation = NextPowerOfTwo(allocation);
                allocation = Min(2048u, allocation);

                // Issue destruction of the old heap:
                device->allocationhandler->destroylocker.lock();
                uint64_t framecount = device->allocationhandler->framecount;
                device->allocationhandler->destroyer_descriptorHeaps.push_back(std::make_pair(heap.heap_GPU, framecount));
                device->allocationhandler->destroylocker.unlock();

                heap.heapDesc.NodeMask = 0;
                heap.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
                heap.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                heap.heapDesc.NumDescriptors = allocation;
                HRESULT hr = device->device->CreateDescriptorHeap(&heap.heapDesc, IID_PPV_ARGS(&heap.heap_GPU));
                assert(SUCCEEDED(hr));

                // Save heap properties:
                heap.start_cpu = heap.heap_GPU->GetCPUDescriptorHandleForHeapStart();
                heap.start_gpu = heap.heap_GPU->GetGPUDescriptorHandleForHeapStart();
            }

            heaps_bound = false;
        }

        if (!heaps_bound)
        {
            // definitely re-index the heap blocks!
            ID3D12DescriptorHeap* heaps[] = {
                heaps_resource[current_resource_heap].heap_GPU,
                heaps_sampler[current_sampler_heap].heap_GPU
            };

            cmd->handle->SetDescriptorHeaps(_countof(heaps), heaps);
        }
    }

    void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::validate(bool graphics, D3D12_CommandList* cmd)
    {
        if (!dirty)
            return;
        dirty = true;

        auto pso_internal = graphics ? to_internal(cmd->active_pso) : to_internal(cmd->active_cs);

        request_heaps((uint32_t)pso_internal->resources.size(), (uint32_t)pso_internal->samplers.size(), cmd);

        UINT root_parameter_index = 0;

        {

            {

                // Resources:
                if (!pso_internal->resources.empty())
                {
                    DescriptorHeap& heap = heaps_resource[current_resource_heap];

                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    srv_desc.Format = DXGI_FORMAT_R32_UINT;
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

                    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                    uav_desc.Format = DXGI_FORMAT_R32_UINT;
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

                    uint32_t index = 0;
                    for (auto& x : pso_internal->resources)
                    {
                        D3D12_CPU_DESCRIPTOR_HANDLE dst = heap.start_cpu;
                        dst.ptr += (heap.ringOffset + index) * device->resource_descriptor_size;

                        switch (x.RangeType)
                        {
                        default:
                        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                        {
                            const GPUResource* resource = SRV[x.BaseShaderRegister];
                            const int subresource = SRV_index[x.BaseShaderRegister];
                            if (resource == nullptr || !resource->IsValid())
                            {
                                device->device->CreateShaderResourceView(nullptr, &srv_desc, dst);
                            }
                            else
                            {
                                auto internal_state = to_internal(resource);

                                if (resource->IsAccelerationStructure())
                                {
                                    device->device->CreateShaderResourceView(nullptr, &internal_state->srv, dst);
                                }
                                else
                                {
                                    if (subresource < 0)
                                    {
                                        device->device->CreateShaderResourceView(internal_state->resource.Get(), &internal_state->srv, dst);
                                    }
                                    else
                                    {
                                        device->device->CreateShaderResourceView(internal_state->resource.Get(), &internal_state->subresources_srv[subresource], dst);
                                    }
                                }
                            }
                        }
                        break;

                        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                        {
                            const GPUResource* resource = UAV[x.BaseShaderRegister];
                            const int subresource = UAV_index[x.BaseShaderRegister];
                            if (resource == nullptr || !resource->IsValid())
                            {
                                device->device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, dst);
                            }
                            else
                            {
                                auto internal_state = to_internal(resource);

                                D3D12_CPU_DESCRIPTOR_HANDLE src = {};
                                if (subresource < 0)
                                {
                                    device->device->CreateUnorderedAccessView(internal_state->resource.Get(), nullptr, &internal_state->uav, dst);
                                }
                                else
                                {
                                    device->device->CreateUnorderedAccessView(internal_state->resource.Get(), nullptr, &internal_state->subresources_uav[subresource], dst);
                                }

                                if (src.ptr != 0)
                                {
                                    device->device->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                                }

                            }
                        }
                        break;

                        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                        {
                            const GraphicsBuffer* buffer = CBV[x.BaseShaderRegister];
                            if (buffer == nullptr)
                            {
                                device->device->CreateConstantBufferView(&cbv_desc, dst);
                            }
                            else
                            {
                                auto internal_state = to_internal(buffer);

                                if (buffer->GetDesc().Usage == USAGE_DYNAMIC)
                                {
                                    GPUAllocation allocation = internal_state->dynamic[cmd->index];
                                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
                                    cbv.BufferLocation = to_internal(allocation.buffer)->resource->GetGPUVirtualAddress();
                                    cbv.BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)allocation.offset;
                                    cbv.SizeInBytes = (uint32_t)Align((size_t)buffer->GetDesc().ByteWidth, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

                                    device->device->CreateConstantBufferView(&cbv, dst);
                                }
                                else
                                {
                                    device->device->CreateConstantBufferView(&internal_state->cbv, dst);
                                }
                            }
                        }
                        break;
                        }

                        index++;
                    }

                    D3D12_GPU_DESCRIPTOR_HANDLE binding_table = heap.start_gpu;
                    binding_table.ptr += (UINT64)heap.ringOffset * (UINT64)device->resource_descriptor_size;

                    if (graphics)
                    {
                        cmd->handle->SetGraphicsRootDescriptorTable(root_parameter_index, binding_table);
                    }
                    else
                    {
                        cmd->handle->SetComputeRootDescriptorTable(root_parameter_index, binding_table);
                    }

                    heap.ringOffset += (uint32_t)pso_internal->resources.size();
                    root_parameter_index++;
                }

                // Samplers:
                if (!pso_internal->samplers.empty())
                {
                    DescriptorHeap& heap = heaps_sampler[current_sampler_heap];

                    D3D12_SAMPLER_DESC sampler_desc = {};
                    sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                    sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                    sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                    sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
                    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

                    size_t index = 0;
                    for (auto& x : pso_internal->samplers)
                    {
                        D3D12_CPU_DESCRIPTOR_HANDLE dst = heap.start_cpu;
                        dst.ptr += (heap.ringOffset + index) * device->sampler_descriptor_size;

                        const Sampler* sampler = SAM[x.BaseShaderRegister];
                        if (sampler == nullptr)
                        {
                            device->device->CreateSampler(&sampler_desc, dst);
                        }
                        else
                        {
                            auto internal_state = to_internal(sampler);
                            device->device->CreateSampler(&internal_state->descriptor, dst);
                        }

                        index++;
                    }

                    D3D12_GPU_DESCRIPTOR_HANDLE binding_table = heap.start_gpu;
                    binding_table.ptr += (UINT64)heap.ringOffset * (UINT64)device->sampler_descriptor_size;

                    if (graphics)
                    {
                        cmd->handle->SetGraphicsRootDescriptorTable(root_parameter_index, binding_table);
                    }
                    else
                    {
                        cmd->handle->SetComputeRootDescriptorTable(root_parameter_index, binding_table);
                    }

                    heap.ringOffset += (uint32_t)pso_internal->samplers.size();
                    root_parameter_index++;
                }

            }

        }
    }
    GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::DescriptorHandles
        GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::commit(const DescriptorTable* table, D3D12_CommandList* cmd)
    {
        auto internal_state = to_internal(table);

        request_heaps(internal_state->resource_heap.desc.NumDescriptors, internal_state->sampler_heap.desc.NumDescriptors, cmd);

        DescriptorHandles handles;

        if (!internal_state->sampler_heap.ranges.empty())
        {
            DescriptorHeap& heap = heaps_sampler[current_sampler_heap];
            D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = heap.start_cpu;
            D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = heap.start_gpu;
            cpu_handle.ptr += heap.ringOffset * device->sampler_descriptor_size;
            gpu_handle.ptr += heap.ringOffset * device->sampler_descriptor_size;
            heap.ringOffset += internal_state->sampler_heap.desc.NumDescriptors;
            device->device->CopyDescriptorsSimple(
                internal_state->sampler_heap.desc.NumDescriptors,
                cpu_handle,
                internal_state->sampler_heap.address,
                internal_state->sampler_heap.desc.Type
            );
            handles.sampler_handle = gpu_handle;
        }

        if (!internal_state->resource_heap.ranges.empty())
        {
            DescriptorHeap& heap = heaps_resource[current_resource_heap];
            D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = heap.start_cpu;
            D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = heap.start_gpu;
            cpu_handle.ptr += heap.ringOffset * device->resource_descriptor_size;
            gpu_handle.ptr += heap.ringOffset * device->resource_descriptor_size;
            heap.ringOffset += internal_state->resource_heap.desc.NumDescriptors;
            device->device->CopyDescriptorsSimple(
                internal_state->resource_heap.desc.NumDescriptors,
                cpu_handle,
                internal_state->resource_heap.address,
                internal_state->resource_heap.desc.Type
            );
            handles.resource_handle = gpu_handle;
        }

        return handles;
    }


    bool GraphicsDevice_DX12::IsAvailable()
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
        CreateDXGIFactory2 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(dxgiDLL, "CreateDXGIFactory2"));
        if (!CreateDXGIFactory2)
        {
            return false;
        }

        DXGIGetDebugInterface1 = reinterpret_cast<PFN_DXGI_GET_DEBUG_INTERFACE1>(GetProcAddress(dxgiDLL, "DXGIGetDebugInterface1"));

        static HMODULE d3d12DLL = LoadLibraryA("d3d12.dll");
        if (!d3d12DLL) {
            return false;
        }

        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12DLL, "D3D12GetDebugInterface");
        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12DLL, "D3D12CreateDevice");
        if (!D3D12CreateDevice) {
            return false;
        }

        D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12DLL, "D3D12SerializeRootSignature");
        D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12DLL, "D3D12CreateRootSignatureDeserializer");
        D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(d3d12DLL, "D3D12SerializeVersionedRootSignature");
        D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12DLL, "D3D12CreateVersionedRootSignatureDeserializer");

        static HMODULE dxcompilerDLL = LoadLibraryA("dxcompiler.dll");
        if (!dxcompilerDLL) {
            return false;
        }

        DxcCreateInstance = (PFN_DXC_CREATE_INSTANCE)GetProcAddress(dxcompilerDLL, "DxcCreateInstance");
#endif

        if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            available = true;
            return true;
        }


        return false;
    }

    GraphicsDevice_DX12::GraphicsDevice_DX12(WindowHandle window, const GraphicsSettings& desc, D3D_FEATURE_LEVEL minFeatureLevel)
        : Graphics(window, desc)
        , minFeatureLevel{ minFeatureLevel }
    {
        if (!IsAvailable()) {
            // TODO: MessageBox
        }

        DESCRIPTOR_MANAGEMENT = true;
        SHADER_IDENTIFIER_SIZE = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);


        HRESULT hr = E_FAIL;

#if defined(_DEBUG)
        if (any(desc.flags & GraphicsDeviceFlags::DebugRuntime))
        {
            ComPtr<ID3D12Debug> d3d12Debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
            {
                d3d12Debug->EnableDebugLayer();

                ComPtr<ID3D12Debug1> d3d12Debug1;
                if (SUCCEEDED(d3d12Debug.As(&d3d12Debug1)))
                {
                    const bool GPUBasedValidation = any(desc.flags & GraphicsDeviceFlags::GPUBasedValidation);

                    d3d12Debug1->SetEnableGPUBasedValidation(GPUBasedValidation);
                }
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

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
#endif

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory4.ReleaseAndGetAddressOf())));

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            ComPtr<IDXGIFactory5> dxgiFactory5;
            HRESULT hr = dxgiFactory4.As(&dxgiFactory5);
            if (SUCCEEDED(hr))
            {
                hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                isTearingSupported = true;
            }
        }


        // Get adapter and create device
        {
            ComPtr<IDXGIAdapter1> adapter;
            GetAdapter(adapter.GetAddressOf());

            hr = D3D12CreateDevice(adapter.Get(), minFeatureLevel, IID_PPV_ARGS(&device));
            if (FAILED(hr))
            {
                std::stringstream ss("");
                ss << "Failed to create the graphics device! ERROR: " << std::hex << hr;
                //wiHelper::messageBox(ss.str(), "Error!");
                assert(0);
                //wiPlatform::Exit();
            }

            D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
            allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            allocatorDesc.pDevice = device;
            allocatorDesc.pAdapter = adapter.Get();

            allocationhandler = std::make_shared<AllocationHandler>();
            allocationhandler->device = device;

            hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocationhandler->allocator);
            assert(SUCCEEDED(hr));
        }

        // Create command queue
        D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
        directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        directQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        directQueueDesc.NodeMask = 0;
        ThrowIfFailed(device->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&directQueue)));
        directQueue->SetName(L"Graphics Command Queue");

        // Create fences for command queue:
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence));
        assert(SUCCEEDED(hr));
        frameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

        // Create swapchain
        {
            IDXGISwapChain1* tempChain;

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = backbufferWidth;
            swapChainDesc.Height = backbufferHeight;
            swapChainDesc.Format = PixelFormatToDXGIFormat(GetBackBufferFormat());
            swapChainDesc.Stereo = FALSE;
            swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = BACKBUFFER_COUNT;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            // It is recommended to always allow tearing if tearing support is available.
            swapChainDesc.Flags = (isTearingSupported) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = !desc.fullscreen;

            hr = dxgiFactory4->CreateSwapChainForHwnd(directQueue, window, &swapChainDesc, &fsSwapChainDesc, nullptr, &tempChain);

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            hr = dxgiFactory4->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
#else
            sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
            sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

            hr = dxgiFactory4->CreateSwapChainForCoreWindow(directQueue, reinterpret_cast<IUnknown*>(window.Get()), &sd, nullptr, &_swtempChainapChain);
#endif

            if (FAILED(hr))
            {
                //wiHelper::messageBox("Failed to create a swapchain for the graphics device!", "Error!");
                assert(0);
                //wiPlatform::Exit();
            }

            ThrowIfFailed(tempChain->QueryInterface(&swapChain));
            tempChain->Release();
        }


        // Create common descriptor heaps
        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NodeMask = 0;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            heapDesc.NumDescriptors = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT * kCommandListCount;
            HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorheap_RTV));
            assert(SUCCEEDED(hr));
            rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            rtv_descriptor_heap_start = descriptorheap_RTV->GetCPUDescriptorHandleForHeapStart();
        }
        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NodeMask = 0;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            heapDesc.NumDescriptors = kCommandListCount;
            HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorheap_DSV));
            assert(SUCCEEDED(hr));
            dsv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            dsv_descriptor_heap_start = descriptorheap_DSV->GetCPUDescriptorHandleForHeapStart();
        }

        resource_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        sampler_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        // Create frame-resident resources:
        for (uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
        {
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));

            // Create copy queue:
            {
                D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
                copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
                copyQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
                copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                copyQueueDesc.NodeMask = 0;

                ThrowIfFailed(device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&frames[i].copyQueue)));
                ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&frames[i].copyAllocator)));
                ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, frames[i].copyAllocator, nullptr, IID_PPV_ARGS(&frames[i].copyCommandList)));
                ThrowIfFailed(frames[i].copyCommandList->Close());
            }
        }

        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence)));

        // Query features:

        TESSELLATION = true;

        hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &features_0, sizeof(features_0));
        CONSERVATIVE_RASTERIZATION = features_0.ConservativeRasterizationTier >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1;
        RASTERIZER_ORDERED_VIEWS = features_0.ROVsSupported == TRUE;
        RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = features_0.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation == TRUE;

        if (features_0.TypedUAVLoadAdditionalFormats)
        {
            // More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
            UAV_LOAD_FORMAT_COMMON = true;

            D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport = { DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
            hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
            if (SUCCEEDED(hr) && (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
            {
                UAV_LOAD_FORMAT_R11G11B10_FLOAT = true;
            }
        }

        hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features_5, sizeof(features_5));
        RAYTRACING = features_5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
        RAYTRACING_INLINE = features_5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;

        hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &features_6, sizeof(features_6));
        VARIABLE_RATE_SHADING = features_6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1;
        VARIABLE_RATE_SHADING_TIER2 = features_6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
        VARIABLE_RATE_SHADING_TILE_SIZE = features_6.ShadingRateImageTileSize;

        hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features_7, sizeof(features_7));
        MESH_SHADER = features_7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;

        // Create common indirect command signatures:
        {
            D3D12_COMMAND_SIGNATURE_DESC cmd_desc = {};

            D3D12_INDIRECT_ARGUMENT_DESC dispatchArgs[1];
            dispatchArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

            D3D12_INDIRECT_ARGUMENT_DESC drawInstancedArgs[1];
            drawInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

            D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedArgs[1];
            drawIndexedInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

            cmd_desc.ByteStride = sizeof(IndirectDispatchArgs);
            cmd_desc.NumArgumentDescs = 1;
            cmd_desc.pArgumentDescs = dispatchArgs;
            hr = device->CreateCommandSignature(&cmd_desc, nullptr, IID_PPV_ARGS(&dispatchIndirectCommandSignature));
            assert(SUCCEEDED(hr));

            cmd_desc.ByteStride = sizeof(IndirectDrawArgsInstanced);
            cmd_desc.NumArgumentDescs = 1;
            cmd_desc.pArgumentDescs = drawInstancedArgs;
            hr = device->CreateCommandSignature(&cmd_desc, nullptr, IID_PPV_ARGS(&drawInstancedIndirectCommandSignature));
            assert(SUCCEEDED(hr));

            cmd_desc.ByteStride = sizeof(IndirectDrawArgsIndexedInstanced);
            cmd_desc.NumArgumentDescs = 1;
            cmd_desc.pArgumentDescs = drawIndexedInstancedArgs;
            hr = device->CreateCommandSignature(&cmd_desc, nullptr, IID_PPV_ARGS(&drawIndexedInstancedIndirectCommandSignature));
            assert(SUCCEEDED(hr));

            if (MESH_SHADER)
            {
                D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshArgs[1];
                dispatchMeshArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

                cmd_desc.ByteStride = sizeof(IndirectDispatchArgs);
                cmd_desc.NumArgumentDescs = 1;
                cmd_desc.pArgumentDescs = dispatchMeshArgs;
                hr = device->CreateCommandSignature(&cmd_desc, nullptr, IID_PPV_ARGS(&dispatchMeshIndirectCommandSignature));
                assert(SUCCEEDED(hr));
            }
        }

        // GPU Queries:
        {
            D3D12_QUERY_HEAP_DESC queryheapdesc = {};
            queryheapdesc.NodeMask = 0;

            for (uint32_t i = 0; i < timestamp_query_count; ++i)
            {
                allocationhandler->free_timestampqueries.push_back(i);
            }
            queryheapdesc.Count = timestamp_query_count;
            queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            hr = device->CreateQueryHeap(&queryheapdesc, IID_PPV_ARGS(&querypool_timestamp));
            assert(SUCCEEDED(hr));

            for (uint32_t i = 0; i < occlusion_query_count; ++i)
            {
                allocationhandler->free_occlusionqueries.push_back(i);
            }
            queryheapdesc.Count = occlusion_query_count;
            queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
            hr = device->CreateQueryHeap(&queryheapdesc, IID_PPV_ARGS(&querypool_occlusion));
            assert(SUCCEEDED(hr));


            D3D12MA::ALLOCATION_DESC allocationDesc = {};
            allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;

            CD3DX12_RESOURCE_DESC resdesc = CD3DX12_RESOURCE_DESC::Buffer(timestamp_query_count * sizeof(uint64_t));

            hr = allocationhandler->allocator->CreateResource(
                &allocationDesc,
                &resdesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                &allocation_querypool_timestamp_readback,
                IID_PPV_ARGS(&querypool_timestamp_readback)
            );
            assert(SUCCEEDED(hr));

            resdesc = CD3DX12_RESOURCE_DESC::Buffer(occlusion_query_count * sizeof(uint64_t));

            hr = allocationhandler->allocator->CreateResource(
                &allocationDesc,
                &resdesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                &allocation_querypool_occlusion_readback,
                IID_PPV_ARGS(&querypool_occlusion_readback)
            );
            assert(SUCCEEDED(hr));
        }

        LOGI("Direct3D12 Graphics Device created");
    }

    GraphicsDevice_DX12::~GraphicsDevice_DX12()
    {
        WaitForGPU();

        // SwapChain
        {
            for (uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
            {
                SafeRelease(backBuffers[i]);
            }

            SafeRelease(swapChain);
        }

        // Frame fence
        {
            SafeRelease(frameFence);
            CloseHandle(frameFenceEvent);
            SafeRelease(copyFence);
        }

        // Command signatures
        {
            SafeRelease(dispatchIndirectCommandSignature);
            SafeRelease(drawInstancedIndirectCommandSignature);
            SafeRelease(drawIndexedInstancedIndirectCommandSignature);
            SafeRelease(dispatchMeshIndirectCommandSignature);
        }

        for (uint32_t i = 0; i < kCommandListCount; i++)
        {
            if (!commandLists[i])
                break;

            for (uint32_t frameIndex = 0; frameIndex < BACKBUFFER_COUNT; ++frameIndex)
            {
                SafeRelease(commandLists[i]->commandAllocators[frameIndex]);
                frames[frameIndex].descriptors[i].shutdown();
                frames[frameIndex].resourceBuffer[i].buffer.Reset();
            }

            SafeRelease(commandLists[i]->handle);
            delete commandLists[i];
        }

        // Frame data
        {
            for (uint32_t frameIndex = 0; frameIndex < BACKBUFFER_COUNT; ++frameIndex)
            {
                SafeRelease(frames[frameIndex].copyCommandList);
                SafeRelease(frames[frameIndex].copyAllocator);
                SafeRelease(frames[frameIndex].copyQueue);
            }
        }

        SafeRelease(directQueue);

        // Descriptor Heaps
        {
            SafeRelease(descriptorheap_RTV);
            SafeRelease(descriptorheap_DSV);
        }

        {
            SafeRelease(querypool_timestamp);
            SafeRelease(querypool_occlusion);
            SafeRelease(querypool_timestamp_readback);
            SafeRelease(querypool_occlusion_readback);
            SafeRelease(allocation_querypool_timestamp_readback);
            SafeRelease(allocation_querypool_occlusion_readback);
        }

        allocationhandler->Update(~0, 0); // destroy all remaining
        if (allocationhandler->allocator) {
            allocationhandler->allocator->Release();
        }

#ifdef _DEBUG
        ULONG refCount = device->Release();
        if (refCount)
        {
            LOGD("Direct3D12: There are {} unreleased references left on the D3D device!", refCount);

            ID3D12DebugDevice* d3d12DebugDevice = nullptr;
            if (SUCCEEDED(device->QueryInterface(&d3d12DebugDevice)))
            {
                d3d12DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
                d3d12DebugDevice->Release();
            }
        }
        else
        {
            LOGD("Direct3D12: No memory leaks detected");
        }
#else
        device->Release();
#endif

        device = nullptr;

        // DXGI Factory
        {
            dxgiFactory4.Reset();

#ifdef _DEBUG
            ComPtr<IDXGIDebug1> dxgiDebug1;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
            {
                dxgiDebug1->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            }
#endif
        }
    }

    void GraphicsDevice_DX12::GetAdapter(IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6 = nullptr;
        HRESULT hr = dxgiFactory4->QueryInterface(&dxgiFactory6);
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

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                    OutputDebugStringW(buff);
#endif
                    break;
                }
            }
        }

        SafeRelease(dxgiFactory6);
#endif

        if (!adapter)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory4->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf())); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                    OutputDebugStringW(buff);
#endif
                    break;
                }
            }
        }

        if (!adapter)
        {
            LOGE("No Direct3D 12 device found");
        }

        *ppAdapter = adapter.Detach();
    }

    void GraphicsDevice_DX12::Resize(uint32_t width, uint32_t height)
    {
        if ((width != backbufferWidth
            || height != backbufferHeight) && width > 0 && height > 0)
        {
            backbufferWidth = width;
            backbufferHeight = height;

            for (uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
            {
                SafeRelease(backBuffers[i]);
            }

            HRESULT hr = swapChain->ResizeBuffers(GetBackBufferCount(), width, height, PixelFormatToDXGIFormat(GetBackBufferFormat()), 0);
            assert(SUCCEEDED(hr));

            for (uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
            {
                uint32_t fr = (GetFrameCount() + i) % BACKBUFFER_COUNT;
                hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[fr]));
                assert(SUCCEEDED(hr));
            }
        }
    }

    Texture GraphicsDevice_DX12::GetBackBuffer()
    {
        auto internal_state = std::make_shared<Texture_DX12>();
        internal_state->allocationhandler = allocationhandler;
        internal_state->resource = backBuffers[backbufferIndex];
        internal_state->rtv = {};
        internal_state->rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        D3D12_RESOURCE_DESC desc = internal_state->resource->GetDesc();
        device->GetCopyableFootprints(&desc, 0, 1, 0, &internal_state->footprint, nullptr, nullptr, nullptr);

        Texture result;
        result.type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;
        result.internal_state = internal_state;
        result.desc = _ConvertTextureDesc_Inv(desc);
        return result;
    }

    RefPtr<GraphicsBuffer> GraphicsDevice_DX12::CreateBuffer(const GPUBufferDesc& desc, const void* initialData)
    {
        RefPtr<Buffer_DX12> result(new Buffer_DX12(desc));
        result->allocationhandler = allocationhandler;

        if (desc.Usage == USAGE_DYNAMIC && desc.BindFlags & BIND_CONSTANT_BUFFER)
        {
            // this special case will use frame allocator
            return result;
        }

        HRESULT hr = E_FAIL;

        uint32_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        if (desc.BindFlags & BIND_CONSTANT_BUFFER)
        {
            alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        }
        size_t alignedSize = Align(desc.ByteWidth, alignment);

        D3D12_RESOURCE_DESC d3d12Desc = {};
        d3d12Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        d3d12Desc.Format = DXGI_FORMAT_UNKNOWN;
        d3d12Desc.Width = (UINT64)alignedSize;
        d3d12Desc.Height = 1;
        d3d12Desc.MipLevels = 1;
        d3d12Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        d3d12Desc.DepthOrArraySize = 1;
        d3d12Desc.Alignment = 0;
        d3d12Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (desc.BindFlags & BIND_UNORDERED_ACCESS)
        {
            d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        d3d12Desc.SampleDesc.Count = 1;
        d3d12Desc.SampleDesc.Quality = 0;

        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        if (desc.Usage == USAGE_STAGING)
        {
            if (desc.CPUAccessFlags & CPU_ACCESS_READ)
            {
                allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
                resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
            }
            else
            {
                allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
                resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
            }
        }

        device->GetCopyableFootprints(&d3d12Desc, 0, 1, 0, &result->footprint, nullptr, nullptr, nullptr);

        hr = allocationhandler->allocator->CreateResource(
            &allocationDesc,
            &d3d12Desc,
            resourceState,
            nullptr,
            &result->allocation,
            IID_PPV_ARGS(&result->resource)
        );

        if (FAILED(hr))
        {
            LOGE("D3D12: Create buffer failed");
            return nullptr;
        }

        // Issue data copy on request:
        if (initialData != nullptr)
        {
            GPUBufferDesc uploadBufferDesc = {};;
            uploadBufferDesc.ByteWidth = desc.ByteWidth;
            uploadBufferDesc.Usage = USAGE_STAGING;

            RefPtr<GraphicsBuffer> uploadbuffer = CreateBuffer(uploadBufferDesc, nullptr);
            ALIMER_ASSERT(uploadbuffer.IsNotNull());
            ID3D12Resource* upload_resource = to_internal(uploadbuffer.Get())->resource.Get();

            void* pData;
            CD3DX12_RANGE readRange(0, 0);
            hr = upload_resource->Map(0, &readRange, &pData);
            assert(SUCCEEDED(hr));
            memcpy(pData, initialData, desc.ByteWidth);

            copyQueueLock.lock();
            {
                auto& frame = GetFrameResources();
                if (!copyQueueUse)
                {
                    copyQueueUse = true;

                    HRESULT hr = frame.copyAllocator->Reset();
                    assert(SUCCEEDED(hr));
                    hr = frame.copyCommandList->Reset(frame.copyAllocator, nullptr);
                    assert(SUCCEEDED(hr));
                }
                frame.copyCommandList->CopyBufferRegion(result->resource.Get(), 0, upload_resource, 0, desc.ByteWidth);
            }
            copyQueueLock.unlock();
        }


        // Create resource views if needed
        if (desc.BindFlags & BIND_CONSTANT_BUFFER)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
            cbv_desc.SizeInBytes = (uint32_t)alignedSize;
            cbv_desc.BufferLocation = result->resource->GetGPUVirtualAddress();

            result->cbv = cbv_desc;
        }

        if (desc.BindFlags & BIND_SHADER_RESOURCE)
        {
            CreateSubresource(result.Get(), SRV, 0);
        }

        if (desc.BindFlags & BIND_UNORDERED_ACCESS)
        {
            CreateSubresource(result.Get(), UAV, 0);
        }

        return result;
    }

    bool GraphicsDevice_DX12::CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture)
    {
        auto internal_state = std::make_shared<Texture_DX12>();
        internal_state->allocationhandler = allocationhandler;
        pTexture->internal_state = internal_state;
        pTexture->type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;

        pTexture->desc = *pDesc;


        HRESULT hr = E_FAIL;

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc;
        desc.Format = PixelFormatToDXGIFormat(pDesc->format);
        desc.Width = pDesc->Width;
        desc.Height = pDesc->Height;
        desc.MipLevels = pDesc->MipLevels;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.DepthOrArraySize = (UINT16)pDesc->ArraySize;
        desc.SampleDesc.Count = pDesc->SampleCount;
        desc.SampleDesc.Quality = 0;
        desc.Alignment = 0;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (pDesc->BindFlags & BIND_DEPTH_STENCIL)
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
            if (!(pDesc->BindFlags & BIND_SHADER_RESOURCE))
            {
                desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            }
        }
        else if (desc.SampleDesc.Count == 1)
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
        }
        if (pDesc->BindFlags & BIND_RENDER_TARGET)
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
        }
        if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        switch (pTexture->desc.type)
        {
        case TextureDesc::TEXTURE_1D:
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;
        case TextureDesc::TEXTURE_2D:
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        case TextureDesc::TEXTURE_3D:
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            desc.DepthOrArraySize = (UINT16)pDesc->Depth;
            break;
        default:
            assert(0);
            break;
        }

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Color[0] = pTexture->desc.clear.color[0];
        optimizedClearValue.Color[1] = pTexture->desc.clear.color[1];
        optimizedClearValue.Color[2] = pTexture->desc.clear.color[2];
        optimizedClearValue.Color[3] = pTexture->desc.clear.color[3];
        optimizedClearValue.DepthStencil.Depth = pTexture->desc.clear.depthstencil.depth;
        optimizedClearValue.DepthStencil.Stencil = pTexture->desc.clear.depthstencil.stencil;
        optimizedClearValue.Format = desc.Format;
        if (optimizedClearValue.Format == DXGI_FORMAT_R16_TYPELESS)
        {
            optimizedClearValue.Format = DXGI_FORMAT_D16_UNORM;
        }
        else if (optimizedClearValue.Format == DXGI_FORMAT_R32_TYPELESS)
        {
            optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        }
        else if (optimizedClearValue.Format == DXGI_FORMAT_R32G8X24_TYPELESS)
        {
            optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        }
        bool useClearValue = pDesc->BindFlags & BIND_RENDER_TARGET || pDesc->BindFlags & BIND_DEPTH_STENCIL;

        D3D12_RESOURCE_STATES resourceState = _ConvertImageLayout(pTexture->desc.layout);

        if (pTexture->desc.Usage == USAGE_STAGING)
        {
            UINT64 RequiredSize = 0;
            device->GetCopyableFootprints(&desc, 0, 1, 0, &internal_state->footprint, nullptr, nullptr, &RequiredSize);
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = RequiredSize;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            if (pTexture->desc.CPUAccessFlags & CPU_ACCESS_READ)
            {
                allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
                resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
            }
            else
            {
                allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
                resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
            }
        }

        hr = allocationhandler->allocator->CreateResource(
            &allocationDesc,
            &desc,
            resourceState,
            useClearValue ? &optimizedClearValue : nullptr,
            &internal_state->allocation,
            IID_PPV_ARGS(&internal_state->resource)
        );
        assert(SUCCEEDED(hr));

        if (pTexture->desc.MipLevels == 0)
        {
            pTexture->desc.MipLevels = (uint32_t)log2(Max(pTexture->desc.Width, pTexture->desc.Height)) + 1;
        }


        // Issue data copy on request:
        if (pInitialData != nullptr)
        {
            uint32_t dataCount = pDesc->ArraySize * Max(1u, pDesc->MipLevels);
            std::vector<D3D12_SUBRESOURCE_DATA> data(dataCount);
            for (uint32_t slice = 0; slice < dataCount; ++slice)
            {
                data[slice] = _ConvertSubresourceData(pInitialData[slice]);
            }

            UINT64 RequiredSize = 0;
            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(dataCount);
            std::vector<UINT64> rowSizesInBytes(dataCount);
            std::vector<UINT> numRows(dataCount);
            device->GetCopyableFootprints(&desc, 0, dataCount, 0, layouts.data(), numRows.data(), rowSizesInBytes.data(), &RequiredSize);

            GPUBufferDesc uploaddesc;
            uploaddesc.ByteWidth = (uint32_t)RequiredSize;
            uploaddesc.Usage = USAGE_STAGING;
            RefPtr<GraphicsBuffer> uploadBuffer = CreateBuffer(uploaddesc, nullptr);
            assert(uploadBuffer.IsNotNull());
            ID3D12Resource* upload_resource = to_internal(uploadBuffer.Get())->resource.Get();

            uint8_t* pData;
            CD3DX12_RANGE readRange(0, 0);
            hr = upload_resource->Map(0, &readRange, (void**)&pData);
            assert(SUCCEEDED(hr));

            for (uint32_t i = 0; i < dataCount; ++i)
            {
                if (rowSizesInBytes[i] > (SIZE_T)-1) return 0;
                D3D12_MEMCPY_DEST DestData = { pData + layouts[i].Offset, layouts[i].Footprint.RowPitch, layouts[i].Footprint.RowPitch * numRows[i] };
                MemcpySubresource(&DestData, &data[i], (SIZE_T)rowSizesInBytes[i], numRows[i], layouts[i].Footprint.Depth);
            }

            copyQueueLock.lock();
            {
                auto& frame = GetFrameResources();
                if (!copyQueueUse)
                {
                    copyQueueUse = true;

                    HRESULT hr = frame.copyAllocator->Reset();
                    assert(SUCCEEDED(hr));
                    hr = frame.copyCommandList->Reset(frame.copyAllocator, nullptr);
                    assert(SUCCEEDED(hr));
                }

                for (UINT i = 0; i < dataCount; ++i)
                {
                    CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state->resource.Get(), i);
                    CD3DX12_TEXTURE_COPY_LOCATION Src(upload_resource, layouts[i]);
                    frame.copyCommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
                }
            }
            copyQueueLock.unlock();
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

    bool GraphicsDevice_DX12::CreateShader(ShaderStage stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader)
    {
        auto internal_state = std::make_shared<PipelineState_DX12>();
        internal_state->allocationhandler = allocationhandler;
        pShader->internal_state = internal_state;

        pShader->code.resize(BytecodeLength);
        std::memcpy(pShader->code.data(), pShaderBytecode, BytecodeLength);
        pShader->stage = stage;

        HRESULT hr = (pShader->code.empty() ? E_FAIL : S_OK);
        assert(SUCCEEDED(hr));


        if (pShader->rootSignature == nullptr)
        {
#ifdef _WIN64 // TODO: Can't use dxcompiler.dll in 32-bit, so can't use shader reflection
#ifndef PLATFORM_UWP // TODO: Can't use dxcompiler.dll in UWP, so can't use shader reflection
            struct ShaderBlob : public IDxcBlob
            {
                LPVOID address;
                SIZE_T size;
                LPVOID STDMETHODCALLTYPE GetBufferPointer() override { return address; }
                SIZE_T STDMETHODCALLTYPE GetBufferSize() override { return size; }
                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) { return E_FAIL; }
                ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
                ULONG STDMETHODCALLTYPE Release(void) { return 0; }
            };
            ShaderBlob blob;
            blob.address = (LPVOID)pShaderBytecode;
            blob.size = BytecodeLength;

            ComPtr<IDxcContainerReflection> container_reflection;
            hr = DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), (void**)&container_reflection);
            assert(SUCCEEDED(hr));
            hr = container_reflection->Load(&blob);
            assert(SUCCEEDED(hr));

            UINT32 shaderIdx;
            hr = container_reflection->FindFirstPartKind('LIXD', &shaderIdx); // Say 'DXIL' in Little-Endian
            assert(SUCCEEDED(hr));

            auto insert_descriptor = [&](const D3D12_SHADER_INPUT_BIND_DESC& desc)
            {
                if (desc.Type == D3D_SIT_SAMPLER)
                {
                    internal_state->samplers.emplace_back();
                    D3D12_DESCRIPTOR_RANGE& descriptor = internal_state->samplers.back();

                    descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                    descriptor.BaseShaderRegister = desc.BindPoint;
                    descriptor.NumDescriptors = desc.BindCount;
                    descriptor.RegisterSpace = desc.Space;
                    descriptor.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                }
                else
                {
                    internal_state->resources.emplace_back();
                    D3D12_DESCRIPTOR_RANGE& descriptor = internal_state->resources.back();

                    switch (desc.Type)
                    {
                    default:
                    case D3D_SIT_CBUFFER:
                        descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                        break;
                    case D3D_SIT_TBUFFER:
                    case D3D_SIT_TEXTURE:
                    case D3D_SIT_STRUCTURED:
                    case D3D_SIT_BYTEADDRESS:
                    case D3D_SIT_RTACCELERATIONSTRUCTURE:
                        descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        break;
                    case D3D_SIT_UAV_RWTYPED:
                    case D3D_SIT_UAV_RWSTRUCTURED:
                    case D3D_SIT_UAV_RWBYTEADDRESS:
                    case D3D_SIT_UAV_APPEND_STRUCTURED:
                    case D3D_SIT_UAV_CONSUME_STRUCTURED:
                    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                    case D3D_SIT_UAV_FEEDBACKTEXTURE:
                        descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                        break;
                    }
                    descriptor.BaseShaderRegister = desc.BindPoint;
                    descriptor.NumDescriptors = desc.BindCount;
                    descriptor.RegisterSpace = desc.Space;
                    descriptor.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                }
            };

            if (stage == ShaderStage::Count) // Library reflection
            {
                ComPtr<ID3D12LibraryReflection> reflection;
                hr = container_reflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&reflection));
                assert(SUCCEEDED(hr));

                D3D12_LIBRARY_DESC library_desc;
                hr = reflection->GetDesc(&library_desc);
                assert(SUCCEEDED(hr));

                for (UINT i = 0; i < library_desc.FunctionCount; ++i)
                {
                    ID3D12FunctionReflection* function_reflection = reflection->GetFunctionByIndex((INT)i);
                    assert(function_reflection != nullptr);
                    D3D12_FUNCTION_DESC function_desc;
                    hr = function_reflection->GetDesc(&function_desc);
                    assert(SUCCEEDED(hr));

                    for (UINT i = 0; i < function_desc.BoundResources; ++i)
                    {
                        D3D12_SHADER_INPUT_BIND_DESC desc;
                        hr = function_reflection->GetResourceBindingDesc(i, &desc);
                        assert(SUCCEEDED(hr));
                        insert_descriptor(desc);
                    }
                }
            }
            else // Shader reflection
            {
                ComPtr<ID3D12ShaderReflection> reflection;
                hr = container_reflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&reflection));
                assert(SUCCEEDED(hr));

                D3D12_SHADER_DESC shader_desc;
                hr = reflection->GetDesc(&shader_desc);
                assert(SUCCEEDED(hr));

                for (UINT i = 0; i < shader_desc.BoundResources; ++i)
                {
                    D3D12_SHADER_INPUT_BIND_DESC desc;
                    hr = reflection->GetResourceBindingDesc(i, &desc);
                    assert(SUCCEEDED(hr));
                    insert_descriptor(desc);
                }
            }
#endif // PLATFORM_UWP
#endif // _X64


            if (stage == ShaderStage::Compute || stage == ShaderStage::Count)
            {
                std::vector<D3D12_ROOT_PARAMETER> params;

                if (!internal_state->resources.empty())
                {
                    params.emplace_back();
                    D3D12_ROOT_PARAMETER& param = params.back();
                    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                    param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->resources.size();
                    param.DescriptorTable.pDescriptorRanges = internal_state->resources.data();
                }

                if (!internal_state->samplers.empty())
                {
                    params.emplace_back();
                    D3D12_ROOT_PARAMETER& param = params.back();
                    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                    param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->samplers.size();
                    param.DescriptorTable.pDescriptorRanges = internal_state->samplers.data();
                }

                D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
                rootSigDesc.NumStaticSamplers = 0;
                rootSigDesc.NumParameters = (UINT)params.size();
                rootSigDesc.pParameters = params.data();
                rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

                ID3DBlob* rootSigBlob;
                ID3DBlob* rootSigError;
                hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
                if (FAILED(hr))
                {
                    OutputDebugStringA((char*)rootSigError->GetBufferPointer());
                    assert(0);
                }
                hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->rootSignature));
                assert(SUCCEEDED(hr));

                if (stage == ShaderStage::Compute)
                {
                    struct PSO_STREAM
                    {
                        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
                        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
                    } stream;

                    stream.pRootSignature = internal_state->rootSignature;
                    stream.CS = { pShader->code.data(), pShader->code.size() };

                    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
                    streamDesc.pPipelineStateSubobjectStream = &stream;
                    streamDesc.SizeInBytes = sizeof(stream);

                    hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&internal_state->handle));
                    assert(SUCCEEDED(hr));
                }
            }
        }

        return SUCCEEDED(hr);
    }

    bool GraphicsDevice_DX12::CreateShader(ShaderStage stage, const char* source, const char* entryPoint, Shader* pShader)
    {
#if defined(ALIMER_DISABLE_SHADER_COMPILER)
        pShader->internal_state = nullptr;
        return false;
#else
        IDxcLibrary* dxcLibrary = GetOrCreateDxcLibrary();
        IDxcCompiler* dxcCompiler = GetOrCreateDxcCompiler();

        ComPtr<IDxcIncludeHandler> includeHandler;
        ThrowIfFailed(dxcLibrary->CreateIncludeHandler(&includeHandler));

        ComPtr<IDxcBlobEncoding> sourceBlob;
        ThrowIfFailed(dxcLibrary->CreateBlobWithEncodingOnHeapCopy(source, (UINT32)strlen(source), CP_UTF8, &sourceBlob));

        WString entryPointW = ToUtf16(entryPoint);
        Vector<const wchar_t*> arguments;
        arguments.push_back(L"/Zpc"); // Column major
#ifdef _DEBUG
        arguments.push_back(L"/Zi");
#else
        arguments.push_back(L"/O3");
#endif
        arguments.push_back(L"-all_resources_bound");
        //arguments.push_back(L"-Vd");

        // Enable FXC backward compatibility by setting the language version to 2016
        //arguments.push_back(L"-HV");
        //arguments.push_back(L"2016");

        const wchar_t* target = L"vs_6_1";
        switch (stage)
        {
        case ShaderStage::Hull:
            target = L"hs_6_1";
            break;
        case ShaderStage::Domain:
            target = L"ds_6_1";
            break;
        case ShaderStage::Geometry:
            target = L"gs_6_1";
            break;
        case ShaderStage::Fragment:
            target = L"ps_6_1";
            break;
        case ShaderStage::Compute:
            target = L"cs_6_1";
            break;
        }

        ComPtr<IDxcOperationResult> result;
        ThrowIfFailed(dxcCompiler->Compile(
            sourceBlob.Get(),
            nullptr,
            entryPointW.c_str(),
            target,
            arguments.data(),
            (UINT32)arguments.size(),
            nullptr,
            0,
            includeHandler.Get(),
            &result));

        HRESULT hr;
        ThrowIfFailed(result->GetStatus(&hr));

        if (FAILED(hr))
        {
            ComPtr<IDxcBlobEncoding> errors;
            ThrowIfFailed(result->GetErrorBuffer(&errors));
            std::string message = std::string("DXC compile failed with ") + static_cast<char*>(errors->GetBufferPointer());
            LOGE("{}", message);
            return false;
        }

        ComPtr<IDxcBlob> compiledShader;
        ThrowIfFailed(result->GetResult(&compiledShader));

        return CreateShader(stage, compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), pShader);
#endif
    }

    RefPtr<Sampler> GraphicsDevice_DX12::CreateSampler(const SamplerDescriptor* descriptor)
    {
        RefPtr<Sampler_DX12> result(new Sampler_DX12());
        result->allocationhandler = allocationhandler;

        D3D12_SAMPLER_DESC desc;
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
            desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
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

        result->descriptor = desc;

        return result;
    }

    bool GraphicsDevice_DX12::CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery)
    {
        auto internal_state = std::make_shared<Query_DX12>();
        internal_state->allocationhandler = allocationhandler;
        pQuery->internal_state = internal_state;

        HRESULT hr = E_FAIL;

        pQuery->desc = *pDesc;
        internal_state->query_type = pQuery->desc.Type;

        switch (pDesc->Type)
        {
        case GPU_QUERY_TYPE_TIMESTAMP:
            if (allocationhandler->free_timestampqueries.pop_front(internal_state->query_index))
            {
                hr = S_OK;
            }
            else
            {
                internal_state->query_type = GPU_QUERY_TYPE_INVALID;
                assert(0);
            }
            break;
        case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
            hr = S_OK;
            break;
        case GPU_QUERY_TYPE_OCCLUSION:
        case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
            if (allocationhandler->free_occlusionqueries.pop_front(internal_state->query_index))
            {
                hr = S_OK;
            }
            else
            {
                internal_state->query_type = GPU_QUERY_TYPE_INVALID;
                assert(0);
            }
            break;
        }

        assert(SUCCEEDED(hr));

        return SUCCEEDED(hr);
    }

    bool GraphicsDevice_DX12::CreateRenderPipelineCore(const RenderPipelineDescriptor* descriptor, RenderPipeline** pipeline)
    {
        RefPtr<PipelineState_DX12> internal_state(new PipelineState_DX12());
        internal_state->allocationhandler = allocationhandler;
        internal_state->desc = *descriptor;

        if (descriptor->rootSignature == nullptr)
        {
            // Root signature comes from reflection data when there is no root signature specified:

            auto insert_shader = [&](const Shader* shader)
            {
                if (shader == nullptr)
                    return;

                auto shader_internal = to_internal(shader);

                if (shader_internal->resources.empty() && shader_internal->samplers.empty())
                    return;

                size_t check_max = internal_state->resources.size(); // dont't check for duplicates within self table
                for (auto& x : shader_internal->resources)
                {
                    bool found = false;
                    size_t i = 0;
                    for (auto& y : internal_state->resources)
                    {
                        if (x.BaseShaderRegister == y.BaseShaderRegister && x.RangeType == y.RangeType)
                        {
                            found = true;
                            break;
                        }
                        if (i++ >= check_max)
                            break;
                    }

                    if (!found)
                    {
                        internal_state->resources.push_back(x);
                    }
                }

                check_max = internal_state->samplers.size(); // dont't check for duplicates within self table
                for (auto& x : shader_internal->samplers)
                {
                    bool found = false;
                    size_t i = 0;
                    for (auto& y : internal_state->samplers)
                    {
                        if (x.BaseShaderRegister == y.BaseShaderRegister && x.RangeType == y.RangeType)
                        {
                            found = true;
                            break;
                        }
                        if (i++ >= check_max)
                            break;
                    }

                    if (!found)
                    {
                        internal_state->samplers.push_back(x);
                    }
                }
            };

            insert_shader(descriptor->ms);
            insert_shader(descriptor->as);
            insert_shader(descriptor->vs);
            insert_shader(descriptor->hs);
            insert_shader(descriptor->ds);
            insert_shader(descriptor->gs);
            insert_shader(descriptor->ps);

            std::vector<D3D12_ROOT_PARAMETER> params;

            if (!internal_state->resources.empty())
            {
                params.emplace_back();
                D3D12_ROOT_PARAMETER& param = params.back();
                param = {};
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->resources.size();
                param.DescriptorTable.pDescriptorRanges = internal_state->resources.data();
            }

            if (!internal_state->samplers.empty())
            {
                params.emplace_back();
                D3D12_ROOT_PARAMETER& param = params.back();
                param = {};
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->samplers.size();
                param.DescriptorTable.pDescriptorRanges = internal_state->samplers.data();
            }

            D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
            rootSigDesc.NumStaticSamplers = 0;
            rootSigDesc.NumParameters = (UINT)params.size();
            rootSigDesc.pParameters = params.data();
            rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ID3DBlob* rootSigBlob;
            ID3DBlob* rootSigError;
            HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
            if (FAILED(hr))
            {
                assert(0);
                OutputDebugStringA((char*)rootSigError->GetBufferPointer());
            }
            hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->rootSignature));
            assert(SUCCEEDED(hr));
        }

        struct PSO_STREAM
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_HS HS;
            CD3DX12_PIPELINE_STATE_STREAM_DS DS;
            CD3DX12_PIPELINE_STATE_STREAM_GS GS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RS;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DSS;
            CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BD;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PT;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT IL;
            CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE STRIP;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSFormat;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS Formats;
            CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
            CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;

            CD3DX12_PIPELINE_STATE_STREAM_MS MS;
            CD3DX12_PIPELINE_STATE_STREAM_AS AS;
        } stream = {};

        if (descriptor->vs != nullptr)
        {
            stream.VS = { descriptor->vs->code.data(), descriptor->vs->code.size() };
        }
        if (descriptor->hs != nullptr)
        {
            stream.HS = { descriptor->hs->code.data(), descriptor->hs->code.size() };
        }
        if (descriptor->ds != nullptr)
        {
            stream.DS = { descriptor->ds->code.data(), descriptor->ds->code.size() };
        }
        if (descriptor->gs != nullptr)
        {
            stream.GS = { descriptor->gs->code.data(), descriptor->gs->code.size() };
        }
        if (descriptor->ps != nullptr)
        {
            stream.PS = { descriptor->ps->code.data(), descriptor->ps->code.size() };
        }

        if (descriptor->ms != nullptr)
        {
            stream.MS = { descriptor->ms->code.data(), descriptor->ms->code.size() };
        }
        if (descriptor->as != nullptr)
        {
            stream.AS = { descriptor->as->code.data(), descriptor->as->code.size() };
        }

        DepthStencilStateDescriptor depthStencilState = descriptor->depthStencilState;
        CD3DX12_DEPTH_STENCIL_DESC dss = {};
        dss.DepthEnable = depthStencilState.depthCompare != CompareFunction::Always || depthStencilState.depthWriteEnabled;
        dss.DepthWriteMask = depthStencilState.depthWriteEnabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        dss.DepthFunc = _ConvertComparisonFunc(depthStencilState.depthCompare);
        dss.StencilEnable = StencilTestEnabled(&depthStencilState) ? TRUE : FALSE;
        dss.StencilReadMask = depthStencilState.stencilReadMask;
        dss.StencilWriteMask = depthStencilState.stencilWriteMask;
        dss.FrontFace = _ConvertStencilOpDesc(depthStencilState.stencilFront);
        dss.BackFace = _ConvertStencilOpDesc(depthStencilState.stencilBack);
        stream.DSS = dss;

        CD3DX12_BLEND_DESC bd = {};
        bd.AlphaToCoverageEnable = descriptor->alphaToCoverageEnable;
        bd.IndependentBlendEnable = TRUE;
        for (uint32_t i = 0; i < kMaxColorAttachments; ++i)
        {
            bd.RenderTarget[i].BlendEnable = descriptor->colorAttachments[i].blendEnable;
            bd.RenderTarget[i].SrcBlend = _ConvertBlend(descriptor->colorAttachments[i].srcColorBlendFactor);
            bd.RenderTarget[i].DestBlend = _ConvertBlend(descriptor->colorAttachments[i].dstColorBlendFactor);
            bd.RenderTarget[i].BlendOp = _ConvertBlendOp(descriptor->colorAttachments[i].colorBlendOp);
            bd.RenderTarget[i].SrcBlendAlpha = _ConvertBlend(descriptor->colorAttachments[i].srcAlphaBlendFactor);
            bd.RenderTarget[i].DestBlendAlpha = _ConvertBlend(descriptor->colorAttachments[i].dstAlphaBlendFactor);
            bd.RenderTarget[i].BlendOpAlpha = _ConvertBlendOp(descriptor->colorAttachments[i].alphaBlendOp);
            bd.RenderTarget[i].RenderTargetWriteMask = _ConvertColorWriteMask(descriptor->colorAttachments[i].colorWriteMask);
        }
        stream.BD = bd;

        // InputLayout
        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
        std::array<D3D12_INPUT_ELEMENT_DESC, kMaxVertexAttributes> inputElements;
        for (uint32_t i = 0; i < kMaxVertexAttributes; ++i)
        {
            const VertexAttributeDescriptor* attrDesc = &descriptor->vertexDescriptor.attributes[i];
            if (attrDesc->format == VertexFormat::Invalid) {
                break;
            }

            const VertexBufferLayoutDescriptor* layoutDesc = &descriptor->vertexDescriptor.layouts[i];

            D3D12_INPUT_ELEMENT_DESC* inputElementDesc = &inputElements[inputLayoutDesc.NumElements++];
            inputElementDesc->SemanticName = "ATTRIBUTE";
            inputElementDesc->SemanticIndex = i;
            inputElementDesc->Format = D3DConvertVertexFormat(attrDesc->format);
            inputElementDesc->InputSlot = attrDesc->bufferIndex;
            inputElementDesc->AlignedByteOffset = attrDesc->offset; // D3D11_APPEND_ALIGNED_ELEMENT; // attrDesc->offset;
            if (layoutDesc->stepMode == InputStepMode::Vertex)
            {
                inputElementDesc->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                inputElementDesc->InstanceDataStepRate = 0;
            }
            else
            {
                inputElementDesc->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                inputElementDesc->InstanceDataStepRate = 1;
            }
        }

        inputLayoutDesc.pInputElementDescs = inputElements.data();
        stream.IL = inputLayoutDesc;

        DXGI_FORMAT DSFormat = DXGI_FORMAT_UNKNOWN;
        D3D12_RT_FORMAT_ARRAY formats = {};
        formats.NumRenderTargets = 0;

        for (uint32_t i = 0; i < kMaxColorAttachments; ++i)
        {
            if (descriptor->colorAttachments[i].format == PixelFormat::Invalid)
                break;

            switch (descriptor->colorAttachments[i].format)
            {
            case PixelFormat::FORMAT_R16_TYPELESS:
                formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R16_UNORM;
                break;
            case PixelFormat::FORMAT_R32_TYPELESS:
                formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT;
                break;
            case PixelFormat::FORMAT_R24G8_TYPELESS:
                formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;
            case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                break;
            default:
                formats.RTFormats[formats.NumRenderTargets] = PixelFormatToDXGIFormat(descriptor->colorAttachments[i].format);
                break;
            }

            formats.NumRenderTargets++;
        }

        /*if (active_renderpass[cmd] == nullptr)
        {
            formats.NumRenderTargets = 1;
            formats.RTFormats[0] = PixelFormatToDXGIFormat(BACKBUFFER_FORMAT);
        }
        else
        {
            for (auto& attachment : active_renderpass[cmd]->desc.attachments)
            {
                if (attachment.type == RenderPassAttachment::RESOLVE || attachment.texture == nullptr)
                    continue;

                switch (attachment.type)
                {
                case RenderPassAttachment::RENDERTARGET:
                    switch (attachment.texture->desc.format)
                    {
                    case PixelFormat::FORMAT_R16_TYPELESS:
                        formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R16_UNORM;
                        break;
                    case PixelFormat::FORMAT_R32_TYPELESS:
                        formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT;
                        break;
                    case PixelFormat::FORMAT_R24G8_TYPELESS:
                        formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                        break;
                    case PixelFormat::FORMAT_R32G8X24_TYPELESS:
                        formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                        break;
                    default:
                        formats.RTFormats[formats.NumRenderTargets] = _ConvertFormat(attachment.texture->desc.Format);
                        break;
                    }
                    formats.NumRenderTargets++;
                    break;
                case RenderPassAttachment::DEPTH_STENCIL:
                    switch (attachment.texture->desc.Format)
                    {
                    case FORMAT_R16_TYPELESS:
                        DSFormat = DXGI_FORMAT_D16_UNORM;
                        break;
                    case FORMAT_R32_TYPELESS:
                        DSFormat = DXGI_FORMAT_D32_FLOAT;
                        break;
                    case FORMAT_R24G8_TYPELESS:
                        DSFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                        break;
                    case FORMAT_R32G8X24_TYPELESS:
                        DSFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
                        break;
                    default:
                        DSFormat = _ConvertFormat(attachment.texture->desc.Format);
                        break;
                    }
                    break;
                default:
                    assert(0);
                    break;
                }

                sampleDesc.Count = attachment.texture->desc.SampleCount;
                sampleDesc.Quality = 0;
            }
        }*/
        stream.DSFormat = DSFormat;
        stream.Formats = formats;

        DXGI_SAMPLE_DESC sampleDesc = { descriptor->sampleCount, 0 };
        stream.SampleDesc = sampleDesc;
        stream.SampleMask = descriptor->sampleMask;

        RasterizationStateDescriptor rasterizationState = descriptor->rasterizationState;
        CD3DX12_RASTERIZER_DESC rs = {};
        rs.FillMode = D3D12_FILL_MODE_SOLID;
        rs.CullMode = _ConvertCullMode(rasterizationState.cullMode);
        rs.FrontCounterClockwise = (rasterizationState.frontFace == FrontFace::CCW) ? TRUE : FALSE;
        rs.DepthBias = rasterizationState.depthBias;
        rs.DepthBiasClamp = rasterizationState.depthBiasClamp;
        rs.SlopeScaledDepthBias = rasterizationState.depthBiasSlopeScale;
        rs.DepthClipEnable = rasterizationState.depthClipEnable;
        rs.MultisampleEnable = (sampleDesc.Count > 1) ? TRUE : FALSE;
        rs.AntialiasedLineEnable = FALSE;
        rs.ConservativeRaster = ((CONSERVATIVE_RASTERIZATION && rasterizationState.conservativeRasterizationEnable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
        rs.ForcedSampleCount = rasterizationState.forcedSampleCount;
        stream.RS = rs;

        internal_state->primitiveTopology = D3DPrimitiveTopology(descriptor->primitiveTopology);
        switch (descriptor->primitiveTopology)
        {
        case PrimitiveTopology::PointList:
            stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            break;

        case PrimitiveTopology::LineList:
        case PrimitiveTopology::LineStrip:
            stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            break;

        case PrimitiveTopology::TriangleList:
        case PrimitiveTopology::TriangleStrip:
            stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;

        case PrimitiveTopology::PatchList:
            stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
            break;

        default:
            internal_state->primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
            stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
            break;
        }

        stream.STRIP = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

        if (descriptor->rootSignature == nullptr)
        {
            stream.pRootSignature = internal_state->rootSignature;
        }
        else
        {
            stream.pRootSignature = to_internal(descriptor->rootSignature)->resource;
        }

        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
        streamDesc.pPipelineStateSubobjectStream = &stream;
        streamDesc.SizeInBytes = sizeof(stream);

        HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&internal_state->handle));
        assert(SUCCEEDED(hr));

        *pipeline = internal_state.Detach();
        return true;
    }

    bool GraphicsDevice_DX12::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
    {
        auto internal_state = std::make_shared<RenderPass_DX12>();
        renderpass->internal_state = internal_state;

        renderpass->desc = *pDesc;

        renderpass->hash = 0;
        CombineHash(renderpass->hash, pDesc->attachments.size());
        for (auto& attachment : pDesc->attachments)
        {
            CombineHash(renderpass->hash, attachment.texture->desc.format);
            CombineHash(renderpass->hash, attachment.texture->desc.SampleCount);
        }

        // Beginning barriers:
        for (auto& attachment : renderpass->desc.attachments)
        {
            if (attachment.texture == nullptr)
                continue;

            auto texture_internal = to_internal(attachment.texture);

            D3D12_RESOURCE_BARRIER& barrierdesc = internal_state->barrierdescs_begin[internal_state->num_barriers_begin++];

            barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierdesc.Transition.pResource = texture_internal->resource.Get();
            barrierdesc.Transition.StateBefore = _ConvertImageLayout(attachment.initial_layout);
            if (attachment.type == RenderPassAttachment::RESOLVE)
            {
                barrierdesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST;
            }
            else
            {
                barrierdesc.Transition.StateAfter = _ConvertImageLayout(attachment.subpass_layout);
            }
            barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            if (barrierdesc.Transition.StateBefore == barrierdesc.Transition.StateAfter)
            {
                internal_state->num_barriers_begin--;
                continue;
            }
        }

        // Ending barriers:
        for (auto& attachment : renderpass->desc.attachments)
        {
            if (attachment.texture == nullptr)
                continue;

            auto texture_internal = to_internal(attachment.texture);

            D3D12_RESOURCE_BARRIER& barrierdesc = internal_state->barrierdescs_end[internal_state->num_barriers_end++];

            barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierdesc.Transition.pResource = texture_internal->resource.Get();
            if (attachment.type == RenderPassAttachment::RESOLVE)
            {
                barrierdesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
            }
            else
            {
                barrierdesc.Transition.StateBefore = _ConvertImageLayout(attachment.subpass_layout);
            }
            barrierdesc.Transition.StateAfter = _ConvertImageLayout(attachment.final_layout);
            barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            if (barrierdesc.Transition.StateBefore == barrierdesc.Transition.StateAfter)
            {
                internal_state->num_barriers_end--;
                continue;
            }
        }

        return true;
    }
    bool GraphicsDevice_DX12::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh)
    {
        auto internal_state = std::make_shared<BVH_DX12>();
        internal_state->allocationhandler = allocationhandler;
        bvh->internal_state = internal_state;
        bvh->type = GPUResource::GPU_RESOURCE_TYPE::RAYTRACING_ACCELERATION_STRUCTURE;

        bvh->desc = *pDesc;

        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
        {
            internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        }
        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
        {
            internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
        }
        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
        {
            internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        }
        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
        {
            internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
        }
        if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
        {
            internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
        }


        switch (pDesc->type)
        {
        case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
        {
            internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

            for (auto& x : pDesc->bottomlevel.geometries)
            {
                internal_state->geometries.emplace_back();
                auto& geometry = internal_state->geometries.back();
                geometry = {};

                if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
                {
                    geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                    geometry.Triangles.VertexBuffer.StartAddress = to_internal(x.triangles.vertexBuffer)->resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertexByteOffset;
                    geometry.Triangles.VertexBuffer.StrideInBytes = (UINT64)x.triangles.vertexStride;
                    geometry.Triangles.VertexCount = x.triangles.vertexCount;
                    geometry.Triangles.VertexFormat = D3DConvertVertexFormat(x.triangles.vertexFormat);
                    geometry.Triangles.IndexFormat = (x.triangles.indexFormat == IndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
                    geometry.Triangles.IndexBuffer = to_internal(x.triangles.indexBuffer)->resource->GetGPUVirtualAddress() +
                        (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.indexOffset * (x.triangles.indexFormat == IndexFormat::UInt16 ? sizeof(uint16_t) : sizeof(uint32_t));
                    geometry.Triangles.IndexCount = x.triangles.indexCount;

                    if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
                    {
                        geometry.Triangles.Transform3x4 = to_internal(x.triangles.transform3x4Buffer)->resource->GetGPUVirtualAddress() +
                            (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform3x4BufferOffset;
                    }
                }
                else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
                {
                    geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
                    geometry.AABBs.AABBs.StartAddress = to_internal(x.aabbs.aabbBuffer)->resource->GetGPUVirtualAddress() +
                        (D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
                    geometry.AABBs.AABBs.StrideInBytes = (UINT64)x.aabbs.stride;
                    geometry.AABBs.AABBCount = x.aabbs.count;
                }
            }

            internal_state->desc.pGeometryDescs = internal_state->geometries.data();
            internal_state->desc.NumDescs = (UINT)internal_state->geometries.size();
        }
        break;
        case RaytracingAccelerationStructureDesc::TOPLEVEL:
        {
            internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

            internal_state->desc.InstanceDescs = to_internal(pDesc->toplevel.instanceBuffer)->resource->GetGPUVirtualAddress() +
                (D3D12_GPU_VIRTUAL_ADDRESS)pDesc->toplevel.offset;
            internal_state->desc.NumDescs = (UINT)pDesc->toplevel.count;
        }
        break;
        }

        device->GetRaytracingAccelerationStructurePrebuildInfo(&internal_state->desc, &internal_state->info);


        size_t alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
        size_t alignedSize = Align((size_t)internal_state->info.ResultDataMaxSizeInBytes, alignment);

        D3D12_RESOURCE_DESC desc;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Width = (UINT64)alignedSize;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.DepthOrArraySize = 1;
        desc.Alignment = 0;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

        HRESULT hr = allocationhandler->allocator->CreateResource(
            &allocationDesc,
            &desc,
            resourceState,
            nullptr,
            &internal_state->allocation,
            IID_PPV_ARGS(&internal_state->resource)
        );
        assert(SUCCEEDED(hr));

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        srv_desc.RaytracingAccelerationStructure.Location = internal_state->resource->GetGPUVirtualAddress();

        internal_state->srv = srv_desc;

        GPUBufferDesc scratch_desc;
        scratch_desc.ByteWidth = (uint32_t)Max(internal_state->info.ScratchDataSizeInBytes, internal_state->info.UpdateScratchDataSizeInBytes);

        internal_state->scratch = CreateBuffer(scratch_desc, nullptr);
        return internal_state->scratch != nullptr;
    }

    bool GraphicsDevice_DX12::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso)
    {
        auto internal_state = std::make_shared<RTPipelineState_DX12>();
        internal_state->allocationhandler = allocationhandler;
        rtpso->internal_state = internal_state;

        rtpso->desc = *pDesc;

        D3D12_STATE_OBJECT_DESC desc = {};
        desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

        std::vector<D3D12_STATE_SUBOBJECT> subobjects;

        D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = {};
        {
            subobjects.emplace_back();
            auto& subobject = subobjects.back();
            subobject = {};
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
            pipeline_config.MaxTraceRecursionDepth = pDesc->max_trace_recursion_depth;
            subobject.pDesc = &pipeline_config;
        }

        D3D12_RAYTRACING_SHADER_CONFIG shader_config = {};
        {
            subobjects.emplace_back();
            auto& subobject = subobjects.back();
            subobject = {};
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            shader_config.MaxAttributeSizeInBytes = pDesc->max_attribute_size_in_bytes;
            shader_config.MaxPayloadSizeInBytes = pDesc->max_payload_size_in_bytes;
            subobject.pDesc = &shader_config;
        }

        D3D12_GLOBAL_ROOT_SIGNATURE global_rootsig = {};
        {
            subobjects.emplace_back();
            auto& subobject = subobjects.back();
            subobject = {};
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
            if (pDesc->rootSignature == nullptr)
            {
                auto shader_internal = to_internal(pDesc->shaderlibraries.front().shader); // think better way
                global_rootsig.pGlobalRootSignature = shader_internal->rootSignature;
            }
            else
            {
                global_rootsig.pGlobalRootSignature = to_internal(pDesc->rootSignature)->resource;
            }
            subobject.pDesc = &global_rootsig;
        }

        internal_state->exports.reserve(pDesc->shaderlibraries.size());
        internal_state->library_descs.reserve(pDesc->shaderlibraries.size());
        for (auto& x : pDesc->shaderlibraries)
        {
            subobjects.emplace_back();
            auto& subobject = subobjects.back();
            subobject = {};
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            internal_state->library_descs.emplace_back();
            auto& library_desc = internal_state->library_descs.back();
            library_desc = {};
            library_desc.DXILLibrary.pShaderBytecode = x.shader->code.data();
            library_desc.DXILLibrary.BytecodeLength = x.shader->code.size();
            library_desc.NumExports = 1;

            internal_state->exports.emplace_back();
            D3D12_EXPORT_DESC& export_desc = internal_state->exports.back();
            internal_state->export_strings.push_back(ToUtf16(x.function_name));
            export_desc.Name = internal_state->export_strings.back().c_str();
            library_desc.pExports = &export_desc;

            subobject.pDesc = &library_desc;
        }

        internal_state->hitgroup_descs.reserve(pDesc->hitgroups.size());
        for (auto& x : pDesc->hitgroups)
        {
            internal_state->group_strings.push_back(ToUtf16(x.name));

            if (x.type == ShaderHitGroup::GENERAL)
                continue;
            subobjects.emplace_back();
            auto& subobject = subobjects.back();
            subobject = {};
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            internal_state->hitgroup_descs.emplace_back();
            auto& hitgroup_desc = internal_state->hitgroup_descs.back();
            hitgroup_desc = {};
            switch (x.type)
            {
            default:
            case ShaderHitGroup::TRIANGLES:
                hitgroup_desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
                break;
            case ShaderHitGroup::PROCEDURAL:
                hitgroup_desc.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
                break;
            }
            if (!x.name.empty())
            {
                hitgroup_desc.HitGroupExport = internal_state->group_strings.back().c_str();
            }
            if (x.closesthit_shader != ~0)
            {
                hitgroup_desc.ClosestHitShaderImport = internal_state->exports[x.closesthit_shader].Name;
            }
            if (x.anyhit_shader != ~0)
            {
                hitgroup_desc.AnyHitShaderImport = internal_state->exports[x.anyhit_shader].Name;
            }
            if (x.intersection_shader != ~0)
            {
                hitgroup_desc.IntersectionShaderImport = internal_state->exports[x.intersection_shader].Name;
            }
            subobject.pDesc = &hitgroup_desc;
        }

        desc.NumSubobjects = (UINT)subobjects.size();
        desc.pSubobjects = subobjects.data();

        HRESULT hr = device->CreateStateObject(&desc, IID_PPV_ARGS(&internal_state->resource));
        assert(SUCCEEDED(hr));

        return SUCCEEDED(hr);
    }
    bool GraphicsDevice_DX12::CreateDescriptorTable(DescriptorTable* table)
    {
        auto internal_state = std::make_shared<DescriptorTable_DX12>();
        internal_state->allocationhandler = allocationhandler;
        table->internal_state = internal_state;

        internal_state->resource_heap.desc.NodeMask = 0;
        internal_state->resource_heap.desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        internal_state->resource_heap.desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        size_t prefix_sum = 0;
        for (auto& x : table->resources)
        {
            if (x.binding < CONSTANTBUFFER)
            {
                internal_state->resource_heap.write_remap.push_back(prefix_sum);
                continue;
            }

            internal_state->resource_heap.ranges.emplace_back();
            auto& range = internal_state->resource_heap.ranges.back();
            range = {};
            range.BaseShaderRegister = x.slot;
            range.NumDescriptors = x.count;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            range.RegisterSpace = 0; // this will be filled by root signature depending on the table position (to mirror Vulkan behaviour)
            internal_state->resource_heap.desc.NumDescriptors += range.NumDescriptors;

            switch (x.binding)
            {
            case CONSTANTBUFFER:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                break;
            case RAWBUFFER:
            case STRUCTUREDBUFFER:
            case TYPEDBUFFER:
            case TEXTURE1D:
            case TEXTURE1DARRAY:
            case TEXTURE2D:
            case TEXTURE2DARRAY:
            case TEXTURECUBE:
            case TEXTURECUBEARRAY:
            case TEXTURE3D:
            case ACCELERATIONSTRUCTURE:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                break;
            case RWRAWBUFFER:
            case RWSTRUCTUREDBUFFER:
            case RWTYPEDBUFFER:
            case RWTEXTURE1D:
            case RWTEXTURE1DARRAY:
            case RWTEXTURE2D:
            case RWTEXTURE2DARRAY:
            case RWTEXTURE3D:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;
            default:
                assert(0);
                break;
            }

            internal_state->resource_heap.write_remap.push_back(prefix_sum);
            prefix_sum += (size_t)range.NumDescriptors;
        }

        internal_state->sampler_heap.desc.NodeMask = 0;
        internal_state->sampler_heap.desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        internal_state->sampler_heap.desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

        prefix_sum = 0;
        for (auto& x : table->samplers)
        {
            internal_state->sampler_heap.ranges.emplace_back();
            auto& range = internal_state->sampler_heap.ranges.back();
            range = {};
            range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            range.BaseShaderRegister = x.slot;
            range.NumDescriptors = x.count;
            range.RegisterSpace = 0; // this will be filled by root signature depending on the table position (to mirror Vulkan behaviour)
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            internal_state->sampler_heap.desc.NumDescriptors += range.NumDescriptors;

            internal_state->sampler_heap.write_remap.push_back(prefix_sum);
            prefix_sum += (size_t)range.NumDescriptors;
        }

        for (auto& x : table->staticsamplers)
        {
            auto sampler_internal_state = to_internal(x.sampler);

            internal_state->staticsamplers.emplace_back();
            auto& desc = internal_state->staticsamplers.back();
            desc = {};
            desc.ShaderRegister = x.slot;
            desc.Filter = sampler_internal_state->descriptor.Filter;
            desc.AddressU = sampler_internal_state->descriptor.AddressU;
            desc.AddressV = sampler_internal_state->descriptor.AddressV;
            desc.AddressW = sampler_internal_state->descriptor.AddressW;
            desc.MipLODBias = sampler_internal_state->descriptor.MipLODBias;
            desc.MaxAnisotropy = sampler_internal_state->descriptor.MaxAnisotropy;
            desc.ComparisonFunc = sampler_internal_state->descriptor.ComparisonFunc;
            desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            desc.MinLOD = sampler_internal_state->descriptor.MinLOD;
            desc.MaxLOD = sampler_internal_state->descriptor.MaxLOD;
        }

        HRESULT hr = S_OK;

        if (internal_state->resource_heap.desc.NumDescriptors > 0)
        {
            hr = device->CreateDescriptorHeap(&internal_state->resource_heap.desc, IID_PPV_ARGS(&internal_state->resource_heap.heap));
            assert(SUCCEEDED(hr));
            internal_state->resource_heap.address = internal_state->resource_heap.heap->GetCPUDescriptorHandleForHeapStart();

            uint32_t slot = 0;
            for (auto& x : table->resources)
            {
                for (uint32_t i = 0; i < x.count; ++i)
                {
                    WriteDescriptor(table, slot, i, (const GPUResource*)nullptr);
                }
                slot++;
            }
        }
        if (internal_state->sampler_heap.desc.NumDescriptors > 0)
        {
            hr = device->CreateDescriptorHeap(&internal_state->sampler_heap.desc, IID_PPV_ARGS(&internal_state->sampler_heap.heap));
            assert(SUCCEEDED(hr));
            internal_state->sampler_heap.address = internal_state->sampler_heap.heap->GetCPUDescriptorHandleForHeapStart();

            uint32_t slot = 0;
            for (auto& x : table->samplers)
            {
                for (uint32_t i = 0; i < x.count; ++i)
                {
                    WriteDescriptor(table, slot, i, (const Sampler*)nullptr);
                }
                slot++;
            }
        }

        return SUCCEEDED(hr);
    }
    bool GraphicsDevice_DX12::CreateRootSignature(RootSignature* rootsig)
    {
        auto internal_state = std::make_shared<RootSignature_DX12>();
        internal_state->allocationhandler = allocationhandler;
        rootsig->internal_state = internal_state;

        internal_state->params.reserve(rootsig->tables.size());
        std::vector<D3D12_STATIC_SAMPLER_DESC> staticsamplers;

        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> table_ranges_resource;
        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> table_ranges_sampler;
        table_ranges_resource.reserve(rootsig->tables.size());
        table_ranges_sampler.reserve(rootsig->tables.size());

        uint32_t space = 0;
        for (auto& x : rootsig->tables)
        {
            table_ranges_resource.emplace_back();
            table_ranges_sampler.emplace_back();

            uint32_t rangeIndex = 0;
            for (auto& binding : x.resources)
            {
                if (binding.binding < CONSTANTBUFFER)
                {
                    assert(binding.count == 1); // descriptor array not allowed in the root
                    internal_state->root_remap.emplace_back();
                    internal_state->root_remap.back().space = space; // Space assignment for Root Signature
                    internal_state->root_remap.back().rangeIndex = rangeIndex;

                    internal_state->params.emplace_back();
                    auto& param = internal_state->params.back();
                    switch (binding.binding)
                    {
                    case ROOT_CONSTANTBUFFER:
                        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                        break;
                    case ROOT_RAWBUFFER:
                    case ROOT_STRUCTUREDBUFFER:
                        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
                        break;
                    case ROOT_RWRAWBUFFER:
                    case ROOT_RWSTRUCTUREDBUFFER:
                        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
                        break;
                    default:
                        break;
                    }
                    param.ShaderVisibility = _ConvertShaderVisibility(x.stage);
                    param.Descriptor.RegisterSpace = space;
                    param.Descriptor.ShaderRegister = binding.slot;
                }
                else
                {
                    // Space assignment for Root Signature:
                    table_ranges_resource.back().emplace_back();
                    D3D12_DESCRIPTOR_RANGE& range = table_ranges_resource.back().back();
                    auto table_internal = to_internal(&x);
                    range = table_internal->resource_heap.ranges[rangeIndex];
                    range.RegisterSpace = space;
                }
                rangeIndex++;
            }
            for (auto& binding : x.samplers)
            {
                // Space assignment for Root Signature:
                table_ranges_sampler.back().emplace_back();
                D3D12_DESCRIPTOR_RANGE& range = table_ranges_sampler.back().back();
                auto table_internal = to_internal(&x);
                range = table_internal->sampler_heap.ranges[rangeIndex];
                range.RegisterSpace = space;
            }
            space++;
        }

        space = 0;
        uint32_t bind_point = (uint32_t)internal_state->params.size();
        for (auto& x : rootsig->tables)
        {
            auto table_internal = to_internal(&x);

            if (table_internal->resource_heap.desc.NumDescriptors == 0 &&
                table_internal->sampler_heap.desc.NumDescriptors == 0)
            {
                // No real bind point
                internal_state->table_bind_point_remap.push_back(-1);
            }
            else
            {
                internal_state->table_bind_point_remap.push_back((int)bind_point);
            }

            if (table_internal->resource_heap.desc.NumDescriptors > 0)
            {
                internal_state->params.emplace_back();
                auto& param = internal_state->params.back();
                param = {};
                param.ShaderVisibility = _ConvertShaderVisibility(x.stage);
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.DescriptorTable.pDescriptorRanges = table_ranges_resource[space].data();
                param.DescriptorTable.NumDescriptorRanges = (UINT)table_ranges_resource[space].size();
                bind_point++;
            }
            if (table_internal->sampler_heap.desc.NumDescriptors > 0)
            {
                internal_state->params.emplace_back();
                auto& param = internal_state->params.back();
                param = {};
                param.ShaderVisibility = _ConvertShaderVisibility(x.stage);
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.DescriptorTable.pDescriptorRanges = table_ranges_sampler[space].data();
                param.DescriptorTable.NumDescriptorRanges = (UINT)table_ranges_sampler[space].size();
                bind_point++;
            }

            std::vector<D3D12_STATIC_SAMPLER_DESC> tmp_staticsamplers(table_internal->staticsamplers.begin(), table_internal->staticsamplers.end());
            for (auto& sam : tmp_staticsamplers)
            {
                // Space assignment for Root Signature:
                sam.RegisterSpace = space;
            }
            staticsamplers.insert(
                staticsamplers.end(),
                tmp_staticsamplers.begin(),
                tmp_staticsamplers.end()
            );

            space++;
        }

        internal_state->root_constant_bind_remap = bind_point;
        for (auto& x : rootsig->rootconstants)
        {
            internal_state->params.emplace_back();
            auto& param = internal_state->params.back();
            param = {};
            param.ShaderVisibility = _ConvertShaderVisibility(x.stage);
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            param.Constants.ShaderRegister = x.slot;
            param.Constants.RegisterSpace = 0;
            param.Constants.Num32BitValues = x.size / sizeof(uint32_t);
        }

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        if (rootsig->_flags & RootSignature::FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        {
            desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        }
        desc.NumParameters = (UINT)internal_state->params.size();
        desc.pParameters = internal_state->params.data();
        desc.NumStaticSamplers = (UINT)staticsamplers.size();
        desc.pStaticSamplers = staticsamplers.data();

        ID3DBlob* rootSigBlob;
        ID3DBlob* rootSigError;
        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
        if (FAILED(hr))
        {
            OutputDebugStringA((char*)rootSigError->GetBufferPointer());
            assert(0);
        }
        hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->resource));
        assert(SUCCEEDED(hr));

        return SUCCEEDED(hr);
    }

    int GraphicsDevice_DX12::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
    {
        auto internal_state = to_internal(texture);

        switch (type)
        {
        case SRV:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

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
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                    srv_desc.Texture1DArray.FirstArraySlice = firstSlice;
                    srv_desc.Texture1DArray.ArraySize = sliceCount;
                    srv_desc.Texture1DArray.MostDetailedMip = firstMip;
                    srv_desc.Texture1DArray.MipLevels = mipCount;
                }
                else
                {
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
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
                            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                            srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
                            srv_desc.TextureCubeArray.NumCubes = Min(texture->desc.ArraySize, sliceCount) / 6;
                            srv_desc.TextureCubeArray.MostDetailedMip = firstMip;
                            srv_desc.TextureCubeArray.MipLevels = mipCount;
                        }
                        else
                        {
                            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                            srv_desc.TextureCube.MostDetailedMip = firstMip;
                            srv_desc.TextureCube.MipLevels = mipCount;
                        }
                    }
                    else
                    {
                        if (texture->desc.SampleCount > 1)
                        {
                            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                            srv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
                            srv_desc.Texture2DMSArray.ArraySize = sliceCount;
                        }
                        else
                        {
                            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
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
                        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srv_desc.Texture2D.MostDetailedMip = firstMip;
                        srv_desc.Texture2D.MipLevels = mipCount;
                    }
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_3D)
            {
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                srv_desc.Texture3D.MostDetailedMip = firstMip;
                srv_desc.Texture3D.MipLevels = mipCount;
            }

            if (internal_state->srv.ViewDimension == D3D12_SRV_DIMENSION_UNKNOWN)
            {
                internal_state->srv = srv_desc;
                return -1;
            }
            internal_state->subresources_srv.push_back(srv_desc);
            return int(internal_state->subresources_srv.size() - 1);
        }
        break;
        case UAV:
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};

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
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                    uav_desc.Texture1DArray.FirstArraySlice = firstSlice;
                    uav_desc.Texture1DArray.ArraySize = sliceCount;
                    uav_desc.Texture1DArray.MipSlice = firstMip;
                }
                else
                {
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                    uav_desc.Texture1D.MipSlice = firstMip;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_2D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uav_desc.Texture2DArray.FirstArraySlice = firstSlice;
                    uav_desc.Texture2DArray.ArraySize = sliceCount;
                    uav_desc.Texture2DArray.MipSlice = firstMip;
                }
                else
                {
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    uav_desc.Texture2D.MipSlice = firstMip;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_3D)
            {
                uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                uav_desc.Texture3D.MipSlice = firstMip;
                uav_desc.Texture3D.FirstWSlice = 0;
                uav_desc.Texture3D.WSize = -1;
            }

            if (internal_state->uav.ViewDimension == D3D12_UAV_DIMENSION_UNKNOWN)
            {
                internal_state->uav = uav_desc;
                return -1;
            }
            internal_state->subresources_uav.push_back(uav_desc);
            return int(internal_state->subresources_uav.size() - 1);
        }
        break;
        case RTV:
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};

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
                    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                    rtv_desc.Texture1DArray.FirstArraySlice = firstSlice;
                    rtv_desc.Texture1DArray.ArraySize = sliceCount;
                    rtv_desc.Texture1DArray.MipSlice = firstMip;
                }
                else
                {
                    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                    rtv_desc.Texture1D.MipSlice = firstMip;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_2D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        rtv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
                        rtv_desc.Texture2DMSArray.ArraySize = sliceCount;
                    }
                    else
                    {
                        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                        rtv_desc.Texture2DArray.FirstArraySlice = firstSlice;
                        rtv_desc.Texture2DArray.ArraySize = sliceCount;
                        rtv_desc.Texture2DArray.MipSlice = firstMip;
                    }
                }
                else
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                        rtv_desc.Texture2D.MipSlice = firstMip;
                    }
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_3D)
            {
                rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                rtv_desc.Texture3D.MipSlice = firstMip;
                rtv_desc.Texture3D.FirstWSlice = 0;
                rtv_desc.Texture3D.WSize = -1;
            }

            if (internal_state->rtv.ViewDimension == D3D12_RTV_DIMENSION_UNKNOWN)
            {
                internal_state->rtv = rtv_desc;
                return -1;
            }
            internal_state->subresources_rtv.push_back(rtv_desc);
            return int(internal_state->subresources_rtv.size() - 1);
        }
        break;
        case DSV:
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};

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
                    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
                    dsv_desc.Texture1DArray.FirstArraySlice = firstSlice;
                    dsv_desc.Texture1DArray.ArraySize = sliceCount;
                    dsv_desc.Texture1DArray.MipSlice = firstMip;
                }
                else
                {
                    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
                    dsv_desc.Texture1D.MipSlice = firstMip;
                }
            }
            else if (texture->desc.type == TextureDesc::TEXTURE_2D)
            {
                if (texture->desc.ArraySize > 1)
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                        dsv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
                        dsv_desc.Texture2DMSArray.ArraySize = sliceCount;
                    }
                    else
                    {
                        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                        dsv_desc.Texture2DArray.FirstArraySlice = firstSlice;
                        dsv_desc.Texture2DArray.ArraySize = sliceCount;
                        dsv_desc.Texture2DArray.MipSlice = firstMip;
                    }
                }
                else
                {
                    if (texture->desc.SampleCount > 1)
                    {
                        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                        dsv_desc.Texture2D.MipSlice = firstMip;
                    }
                }
            }

            if (internal_state->dsv.ViewDimension == D3D12_DSV_DIMENSION_UNKNOWN)
            {
                internal_state->dsv = dsv_desc;
                return -1;
            }
            internal_state->subresources_dsv.push_back(dsv_desc);
            return int(internal_state->subresources_dsv.size() - 1);
        }
        break;
        default:
            break;
        }
        return -1;
    }

    int GraphicsDevice_DX12::CreateSubresource(GraphicsBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size)
    {
        auto internal_state = to_internal(buffer);
        const GPUBufferDesc& desc = buffer->GetDesc();

        switch (type)
        {
        case SRV:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            if (desc.MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
            {
                // This is a Raw Buffer
                srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srv_desc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
                srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
                srv_desc.Buffer.NumElements = Min((UINT)size, desc.ByteWidth - (UINT)offset) / sizeof(uint32_t);
            }
            else if (desc.MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
            {
                // This is a Structured Buffer
                srv_desc.Format = DXGI_FORMAT_UNKNOWN;
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srv_desc.Buffer.FirstElement = (UINT)offset / desc.StructureByteStride;
                srv_desc.Buffer.NumElements = Min((UINT)size, desc.ByteWidth - (UINT)offset) / desc.StructureByteStride;
                srv_desc.Buffer.StructureByteStride = desc.StructureByteStride;
            }
            else
            {
                // This is a Typed Buffer
                uint32_t stride = GetPixelFormatSize(desc.format);
                srv_desc.Format = PixelFormatToDXGIFormat(desc.format);
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srv_desc.Buffer.FirstElement = offset / stride;
                srv_desc.Buffer.NumElements = Min((UINT)size, desc.ByteWidth - (UINT)offset) / stride;
            }

            if (internal_state->srv.ViewDimension == D3D12_SRV_DIMENSION_UNKNOWN)
            {
                internal_state->srv = srv_desc;
                return -1;
            }
            internal_state->subresources_srv.push_back(srv_desc);
            return int(internal_state->subresources_srv.size() - 1);
        }
        break;
        case UAV:
        {

            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uav_desc.Buffer.FirstElement = 0;

            if (desc.MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
            {
                // This is a Raw Buffer
                uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
                uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
                uav_desc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
                uav_desc.Buffer.NumElements = Min((UINT)size, desc.ByteWidth - (UINT)offset) / sizeof(uint32_t);
            }
            else if (desc.MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
            {
                // This is a Structured Buffer
                uav_desc.Format = DXGI_FORMAT_UNKNOWN;
                uav_desc.Buffer.FirstElement = (UINT)offset / desc.StructureByteStride;
                uav_desc.Buffer.NumElements = Min((UINT)size, desc.ByteWidth - (UINT)offset) / desc.StructureByteStride;
                uav_desc.Buffer.StructureByteStride = desc.StructureByteStride;
            }
            else
            {
                // This is a Typed Buffer
                uint32_t stride = GetPixelFormatSize(desc.format);
                uav_desc.Format = PixelFormatToDXGIFormat(desc.format);
                uav_desc.Buffer.FirstElement = (UINT)offset / stride;
                uav_desc.Buffer.NumElements = Min((UINT)size, desc.ByteWidth - (UINT)offset) / stride;
            }

            if (internal_state->uav.ViewDimension == D3D12_UAV_DIMENSION_UNKNOWN)
            {
                internal_state->uav = uav_desc;
                return -1;
            }
            internal_state->subresources_uav.push_back(uav_desc);
            return int(internal_state->subresources_uav.size() - 1);
        }
        break;
        default:
            assert(0);
            break;
        }

        return -1;
    }

    void GraphicsDevice_DX12::WriteShadingRateValue(ShadingRate rate, void* dest)
    {
        D3D12_SHADING_RATE _rate = _ConvertShadingRate(rate);
        if (!features_6.AdditionalShadingRatesSupported)
        {
            _rate = Min(_rate, D3D12_SHADING_RATE_2X2);
        }

        *(uint8_t*)dest = _rate;
    }

    void GraphicsDevice_DX12::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest)
    {
        D3D12_RAYTRACING_INSTANCE_DESC* desc = (D3D12_RAYTRACING_INSTANCE_DESC*)dest;
        desc->AccelerationStructure = to_internal(&instance->bottomlevel)->resource->GetGPUVirtualAddress();
        memcpy(desc->Transform, &instance->transform, sizeof(desc->Transform));
        desc->InstanceID = instance->InstanceID;
        desc->InstanceMask = instance->InstanceMask;
        desc->InstanceContributionToHitGroupIndex = instance->InstanceContributionToHitGroupIndex;
        desc->Flags = instance->Flags;
    }

    void GraphicsDevice_DX12::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest)
    {
        auto internal_state = to_internal(rtpso);

        ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        HRESULT hr = internal_state->resource.As(&stateObjectProperties);
        assert(SUCCEEDED(hr));

        void* identifier = stateObjectProperties->GetShaderIdentifier(internal_state->group_strings[group_index].c_str());
        memcpy(dest, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }

    void GraphicsDevice_DX12::WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource, uint64_t offset)
    {
        auto table_internal = to_internal(table);
        D3D12_CPU_DESCRIPTOR_HANDLE dst = table_internal->resource_heap.address;
        size_t remap = table_internal->resource_heap.write_remap[rangeIndex];
        dst.ptr += (remap + arrayIndex) * (size_t)resource_descriptor_size;

        RESOURCEBINDING binding = table->resources[rangeIndex].binding;
        switch (binding)
        {
        case CONSTANTBUFFER:
            if (resource == nullptr || !resource->IsValid())
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
                device->CreateConstantBufferView(&cbv_desc, dst);
            }
            else if (resource->IsBuffer())
            {
                const GraphicsBuffer* buffer = (const GraphicsBuffer*)resource;
                auto internal_state = to_internal(buffer);
                if (buffer->GetDesc().BindFlags & BIND_CONSTANT_BUFFER)
                {
                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = internal_state->cbv;
                    cbv.BufferLocation += offset;
                    device->CreateConstantBufferView(&cbv, dst);
                }
            }
            break;
        case RAWBUFFER:
        case STRUCTUREDBUFFER:
        case TYPEDBUFFER:
        case TEXTURE1D:
        case TEXTURE1DARRAY:
        case TEXTURE2D:
        case TEXTURE2DARRAY:
        case TEXTURECUBE:
        case TEXTURECUBEARRAY:
        case TEXTURE3D:
        case ACCELERATIONSTRUCTURE:
            if (resource == nullptr || !resource->IsValid())
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                switch (binding)
                {
                case RAWBUFFER:
                case STRUCTUREDBUFFER:
                case TYPEDBUFFER:
                    srv_desc.Format = DXGI_FORMAT_R32_UINT;
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                    break;
                case TEXTURE1D:
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                    break;
                case TEXTURE1DARRAY:
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                    break;
                case TEXTURE2D:
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    break;
                case TEXTURE2DARRAY:
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    break;
                case TEXTURECUBE:
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    break;
                case TEXTURECUBEARRAY:
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                    break;
                case TEXTURE3D:
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                    break;
                case ACCELERATIONSTRUCTURE:
                    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                    break;
                default:
                    assert(0);
                    break;
                }
                device->CreateShaderResourceView(nullptr, &srv_desc, dst);
            }
            else if (resource->IsTexture())
            {
                auto internal_state = to_internal((const Texture*)resource);
                if (subresource < 0)
                {
                    device->CreateShaderResourceView(internal_state->resource.Get(), &internal_state->srv, dst);
                }
                else
                {
                    device->CreateShaderResourceView(internal_state->resource.Get(), &internal_state->subresources_srv[subresource], dst);
                }
            }
            else if (resource->IsBuffer())
            {
                const GraphicsBuffer* buffer = (const GraphicsBuffer*)resource;
                auto internal_state = to_internal(buffer);
                D3D12_SHADER_RESOURCE_VIEW_DESC srv = subresource < 0 ? internal_state->srv : internal_state->subresources_srv[subresource];
                switch (binding)
                {
                default:
                case RAWBUFFER:
                    srv.Buffer.FirstElement += offset / sizeof(uint32_t);
                    break;
                case STRUCTUREDBUFFER:
                    srv.Buffer.FirstElement += offset / srv.Buffer.StructureByteStride;
                    break;
                case TYPEDBUFFER:
                    srv.Buffer.FirstElement += offset / GetPixelFormatSize(buffer->GetDesc().format);
                    break;
                }
                device->CreateShaderResourceView(internal_state->resource.Get(), &srv, dst);
            }
            else if (resource->IsAccelerationStructure())
            {
                auto internal_state = to_internal((const RaytracingAccelerationStructure*)resource);
                device->CreateShaderResourceView(nullptr, &internal_state->srv, dst);
            }
            break;
        case RWRAWBUFFER:
        case RWSTRUCTUREDBUFFER:
        case RWTYPEDBUFFER:
        case RWTEXTURE1D:
        case RWTEXTURE1DARRAY:
        case RWTEXTURE2D:
        case RWTEXTURE2DARRAY:
        case RWTEXTURE3D:
            if (resource == nullptr || !resource->IsValid())
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                uav_desc.Format = DXGI_FORMAT_R32_UINT;
                switch (binding)
                {
                case RWRAWBUFFER:
                case RWSTRUCTUREDBUFFER:
                case RWTYPEDBUFFER:
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                    break;
                case RWTEXTURE1D:
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                    break;
                case RWTEXTURE1DARRAY:
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                    break;
                case RWTEXTURE2D:
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    break;
                case RWTEXTURE2DARRAY:
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    break;
                case RWTEXTURE3D:
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                    break;
                default:
                    assert(0);
                    break;
                }
                device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, dst);
            }
            else if (resource->IsTexture())
            {
                auto internal_state = to_internal((const Texture*)resource);
                if (subresource < 0)
                {
                    device->CreateUnorderedAccessView(internal_state->resource.Get(), nullptr, &internal_state->uav, dst);
                }
                else
                {
                    device->CreateUnorderedAccessView(internal_state->resource.Get(), nullptr, &internal_state->subresources_uav[subresource], dst);
                }
            }
            else if (resource->IsBuffer())
            {
                const GraphicsBuffer* buffer = (const GraphicsBuffer*)resource;
                auto internal_state = to_internal(buffer);
                D3D12_UNORDERED_ACCESS_VIEW_DESC uav = subresource < 0 ? internal_state->uav : internal_state->subresources_uav[subresource];
                switch (binding)
                {
                default:
                case RWRAWBUFFER:
                    uav.Buffer.FirstElement += offset / sizeof(uint32_t);
                    break;
                case RWSTRUCTUREDBUFFER:
                    uav.Buffer.FirstElement += offset / uav.Buffer.StructureByteStride;
                    break;
                case RWTYPEDBUFFER:
                    uav.Buffer.FirstElement += offset / GetPixelFormatSize(buffer->GetDesc().format);
                    break;
                }
                device->CreateUnorderedAccessView(internal_state->resource.Get(), nullptr, &uav, dst);
            }
            break;
        default:
            break;
        }
    }

    void GraphicsDevice_DX12::WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler)
    {
        auto table_internal = to_internal(table);
        D3D12_CPU_DESCRIPTOR_HANDLE dst = table_internal->sampler_heap.address;
        size_t remap = table_internal->sampler_heap.write_remap[rangeIndex];
        dst.ptr += (remap + arrayIndex) * (size_t)sampler_descriptor_size;

        if (sampler == nullptr)
        {
            D3D12_SAMPLER_DESC sam = {};
            sam.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            sam.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sam.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sam.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sam.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            device->CreateSampler(&sam, dst);
        }
        else
        {
            auto internal_state = to_internal(sampler);
            device->CreateSampler(&internal_state->descriptor, dst);
        }
    }

    void GraphicsDevice_DX12::Map(const GPUResource* resource, Mapping* mapping)
    {
        auto internal_state = to_internal(resource);
        D3D12_RANGE read_range = {};
        if (mapping->_flags & Mapping::FLAG_READ)
        {
            read_range.Begin = mapping->offset;
            read_range.End = mapping->size;
        }
        HRESULT hr = internal_state->resource->Map(0, &read_range, &mapping->data);
        if (SUCCEEDED(hr))
        {
            mapping->rowpitch = internal_state->footprint.Footprint.RowPitch;
        }
        else
        {
            assert(0);
            mapping->data = nullptr;
            mapping->rowpitch = 0;
        }
    }
    void GraphicsDevice_DX12::Unmap(const GPUResource* resource)
    {
        auto internal_state = to_internal(resource);
        internal_state->resource->Unmap(0, nullptr);
    }
    bool GraphicsDevice_DX12::QueryRead(const GPUQuery* query, GPUQueryResult* result)
    {
        auto internal_state = to_internal(query);

        D3D12_RANGE range;
        range.Begin = (size_t)internal_state->query_index * sizeof(size_t);
        range.End = range.Begin + sizeof(uint64_t);
        D3D12_RANGE nullrange = {};
        void* data = nullptr;

        switch (query->desc.Type)
        {
        case GPU_QUERY_TYPE_EVENT:
            assert(0); // not implemented yet
            break;
        case GPU_QUERY_TYPE_TIMESTAMP:
            querypool_timestamp_readback->Map(0, &range, &data);
            result->result_timestamp = *(uint64_t*)((size_t)data + range.Begin);
            querypool_timestamp_readback->Unmap(0, &nullrange);
            break;
        case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
            directQueue->GetTimestampFrequency(&result->result_timestamp_frequency);
            break;
        case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
        {
            BOOL passed = FALSE;
            querypool_occlusion_readback->Map(0, &range, &data);
            passed = *(BOOL*)((size_t)data + range.Begin);
            querypool_occlusion_readback->Unmap(0, &nullrange);
            result->result_passed_sample_count = (uint64_t)passed;
            break;
        }
        case GPU_QUERY_TYPE_OCCLUSION:
            querypool_occlusion_readback->Map(0, &range, &data);
            result->result_passed_sample_count = *(uint64_t*)((size_t)data + range.Begin);
            querypool_occlusion_readback->Unmap(0, &nullrange);
            break;
        }

        return true;
    }

    void GraphicsDevice_DX12::SetName(GPUResource* pResource, const char* name)
    {
        auto internal_state = to_internal(pResource);
        if (internal_state->resource != nullptr)
        {
            auto wName = ToUtf16(name);
            internal_state->resource->SetName(wName.c_str());
        }
    }

    /* CommandList */
    void GraphicsDevice_DX12::PresentBegin(ID3D12GraphicsCommandList6* commandList)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = backBuffers[backbufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        commandList->ResourceBarrier(1, &barrier);

        const float clearColor[] = { 0, 0, 0, 1 };
        device->CreateRenderTargetView(backBuffers[backbufferIndex], nullptr, rtv_descriptor_heap_start);

        D3D12_RENDER_PASS_RENDER_TARGET_DESC RTV = {};
        RTV.cpuDescriptor = rtv_descriptor_heap_start;
        RTV.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        RTV.BeginningAccess.Clear.ClearValue.Color[0] = clearColor[0];
        RTV.BeginningAccess.Clear.ClearValue.Color[1] = clearColor[1];
        RTV.BeginningAccess.Clear.ClearValue.Color[2] = clearColor[2];
        RTV.BeginningAccess.Clear.ClearValue.Color[3] = clearColor[3];
        RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        commandList->BeginRenderPass(1, &RTV, nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
    }

    void GraphicsDevice_DX12::PresentEnd(ID3D12GraphicsCommandList6* commandList)
    {
        commandList->EndRenderPass();

        // Indicate that the back buffer will now be used to present.
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = backBuffers[backbufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        commandList->ResourceBarrier(1, &barrier);

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

        backbufferIndex = (backbufferIndex + 1) % BACKBUFFER_COUNT;

        // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
        if (!dxgiFactory4->IsCurrent())
        {
            SafeRelease(dxgiFactory4);

            ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
        }
    }

    void D3D12_CommandList::Reset()
    {
        active_renderpass = nullptr;
        prev_pt = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        dirty_pso = false;
        active_pso = nullptr;
        active_rt = nullptr;
        active_rootsig_graphics = nullptr;
        active_cs = nullptr;
        active_rootsig_compute = nullptr;
        prev_shadingrate = D3D12_SHADING_RATE_1X1;
    }

    CommandList& GraphicsDevice_DX12::BeginCommandList()
    {
        std::atomic_uint32_t cmd = commandListsCount.fetch_add(1);
        ALIMER_ASSERT(cmd < kCommandListCount);

        if (commandLists[cmd] == nullptr)
        {
            commandLists[cmd] = new D3D12_CommandList();
            commandLists[cmd]->index = cmd;
            commandLists[cmd]->device = this;

            HRESULT hr;
            for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
            {
                hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandLists[cmd]->commandAllocators[fr]));
                frames[fr].descriptors[cmd].init(this);
                frames[fr].resourceBuffer[cmd].init(this, 1024 * 1024); // 1 MB starting size
            }

            hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandLists[cmd]->commandAllocators[0], nullptr, IID_PPV_ARGS(&commandLists[cmd]->handle));
            hr = commandLists[cmd]->handle->Close();

            std::wstringstream wss;
            wss << "CommandBuffer" << cmd;
            commandLists[cmd]->handle->SetName(wss.str().c_str());
        }

        // Start the command list in a default state:
        commandLists[cmd]->Reset();

        HRESULT hr = commandLists[cmd]->commandAllocators[GetFrameIndex()]->Reset();
        assert(SUCCEEDED(hr));
        hr = commandLists[cmd]->handle->Reset(commandLists[cmd]->commandAllocators[GetFrameIndex()], nullptr);
        assert(SUCCEEDED(hr));

        GetFrameResources().descriptors[cmd].reset();
        GetFrameResources().resourceBuffer[cmd].clear();

        D3D12_VIEWPORT vp = {};
        vp.Width = (float)backbufferWidth;
        vp.Height = (float)backbufferHeight;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        commandLists[cmd]->handle->RSSetViewports(1, &vp);

        D3D12_RECT pRects[8];
        for (uint32_t i = 0; i < 8; ++i)
        {
            pRects[i].bottom = INT32_MAX;
            pRects[i].left = INT32_MIN;
            pRects[i].right = INT32_MAX;
            pRects[i].top = INT32_MIN;
        }
        commandLists[cmd]->handle->RSSetScissorRects(8, pRects);
        

        if (VARIABLE_RATE_SHADING)
        {
            D3D12_SHADING_RATE_COMBINER combiners[] =
            {
                D3D12_SHADING_RATE_COMBINER_MAX,
                D3D12_SHADING_RATE_COMBINER_MAX,
            };
            commandLists[cmd]->handle->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
        }

        return *commandLists[cmd];
    }

    void GraphicsDevice_DX12::SubmitCommandLists()
    {
        // Sync up copy queue:
        copyQueueLock.lock();
        if (copyQueueUse)
        {
            copyQueueUse = false;
            auto& frame = GetFrameResources();
            frame.copyCommandList->Close();
            ID3D12CommandList* commandlists[] = {
                frame.copyCommandList
            };
            frame.copyQueue->ExecuteCommandLists(1, commandlists);

            // Signal and increment the fence value.
            HRESULT hr = frame.copyQueue->Signal(copyFence, FRAMECOUNT);
            assert(SUCCEEDED(hr));
            hr = frame.copyQueue->Wait(copyFence, FRAMECOUNT);
            assert(SUCCEEDED(hr));
        }

        // Execute deferred command lists:
        {
            ID3D12CommandList* cmdLists[kCommandListCount];
            uint32_t counter = 0;

            uint32_t cmd_last = commandListsCount.load();
            commandListsCount.store(0);
            for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
            {
                // Perform query resolves (must be outside of render pass):
                commandLists[cmd]->ResolveQueryData();

                HRESULT hr = commandLists[cmd]->handle->Close();
                assert(SUCCEEDED(hr));

                cmdLists[counter] = commandLists[cmd]->handle;
                counter++;
            }

            directQueue->ExecuteCommandLists(counter, cmdLists);
        }

        // This acts as a barrier, following this we will be using the next frame's resources when calling GetFrameResources()!
        HRESULT hr = directQueue->Signal(frameFence, ++FRAMECOUNT);

        // Determine the last frame that we should not wait on:
        uint64_t GPUFrameCount = frameFence->GetCompletedValue();

        // Wait if too many frames are being incomplete:
        if ((FRAMECOUNT - GPUFrameCount) >= BACKBUFFER_COUNT)
        {
            hr = frameFence->SetEventOnCompletion(GPUFrameCount + 1, frameFenceEvent);
            WaitForSingleObject(frameFenceEvent, INFINITE);
        }

        allocationhandler->Update(FRAMECOUNT, BACKBUFFER_COUNT);

        copyQueueLock.unlock();
    }

    void GraphicsDevice_DX12::WaitForGPU()
    {
        directQueue->Signal(frameFence, ++FRAMECOUNT);
        frameFence->SetEventOnCompletion(FRAMECOUNT, frameFenceEvent);
        WaitForSingleObject(frameFenceEvent, INFINITE);
    }

    void GraphicsDevice_DX12::ClearPipelineStateCache()
    {
    }

    /* D3D12_CommandList */
    void D3D12_CommandList::PresentBegin()
    {
        device->PresentBegin(handle);
    }

    void D3D12_CommandList::PresentEnd()
    {
        device->PresentEnd(handle);
    }

    void D3D12_CommandList::RenderPassBegin(const RenderPass* renderpass)
    {
        active_renderpass = renderpass;

        auto internal_state = to_internal(active_renderpass);
        if (internal_state->num_barriers_begin > 0)
        {
            handle->ResourceBarrier(internal_state->num_barriers_begin, internal_state->barrierdescs_begin);
        }

        const RenderPassDesc& desc = renderpass->GetDesc();

        D3D12_CPU_DESCRIPTOR_HANDLE descriptors_RTV = device->rtv_descriptor_heap_start;
        descriptors_RTV.ptr += device->rtv_descriptor_size * D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT * index;

        D3D12_CPU_DESCRIPTOR_HANDLE descriptors_DSV = device->dsv_descriptor_heap_start;
        descriptors_DSV.ptr += device->dsv_descriptor_size * index;

        uint32_t rt_count = 0;
        D3D12_RENDER_PASS_RENDER_TARGET_DESC RTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        bool dsv = false;
        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};
        int resolve_dst_counter = 0;
        for (auto& attachment : desc.attachments)
        {
            const Texture* texture = attachment.texture;
            int subresource = attachment.subresource;
            auto texture_internal = to_internal(texture);

            D3D12_CLEAR_VALUE clear_value;
            clear_value.Format = PixelFormatToDXGIFormat(texture->desc.format);

            if (attachment.type == RenderPassAttachment::RENDERTARGET)
            {
                RTVs[rt_count].cpuDescriptor = descriptors_RTV;
                RTVs[rt_count].cpuDescriptor.ptr += device->rtv_descriptor_size * rt_count;

                if (subresource < 0 || texture_internal->subresources_rtv.empty())
                {
                    device->device->CreateRenderTargetView(texture_internal->resource.Get(), &texture_internal->rtv, RTVs[rt_count].cpuDescriptor);
                }
                else
                {
                    assert(texture_internal->subresources_rtv.size() > size_t(subresource) && "Invalid RTV subresource!");
                    device->device->CreateRenderTargetView(texture_internal->resource.Get(), &texture_internal->subresources_rtv[subresource], RTVs[rt_count].cpuDescriptor);
                }

                switch (attachment.loadop)
                {
                default:
                case RenderPassAttachment::LOADOP_LOAD:
                    RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                    break;
                case RenderPassAttachment::LOADOP_CLEAR:
                    RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                    clear_value.Color[0] = texture->desc.clear.color[0];
                    clear_value.Color[1] = texture->desc.clear.color[1];
                    clear_value.Color[2] = texture->desc.clear.color[2];
                    clear_value.Color[3] = texture->desc.clear.color[3];
                    RTVs[rt_count].BeginningAccess.Clear.ClearValue = clear_value;
                    break;
                case RenderPassAttachment::LOADOP_DONTCARE:
                    RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                    break;
                }

                switch (attachment.storeop)
                {
                default:
                case RenderPassAttachment::STOREOP_STORE:
                    RTVs[rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                    break;
                case RenderPassAttachment::STOREOP_DONTCARE:
                    RTVs[rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                    break;
                }

                rt_count++;
            }
            else if (attachment.type == RenderPassAttachment::DEPTH_STENCIL)
            {
                dsv = true;

                DSV.cpuDescriptor = descriptors_DSV;

                if (subresource < 0 || texture_internal->subresources_dsv.empty())
                {
                    device->device->CreateDepthStencilView(texture_internal->resource.Get(), &texture_internal->dsv, DSV.cpuDescriptor);
                }
                else
                {
                    assert(texture_internal->subresources_dsv.size() > size_t(subresource) && "Invalid DSV subresource!");
                    device->device->CreateDepthStencilView(texture_internal->resource.Get(), &texture_internal->subresources_dsv[subresource], DSV.cpuDescriptor);
                }

                switch (attachment.loadop)
                {
                default:
                case RenderPassAttachment::LOADOP_LOAD:
                    DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                    DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                    break;
                case RenderPassAttachment::LOADOP_CLEAR:
                    DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                    DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                    clear_value.DepthStencil.Depth = texture->desc.clear.depthstencil.depth;
                    clear_value.DepthStencil.Stencil = texture->desc.clear.depthstencil.stencil;
                    DSV.DepthBeginningAccess.Clear.ClearValue = clear_value;
                    DSV.StencilBeginningAccess.Clear.ClearValue = clear_value;
                    break;
                case RenderPassAttachment::LOADOP_DONTCARE:
                    DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                    DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                    break;
                }

                switch (attachment.storeop)
                {
                default:
                case RenderPassAttachment::STOREOP_STORE:
                    DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                    DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                    break;
                case RenderPassAttachment::STOREOP_DONTCARE:
                    DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                    DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                    break;
                }
            }
            else if (attachment.type == RenderPassAttachment::RESOLVE)
            {
                if (texture != nullptr)
                {
                    int resolve_src_counter = 0;
                    for (auto& src : active_renderpass->desc.attachments)
                    {
                        if (src.type == RenderPassAttachment::RENDERTARGET && src.texture != nullptr)
                        {
                            if (resolve_src_counter == resolve_dst_counter)
                            {
                                auto src_internal = to_internal(src.texture);

                                D3D12_RENDER_PASS_RENDER_TARGET_DESC& src_RTV = RTVs[resolve_src_counter];
                                src_RTV.EndingAccess.Resolve.PreserveResolveSource = src_RTV.EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                                src_RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
                                src_RTV.EndingAccess.Resolve.Format = clear_value.Format;
                                src_RTV.EndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_AVERAGE;
                                src_RTV.EndingAccess.Resolve.SubresourceCount = 1;
                                src_RTV.EndingAccess.Resolve.pDstResource = texture_internal->resource.Get();
                                src_RTV.EndingAccess.Resolve.pSrcResource = src_internal->resource.Get();

                                // Due to a API bug, this resolve_subresources array must be kept alive between BeginRenderpass() and EndRenderpass()!
                                src_RTV.EndingAccess.Resolve.pSubresourceParameters = &resolve_subresources[resolve_src_counter];
                                resolve_subresources[resolve_src_counter].SrcRect.left = 0;
                                resolve_subresources[resolve_src_counter].SrcRect.right = (LONG)texture->desc.Width;
                                resolve_subresources[resolve_src_counter].SrcRect.bottom = (LONG)texture->desc.Height;
                                resolve_subresources[resolve_src_counter].SrcRect.top = 0;

                                break;
                            }
                            resolve_src_counter++;
                        }
                    }
                }
                resolve_dst_counter++;
            }

        }

        D3D12_RENDER_PASS_FLAGS flags = D3D12_RENDER_PASS_FLAG_NONE;
        if (desc._flags & RenderPassDesc::FLAG_ALLOW_UAV_WRITES)
        {
            flags &= D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
        }
        handle->BeginRenderPass(rt_count, RTVs, dsv ? &DSV : nullptr, flags);
    }

    void D3D12_CommandList::RenderPassEnd()
    {
        handle->EndRenderPass();

        auto internal_state = to_internal(active_renderpass);
        if (internal_state->num_barriers_end > 0)
        {
            handle->ResourceBarrier(internal_state->num_barriers_end, internal_state->barrierdescs_end);
        }

        active_renderpass = nullptr;
    }

    void D3D12_CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        viewports[0].TopLeftX = x;
        viewports[0].TopLeftY = y;
        viewports[0].Width = width;
        viewports[0].Height = height;
        viewports[0].MinDepth = minDepth;
        viewports[0].MaxDepth = maxDepth;
        handle->RSSetViewports(1, &viewports[0]);
    }

    void D3D12_CommandList::SetViewport(const Viewport& viewport)
    {
        viewports[0].TopLeftX = viewport.x;
        viewports[0].TopLeftY = viewport.y;
        viewports[0].Width = viewport.width;
        viewports[0].Height = viewport.height;
        viewports[0].MinDepth = viewport.minDepth;
        viewports[0].MaxDepth = viewport.maxDepth;
        handle->RSSetViewports(1, &viewports[0]);
    }

    void D3D12_CommandList::SetViewports(uint32_t viewportCount, const Viewport* pViewports)
    {
        ALIMER_ASSERT(pViewports != nullptr);
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

    void D3D12_CommandList::SetScissorRect(const ScissorRect& rect)
    {
        scissorRects[0].left = LONG(rect.x);
        scissorRects[0].top = LONG(rect.y);
        scissorRects[0].right = LONG(rect.x + rect.width);
        scissorRects[0].bottom = LONG(rect.y + rect.height);
        handle->RSSetScissorRects(1, &scissorRects[0]);
    }

    void D3D12_CommandList::SetScissorRects(uint32_t scissorCount, const ScissorRect* rects)
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

    void D3D12_CommandList::BindResource(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource)
    {
        assert(slot < GPU_RESOURCE_HEAP_SRV_COUNT);
        auto& descriptors = device->GetFrameResources().descriptors[index];
        if (descriptors.SRV[slot] != resource || descriptors.SRV_index[slot] != subresource)
        {
            descriptors.SRV[slot] = resource;
            descriptors.SRV_index[slot] = subresource;
            descriptors.dirty = true;
        }
    }

    void D3D12_CommandList::BindResources(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count)
    {
        if (resources != nullptr)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                BindResource(stage, resources[i], slot + i, -1);
            }
        }
    }

    void D3D12_CommandList::BindUAV(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource)
    {
        assert(slot < GPU_RESOURCE_HEAP_UAV_COUNT);
        auto& descriptors = device->GetFrameResources().descriptors[index];
        if (descriptors.UAV[slot] != resource || descriptors.UAV_index[slot] != subresource)
        {
            descriptors.UAV[slot] = resource;
            descriptors.UAV_index[slot] = subresource;
            descriptors.dirty = true;
        }
    }

    void D3D12_CommandList::BindUAVs(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count)
    {
        if (resources != nullptr)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                BindUAV(stage, resources[i], slot + i, -1);
            }
        }
    }

    void D3D12_CommandList::BindSampler(ShaderStage stage, const Sampler* sampler, uint32_t slot)
    {
        assert(slot < GPU_SAMPLER_HEAP_COUNT);
        auto& descriptors = device->GetFrameResources().descriptors[index];
        if (descriptors.SAM[slot] != sampler)
        {
            descriptors.SAM[slot] = sampler;
            descriptors.dirty = true;
        }
    }

    void D3D12_CommandList::BindConstantBuffer(ShaderStage stage, const GraphicsBuffer* buffer, uint32_t slot)
    {
        assert(slot < GPU_RESOURCE_HEAP_CBV_COUNT);
        auto& descriptors = device->GetFrameResources().descriptors[index];
        if (buffer->GetDesc().Usage == USAGE_DYNAMIC || descriptors.CBV[slot] != buffer)
        {
            descriptors.CBV[slot] = buffer;
            descriptors.dirty = true;
        }
    }

    void D3D12_CommandList::BindVertexBuffers(const GraphicsBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets)
    {
        assert(count <= 8);
        D3D12_VERTEX_BUFFER_VIEW res[8] = { 0 };
        for (uint32_t i = 0; i < count; ++i)
        {
            if (vertexBuffers[i] != nullptr)
            {
                res[i].BufferLocation = vertexBuffers[i] != nullptr ? to_internal(vertexBuffers[i])->resource->GetGPUVirtualAddress() : 0;
                res[i].SizeInBytes = vertexBuffers[i]->GetDesc().ByteWidth;
                if (offsets != nullptr)
                {
                    res[i].BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)offsets[i];
                    res[i].SizeInBytes -= offsets[i];
                }
                res[i].StrideInBytes = strides[i];
            }
        }

        handle->IASetVertexBuffers(slot, count, res);
    }

    void D3D12_CommandList::BindIndexBuffer(const GraphicsBuffer* indexBuffer, IndexFormat format, uint32_t offset)
    {
        D3D12_INDEX_BUFFER_VIEW view = {};
        if (indexBuffer != nullptr)
        {
            auto internal_state = to_internal(indexBuffer);

            view.BufferLocation = internal_state->resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
            view.Format = (format == IndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
            view.SizeInBytes = indexBuffer->GetDesc().ByteWidth;
        }

        handle->IASetIndexBuffer(&view);
    }

    void D3D12_CommandList::BindStencilRef(uint32_t value)
    {
        handle->OMSetStencilRef(value);
    }

    void D3D12_CommandList::BindBlendFactor(float r, float g, float b, float a)
    {
        const float blendFactor[4] = { r, g, b, a };
        handle->OMSetBlendFactor(blendFactor);
    }

    void D3D12_CommandList::BindShadingRate(ShadingRate rate)
    {
        D3D12_SHADING_RATE _rate = D3D12_SHADING_RATE_1X1;
        device->WriteShadingRateValue(rate, &_rate);

        if (device->VARIABLE_RATE_SHADING && prev_shadingrate != _rate)
        {
            prev_shadingrate = _rate;
            // Combiners are set to MAX by default in BeginCommandList
            handle->RSSetShadingRate(_rate, nullptr);
        }
    }

    void D3D12_CommandList::BindShadingRateImage(const Texture* texture)
    {
        if (device->VARIABLE_RATE_SHADING_TIER2)
        {
            if (texture == nullptr)
            {
                handle->RSSetShadingRateImage(nullptr);
            }
            else
            {
                ALIMER_ASSERT(texture->desc.format == PixelFormat::R8UInt);
                handle->RSSetShadingRateImage(to_internal(texture)->resource.Get());
            }
        }
    }

    void D3D12_CommandList::SetRenderPipeline(RenderPipeline* pipeline)
    {
        if (active_pso == pipeline)
            return;

        auto internal_state = to_internal(pipeline);
        handle->SetPipelineState(internal_state->handle.Get());

        if (prev_pt != internal_state->primitiveTopology)
        {
            prev_pt = internal_state->primitiveTopology;

            handle->IASetPrimitiveTopology(internal_state->primitiveTopology);
        }

        if (internal_state->desc.rootSignature == nullptr)
        {
            active_rootsig_graphics = nullptr;
            handle->SetGraphicsRootSignature(internal_state->rootSignature);
        }
        else if (active_pso != pipeline && active_rootsig_graphics != internal_state->desc.rootSignature)
        {
            active_rootsig_graphics = internal_state->desc.rootSignature;
            handle->SetGraphicsRootSignature(to_internal(internal_state->desc.rootSignature)->resource);
        }

        device->GetFrameResources().descriptors[index].dirty = true;
        active_pso = pipeline;
        dirty_pso = true;
    }

    void D3D12_CommandList::BindComputeShader(const Shader* shader)
    {
        ALIMER_ASSERT(shader->stage == ShaderStage::Compute);

        if (active_cs != shader)
        {
            device->GetFrameResources().descriptors[index].dirty = true;
            active_cs = shader;

            auto internal_state = to_internal(shader);
            handle->SetPipelineState(internal_state->handle.Get());

            if (shader->rootSignature == nullptr)
            {
                active_rootsig_compute = nullptr;
                handle->SetComputeRootSignature(internal_state->rootSignature);
            }
            else if (active_rootsig_compute != shader->rootSignature)
            {
                active_rootsig_compute = shader->rootSignature;
                handle->SetComputeRootSignature(to_internal(shader->rootSignature)->resource);
            }
        }
    }

    void D3D12_CommandList::PrepareDraw()
    {
        if (to_internal(active_pso)->desc.rootSignature == nullptr)
        {
            device->GetFrameResources().descriptors[index].validate(true, this);
        }
    }

    void D3D12_CommandList::PrepareDispatch()
    {
        if (active_cs->rootSignature == nullptr)
        {
            device->GetFrameResources().descriptors[index].validate(false, this);
        }
    }

    void D3D12_CommandList::PrepareRaytrace()
    {
        if (active_rt->desc.rootSignature == nullptr)
        {
            device->GetFrameResources().descriptors[index].validate(false, this);
        }
    }

    void D3D12_CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        PrepareDraw();
        handle->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void D3D12_CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) 
    {
        PrepareDraw();
        handle->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }

    void D3D12_CommandList::DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
        PrepareDraw();
        auto internal_state = to_internal(args);
        handle->ExecuteIndirect(device->drawInstancedIndirectCommandSignature, 1, internal_state->resource.Get(), args_offset, nullptr, 0);
    }

    void D3D12_CommandList::DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
        PrepareDraw();
        auto internal_state = to_internal(args);
        handle->ExecuteIndirect(device->drawIndexedInstancedIndirectCommandSignature, 1, internal_state->resource.Get(), args_offset, nullptr, 0);
    }

    GPUAllocation D3D12_CommandList::AllocateGPU(const uint32_t size)
    {
        ALIMER_ASSERT_MSG(size > 0, "Allocation size must be greater than zero");
        
        auto& allocator = device->GetFrameResources().resourceBuffer[index];
        uint8_t* dest = allocator.allocate(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        assert(dest != nullptr);

        GPUAllocation result{};
        result.buffer = allocator.buffer;
        result.offset = (uint32_t)allocator.calculateOffset(dest);
        result.data = (void*)dest;
        return result;
    }

    void D3D12_CommandList::UpdateBuffer(GraphicsBuffer* buffer, const void* data, uint64_t size)
    {
        const GPUBufferDesc& bufferDesc = buffer->GetDesc();
        assert(bufferDesc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
        assert(bufferDesc.ByteWidth >= size && "Data size is too big!");

        if (size == 0)
        {
            size = bufferDesc.ByteWidth;
        }
        else
        {
            size = Alimer::Min<uint64_t>(bufferDesc.ByteWidth, size);
        }

        if (bufferDesc.Usage == USAGE_DYNAMIC && bufferDesc.BindFlags & BIND_CONSTANT_BUFFER)
        {
            // Dynamic buffer will be used from host memory directly:
            auto internal_state = to_internal(buffer);
            GPUAllocation allocation = AllocateGPU(size);
            memcpy(allocation.data, data, size);
            internal_state->dynamic[index] = allocation;

            device->GetFrameResources().descriptors[index].dirty = true;
        }
        else
        {
            ALIMER_ASSERT(active_renderpass == nullptr);

            // Contents will be transferred to device memory:
            auto internal_state_src = to_internal(device->GetFrameResources().resourceBuffer[index].buffer.Get());
            auto internal_state_dst = to_internal(buffer);

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = internal_state_dst->resource.Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
            if (bufferDesc.BindFlags & BIND_CONSTANT_BUFFER || bufferDesc.BindFlags & BIND_VERTEX_BUFFER)
            {
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }
            else if (bufferDesc.BindFlags & BIND_INDEX_BUFFER)
            {
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            handle->ResourceBarrier(1, &barrier);

            uint8_t* dest = device->GetFrameResources().resourceBuffer[index].allocate(size, 1);
            memcpy(dest, data, size);
            handle->CopyBufferRegion(
                internal_state_dst->resource.Get(), 0,
                internal_state_src->resource.Get(), device->GetFrameResources().resourceBuffer[index].calculateOffset(dest),
                size
            );

            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
            handle->ResourceBarrier(1, &barrier);
        }
    }

    void D3D12_CommandList::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
    {
        PrepareDispatch();
        handle->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }

    void D3D12_CommandList::DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {
        PrepareDispatch();
        auto internal_state = to_internal(args);
        handle->ExecuteIndirect(device->dispatchIndirectCommandSignature, 1, internal_state->resource.Get(), args_offset, nullptr, 0);
    }

    void D3D12_CommandList::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
    {
        PrepareDraw();
        handle->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }

    void D3D12_CommandList::DispatchMeshIndirect(const GraphicsBuffer* args, uint32_t args_offset)
    {

        PrepareDraw();

        auto internal_state = to_internal(args);
        handle->ExecuteIndirect(device->dispatchMeshIndirectCommandSignature, 1, internal_state->resource.Get(), args_offset, nullptr, 0);
    }

    void D3D12_CommandList::CopyResource(const GPUResource* pDst, const GPUResource* pSrc)
    {
        auto internal_state_src = to_internal(pSrc);
        auto internal_state_dst = to_internal(pDst);
        D3D12_RESOURCE_DESC desc_src = internal_state_src->resource->GetDesc();
        D3D12_RESOURCE_DESC desc_dst = internal_state_dst->resource->GetDesc();
        if (desc_dst.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && desc_src.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), 0);
            CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), internal_state_src->footprint);
            handle->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
        }
        else if (desc_src.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && desc_dst.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), internal_state_dst->footprint);
            CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), 0);
            handle->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
        }
        else
        {
            handle->CopyResource(internal_state_dst->resource.Get(), internal_state_src->resource.Get());
        }
    }

    void D3D12_CommandList::ResolveQueryData()
    {
        for (auto& x : query_resolves)
        {
            switch (x.type)
            {
            case GPU_QUERY_TYPE_TIMESTAMP:
                handle->ResolveQueryData(device->querypool_timestamp, D3D12_QUERY_TYPE_TIMESTAMP, x.index, 1, device->querypool_timestamp_readback, (uint64_t)x.index * sizeof(uint64_t));
                break;
            case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
                handle->ResolveQueryData(device->querypool_occlusion, D3D12_QUERY_TYPE_BINARY_OCCLUSION, x.index, 1, device->querypool_occlusion_readback, (uint64_t)x.index * sizeof(uint64_t));
                break;
            case GPU_QUERY_TYPE_OCCLUSION:
                handle->ResolveQueryData(device->querypool_occlusion, D3D12_QUERY_TYPE_OCCLUSION, x.index, 1, device->querypool_occlusion_readback, (uint64_t)x.index * sizeof(uint64_t));
                break;
            }
        }
        query_resolves.clear();
    }

    void D3D12_CommandList::QueryBegin(const GPUQuery* query)
    {
        auto internal_state = to_internal(query);

        switch (query->desc.Type)
        {
        case GPU_QUERY_TYPE_TIMESTAMP:
            handle->BeginQuery(device->querypool_timestamp, D3D12_QUERY_TYPE_TIMESTAMP, internal_state->query_index);
            break;
        case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
            handle->BeginQuery(device->querypool_occlusion, D3D12_QUERY_TYPE_BINARY_OCCLUSION, internal_state->query_index);
            break;
        case GPU_QUERY_TYPE_OCCLUSION:
            handle->BeginQuery(device->querypool_occlusion, D3D12_QUERY_TYPE_OCCLUSION, internal_state->query_index);
            break;
        }
    }

    void D3D12_CommandList::QueryEnd(const GPUQuery* query)
    {
        auto internal_state = to_internal(query);

        switch (query->desc.Type)
        {
        case GPU_QUERY_TYPE_TIMESTAMP:
            handle->EndQuery(device->querypool_timestamp, D3D12_QUERY_TYPE_TIMESTAMP, internal_state->query_index);
            break;
        case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
            handle->EndQuery(device->querypool_occlusion, D3D12_QUERY_TYPE_BINARY_OCCLUSION, internal_state->query_index);
            break;
        case GPU_QUERY_TYPE_OCCLUSION:
            handle->EndQuery(device->querypool_occlusion, D3D12_QUERY_TYPE_OCCLUSION, internal_state->query_index);
            break;
        }

        query_resolves.emplace_back();
        Query_Resolve& resolver = query_resolves.back();
        resolver.type = query->desc.Type;
        resolver.index = internal_state->query_index;
    }

    void D3D12_CommandList::Barrier(const GPUBarrier* barriers, uint32_t numBarriers)
    {
        D3D12_RESOURCE_BARRIER barrierDescs[8];

        for (uint32_t i = 0; i < numBarriers; ++i)
        {
            const GPUBarrier& barrier = barriers[i];
            D3D12_RESOURCE_BARRIER& barrierDesc = barrierDescs[i];

            switch (barrier.type)
            {
            default:
            case GPUBarrier::MEMORY_BARRIER:
            {
                barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrierDesc.UAV.pResource = barrier.memory.resource == nullptr ? nullptr : to_internal(barrier.memory.resource)->resource.Get();
            }
            break;
            case GPUBarrier::IMAGE_BARRIER:
            {
                barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrierDesc.Transition.pResource = to_internal(barrier.image.texture)->resource.Get();
                barrierDesc.Transition.StateBefore = _ConvertImageLayout(barrier.image.layout_before);
                barrierDesc.Transition.StateAfter = _ConvertImageLayout(barrier.image.layout_after);
                barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            }
            break;
            case GPUBarrier::BUFFER_BARRIER:
            {
                barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrierDesc.Transition.pResource = to_internal(barrier.buffer.buffer)->resource.Get();
                barrierDesc.Transition.StateBefore = _ConvertBufferState(barrier.buffer.state_before);
                barrierDesc.Transition.StateAfter = _ConvertBufferState(barrier.buffer.state_after);
                barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            }
            break;
            }
        }

        handle->ResourceBarrier(numBarriers, barrierDescs);
    }

    void D3D12_CommandList::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, const RaytracingAccelerationStructure* src)
    {
        auto dst_internal = to_internal(dst);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
        desc.Inputs = dst_internal->desc;

        // Make a copy of geometries, don't overwrite internal_state (thread safety)
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
        geometries = dst_internal->geometries;
        desc.Inputs.pGeometryDescs = geometries.data();

        // The real GPU addresses get filled here:
        switch (dst->desc.type)
        {
        case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
        {
            size_t i = 0;
            for (auto& x : dst->desc.bottomlevel.geometries)
            {
                auto& geometry = geometries[i++];
                if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE)
                {
                    geometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                }
                if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
                {
                    geometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
                }

                if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
                {
                    geometry.Triangles.VertexBuffer.StartAddress = to_internal(x.triangles.vertexBuffer)->resource->GetGPUVirtualAddress() +
                        (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertexByteOffset;
                    geometry.Triangles.IndexBuffer = to_internal(x.triangles.indexBuffer)->resource->GetGPUVirtualAddress() +
                        (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.indexOffset * (x.triangles.indexFormat == IndexFormat::UInt16 ? sizeof(uint16_t) : sizeof(uint32_t));

                    if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
                    {
                        geometry.Triangles.Transform3x4 = to_internal(x.triangles.transform3x4Buffer)->resource->GetGPUVirtualAddress() +
                            (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform3x4BufferOffset;
                    }
                }
                else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
                {
                    geometry.AABBs.AABBs.StartAddress = to_internal(x.aabbs.aabbBuffer)->resource->GetGPUVirtualAddress() +
                        (D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
                }
            }
        }
        break;
        case RaytracingAccelerationStructureDesc::TOPLEVEL:
        {
            desc.Inputs.InstanceDescs = to_internal(dst->desc.toplevel.instanceBuffer)->resource->GetGPUVirtualAddress() +
                (D3D12_GPU_VIRTUAL_ADDRESS)dst->desc.toplevel.offset;
        }
        break;
        }

        if (src != nullptr)
        {
            desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

            auto src_internal = to_internal(src);
            desc.SourceAccelerationStructureData = src_internal->resource->GetGPUVirtualAddress();
        }
        desc.DestAccelerationStructureData = dst_internal->resource->GetGPUVirtualAddress();
        desc.ScratchAccelerationStructureData = to_internal(dst_internal->scratch)->resource->GetGPUVirtualAddress();
        handle->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
    }

    void D3D12_CommandList::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso)
    {
        if (rtpso != active_rt)
        {
            active_rt = rtpso;
            device->GetFrameResources().descriptors[index].dirty = true;

            auto internal_state = to_internal(rtpso);
            handle->SetPipelineState1(internal_state->resource.Get());

            if (rtpso->desc.rootSignature == nullptr)
            {
                // we just take the first shader (todo: better)
                active_cs = rtpso->desc.shaderlibraries.front().shader;
                active_rootsig_compute = nullptr;
                handle->SetComputeRootSignature(to_internal(active_cs)->rootSignature);
            }
            else if (active_rootsig_compute != rtpso->desc.rootSignature)
            {
                active_rootsig_compute = rtpso->desc.rootSignature;
                handle->SetComputeRootSignature(to_internal(rtpso->desc.rootSignature)->resource);
            }
        }
    }

    void D3D12_CommandList::DispatchRays(const DispatchRaysDesc* desc)
    {
        PrepareRaytrace();

        D3D12_DISPATCH_RAYS_DESC dispatchrays_desc = {};

        dispatchrays_desc.Width = desc->Width;
        dispatchrays_desc.Height = desc->Height;
        dispatchrays_desc.Depth = desc->Depth;

        if (desc->raygeneration.buffer != nullptr)
        {
            dispatchrays_desc.RayGenerationShaderRecord.StartAddress =
                to_internal(desc->raygeneration.buffer)->resource->GetGPUVirtualAddress() +
                (D3D12_GPU_VIRTUAL_ADDRESS)desc->raygeneration.offset;
            dispatchrays_desc.RayGenerationShaderRecord.SizeInBytes =
                desc->raygeneration.size;
        }

        if (desc->miss.buffer != nullptr)
        {
            dispatchrays_desc.MissShaderTable.StartAddress =
                to_internal(desc->miss.buffer)->resource->GetGPUVirtualAddress() +
                (D3D12_GPU_VIRTUAL_ADDRESS)desc->miss.offset;
            dispatchrays_desc.MissShaderTable.SizeInBytes =
                desc->miss.size;
            dispatchrays_desc.MissShaderTable.StrideInBytes =
                desc->miss.stride;
        }

        if (desc->hitgroup.buffer != nullptr)
        {
            dispatchrays_desc.HitGroupTable.StartAddress =
                to_internal(desc->hitgroup.buffer)->resource->GetGPUVirtualAddress() +
                (D3D12_GPU_VIRTUAL_ADDRESS)desc->hitgroup.offset;
            dispatchrays_desc.HitGroupTable.SizeInBytes =
                desc->hitgroup.size;
            dispatchrays_desc.HitGroupTable.StrideInBytes =
                desc->hitgroup.stride;
        }

        if (desc->callable.buffer != nullptr)
        {
            dispatchrays_desc.CallableShaderTable.StartAddress =
                to_internal(desc->callable.buffer)->resource->GetGPUVirtualAddress() +
                (D3D12_GPU_VIRTUAL_ADDRESS)desc->callable.offset;
            dispatchrays_desc.CallableShaderTable.SizeInBytes =
                desc->callable.size;
            dispatchrays_desc.CallableShaderTable.StrideInBytes =
                desc->callable.stride;
        }

        handle->DispatchRays(&dispatchrays_desc);
    }

    void D3D12_CommandList::BindDescriptorTable(PipelineBindPoint bindPoint, uint32_t space, const DescriptorTable* table)
    {
        const RootSignature* rootsig = nullptr;
        switch (bindPoint)
        {
        default:
        case PipelineBindPoint::Graphics:
            rootsig = to_internal(active_pso)->desc.rootSignature;
            break;
        case PipelineBindPoint::Compute:
            rootsig = active_cs->rootSignature;
            break;
        case PipelineBindPoint::Raytracing:
            rootsig = active_rt->desc.rootSignature;
            break;
        }

        auto rootsig_internal = to_internal(rootsig);
        uint32_t bind_point_remap = rootsig_internal->table_bind_point_remap[space];
        auto& descriptors = device->GetFrameResources().descriptors[index];
        auto handles = descriptors.commit(table, this);
        if (handles.resource_handle.ptr != 0)
        {
            switch (bindPoint)
            {
            default:
            case PipelineBindPoint::Graphics:
                handle->SetGraphicsRootDescriptorTable(bind_point_remap, handles.resource_handle);
                break;
            case PipelineBindPoint::Compute:
            case PipelineBindPoint::Raytracing:
                handle->SetComputeRootDescriptorTable(bind_point_remap, handles.resource_handle);
                break;
            }
            bind_point_remap++;
        }
        if (handles.sampler_handle.ptr != 0)
        {
            switch (bindPoint)
            {
            default:
            case PipelineBindPoint::Graphics:
                handle->SetGraphicsRootDescriptorTable(bind_point_remap, handles.sampler_handle);
                break;
            case PipelineBindPoint::Compute:
            case PipelineBindPoint::Raytracing:
                handle->SetComputeRootDescriptorTable(bind_point_remap, handles.sampler_handle);
                break;
            }
        }
    }

    void D3D12_CommandList::BindRootDescriptor(PipelineBindPoint bindPoint, uint32_t index, const GraphicsBuffer* buffer, uint32_t offset)
    {
        const RootSignature* rootsig = nullptr;
        switch (bindPoint)
        {
        default:
        case PipelineBindPoint::Graphics:
            rootsig = to_internal(active_pso)->desc.rootSignature;
            break;
        case PipelineBindPoint::Compute:
            rootsig = active_cs->rootSignature;
            break;
        case PipelineBindPoint::Raytracing:
            rootsig = active_rt->desc.rootSignature;
            break;
        }
        auto rootsig_internal = to_internal(rootsig);
        auto internal_state = to_internal(buffer);
        D3D12_GPU_VIRTUAL_ADDRESS address = internal_state->resource.Get()->GetGPUVirtualAddress() + (UINT64)offset;

        auto remap = rootsig_internal->root_remap[index];
        auto binding = rootsig->tables[remap.space].resources[remap.rangeIndex].binding;
        switch (binding)
        {
        case ROOT_CONSTANTBUFFER:
            switch (bindPoint)
            {
            default:
            case PipelineBindPoint::Graphics:
                handle->SetGraphicsRootConstantBufferView(index, address);
                break;
            case PipelineBindPoint::Compute:
            case PipelineBindPoint::Raytracing:
                handle->SetComputeRootConstantBufferView(index, address);
                break;
            }
            break;
        case ROOT_RAWBUFFER:
        case ROOT_STRUCTUREDBUFFER:
            switch (bindPoint)
            {
            default:
            case PipelineBindPoint::Graphics:
                handle->SetGraphicsRootShaderResourceView(index, address);
                break;
            case PipelineBindPoint::Compute:
            case PipelineBindPoint::Raytracing:
                handle->SetComputeRootShaderResourceView(index, address);
                break;
            }
            break;
        case ROOT_RWRAWBUFFER:
        case ROOT_RWSTRUCTUREDBUFFER:
            switch (bindPoint)
            {
            default:
            case PipelineBindPoint::Graphics:
                handle->SetGraphicsRootUnorderedAccessView(index, address);
                break;
            case PipelineBindPoint::Compute:
            case PipelineBindPoint::Raytracing:
                handle->SetComputeRootUnorderedAccessView(index, address);
                break;
            }
            break;
        default:
            break;
        }
    }

    void D3D12_CommandList::BindRootConstants(PipelineBindPoint bindPoint, uint32_t index, const void* srcData)
    {
        const RootSignature* rootsig = nullptr;
        switch (bindPoint)
        {
        default:
        case PipelineBindPoint::Graphics:
            rootsig = to_internal(active_pso)->desc.rootSignature;
            break;
        case PipelineBindPoint::Compute:
            rootsig = active_cs->rootSignature;
            break;
        case PipelineBindPoint::Raytracing:
            rootsig = active_rt->desc.rootSignature;
            break;
        }
        auto rootsig_internal = to_internal(rootsig);
        const RootConstantRange& range = rootsig->rootconstants[index];

        switch (bindPoint)
        {
        default:
        case PipelineBindPoint::Graphics:
            handle->SetGraphicsRoot32BitConstants(
                rootsig_internal->root_constant_bind_remap + index,
                range.size / sizeof(uint32_t),
                srcData,
                range.offset / sizeof(uint32_t)
            );
            break;
        case PipelineBindPoint::Compute:
        case PipelineBindPoint::Raytracing:
            handle->SetComputeRoot32BitConstants(
                rootsig_internal->root_constant_bind_remap + index,
                range.size / sizeof(uint32_t),
                srcData,
                range.offset / sizeof(uint32_t)
            );
            break;
        }
    }

    void D3D12_CommandList::PushDebugGroup(const char* name)
    {
        PIXBeginEvent(handle, PIX_COLOR_DEFAULT, name);
    }

    void D3D12_CommandList::PopDebugGroup()
    {
        PIXEndEvent(handle);
    }

    void D3D12_CommandList::InsertDebugMarker(const char* name)
    {
        PIXSetMarker(handle, PIX_COLOR_DEFAULT, name);
    }

}
