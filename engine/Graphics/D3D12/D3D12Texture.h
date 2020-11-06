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
// The implementation is based on WickedEngine graphics code, MIT license
// (https://github.com/turanszkij/WickedEngine/blob/master/LICENSE.md)

#pragma once

#include "D3D12Backend.h"
#include "Graphics/Texture.h"

namespace alimer
{
    class D3D12Texture final : public Texture
    {
        friend class D3D12GraphicsDevice;
        friend class D3D12_CommandList;

    public:
        D3D12Texture(D3D12GraphicsDevice* device, ID3D12Resource* resource);
        D3D12Texture(D3D12GraphicsDevice* device, const TextureDescription& description);
        ~D3D12Texture() override;

        void Destroy() override;
        bool Init(bool hasInitData);
        inline ID3D12Resource* GetResource() const
        {
            return resource;
        }

    private:
        D3D12GraphicsDevice* device;

        ID3D12Resource* resource = nullptr;
        D3D12MA::Allocation* allocation = nullptr;
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
        D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
        std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> subresources_srv;
        std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> subresources_uav;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;

        GPUAllocation dynamic[kCommandListCount];
        D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
        std::vector<D3D12_RENDER_TARGET_VIEW_DESC> subresources_rtv;
        std::vector<D3D12_DEPTH_STENCIL_VIEW_DESC> subresources_dsv;
    };
}
