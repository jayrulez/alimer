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

#include "graphics/Texture.h"
#include "D3D12Backend.h"
#include <unordered_map>

namespace alimer
{
    class D3D12Texture final : public Texture, public D3D12GpuResource
    {
    public:
        D3D12Texture(D3D12GraphicsDevice* device, const TextureDescription& desc, const void* initialData);
        D3D12Texture(D3D12GraphicsDevice* device, const TextureDescription& desc, ID3D12Resource* resource_, D3D12_RESOURCE_STATES currentState);
        ~D3D12Texture() override;
        void Destroy() override;

        static D3D12Texture* CreateFromExternal(D3D12GraphicsDevice* device, ID3D12Resource* resource, PixelFormat format, D3D12_RESOURCE_STATES currentState);
        D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(uint32_t mipLevel, uint32_t slice);

        DXGI_FORMAT GetDXGIFormat() const { return dxgiFormat; }

    private:
        D3D12MA::Allocation* allocation = nullptr;
        DXGI_FORMAT dxgiFormat;

        struct RTVInfo
        {
            uint32_t level;
            uint32_t slice;

            bool operator==(const RTVInfo& other) const
            {
                return (level == other.level)
                    && (slice == other.slice)
                    ;
            }
        };

        struct RTVInfoHashFunc
        {
            std::size_t operator()(const RTVInfo& value) const
            {
                return ((std::hash<uint32_t>()(value.level) ^ (std::hash<uint32_t>()(value.slice) << 1)) >> 1);
            }
        };

        std::unordered_map<RTVInfo, D3D12_CPU_DESCRIPTOR_HANDLE, RTVInfoHashFunc> rtvs;
    };
}
