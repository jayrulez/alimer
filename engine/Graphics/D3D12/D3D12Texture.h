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

#include "Graphics/Texture.h"
#include "D3D12Backend.h"

namespace Alimer
{
    class D3D12Texture final : public Texture
    {
    public:
        D3D12Texture(D3D12GraphicsDevice* device, ID3D12Resource* resource, D3D12_RESOURCE_STATES state);
        D3D12Texture(D3D12GraphicsDevice* device, const TextureDescription& desc, const void* initialData);
        ~D3D12Texture() override;
        void Destroy() override;

        void TransitionBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState);

        void UploadTextureData(const void* initData);
        void UploadTextureData(const void* initData, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* uploadResource, void* uploadCPUMem, uint64_t resourceOffset);

        D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return SRV; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return RTV; }

    private:
        void BackendSetName() override;

        D3D12GraphicsDevice* device;
        D3D12_RESOURCE_STATES state{ D3D12_RESOURCE_STATE_COMMON };
        D3D12_CPU_DESCRIPTOR_HANDLE SRV{};
        D3D12_CPU_DESCRIPTOR_HANDLE RTV{};
        UINT64 sizeInBytes{ 0 };
    };
}
