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

#include "Core/Log.h"
#include "D3D11GPUTexture.h"
#include "D3D11GPUDevice.h"

namespace alimer
{
    namespace
    {
        GPUTextureDescriptor Convert2DDesc(ID3D11Texture2D* texture, PixelFormat format)
        {
            D3D11_TEXTURE2D_DESC d3dDesc;
            texture->GetDesc(&d3dDesc);

            return GPUTextureDescriptor::New2D(d3dDesc.Width, d3dDesc.Height, format, d3dDesc.MipLevels > 1, D3D11GetTextureUsage(d3dDesc.BindFlags));
        }
    }

    D3D11GPUTexture::D3D11GPUTexture(D3D11GPUDevice* device, ID3D11Texture2D* externalTexture, PixelFormat format)
        : GPUTexture(Convert2DDesc(externalTexture, format))
        , device{ device }
        , handle(externalTexture)
    {
        handle->AddRef();
    }

    D3D11GPUTexture::D3D11GPUTexture(D3D11GPUDevice* device, const GPUTextureDescriptor& descriptor)
        : GPUTexture(descriptor)
        , device{ device }
    {
        D3D11_TEXTURE2D_DESC d3dDesc;
        d3dDesc.Width = width;
        d3dDesc.Height = height;
        d3dDesc.MipLevels = 1u;
        d3dDesc.ArraySize = 1u;
        d3dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        d3dDesc.SampleDesc.Count = 1;
        d3dDesc.SampleDesc.Quality = 0;
        d3dDesc.Usage = D3D11_USAGE_DEFAULT;
        d3dDesc.BindFlags = 0;
        d3dDesc.CPUAccessFlags = 0;
        d3dDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        D3D11_SUBRESOURCE_DATA initialData = {};
        /*if (data != nullptr) {
            initialData.pSysMem = data;
            initialDataPtr = &initialData;
        }*/

        HRESULT hr = device->GetD3DDevice()->CreateTexture2D(&d3dDesc, initialDataPtr, reinterpret_cast<ID3D11Texture2D**>(&handle));
        if (FAILED(hr)) {
            LOGE("Direct3D11: Failed to create 2D texture");
            return;
        }
    }

    D3D11GPUTexture::~D3D11GPUTexture()
    {
        Destroy();
    }

    void D3D11GPUTexture::Destroy()
    {
        srvs.clear();
        uavs.clear();
        rtvs.clear();
        dsvs.clear();
        SafeRelease(handle);
    }

    void D3D11GPUTexture::BackendSetName()
    {
        D3D11SetObjectName(handle, name);
    }

    ID3D11ShaderResourceView* D3D11GPUTexture::GetSRV(DXGI_FORMAT format, uint32_t level, uint32_t slice)
    {
        // For non-array textures force slice to 0.
        if (GetArrayLayers() <= 1)
            slice = 0;

        const uint32_t subresource = GetSubresourceIndex(level, slice);

        // Already created?
        if (srvs.size() > subresource) {
            return srvs[subresource].Get();
        }

        D3D11_RESOURCE_DIMENSION type;
        handle->GetType(&type);

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.Format = format;

        switch (type)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            ((ID3D11Texture1D*)handle)->GetDesc(&desc);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray.MostDetailedMip = level;
                viewDesc.Texture1DArray.MipLevels = desc.MipLevels;

                if (slice > 0)
                {
                    viewDesc.Texture1DArray.FirstArraySlice = slice;
                    viewDesc.Texture1DArray.ArraySize = 1;
                }
                else
                {
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D.MostDetailedMip = level;
                viewDesc.Texture1D.MipLevels = desc.MipLevels;
            }
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            ((ID3D11Texture2D*)handle)->GetDesc(&desc);

            if (desc.SampleDesc.Count > 1)
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = slice;
                        viewDesc.Texture2DMSArray.ArraySize = 1;
                    }
                    else
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = 0;
                        viewDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.MostDetailedMip = level;
                    viewDesc.Texture2DArray.MipLevels = desc.MipLevels;

                    if (slice > 0)
                    {
                        viewDesc.Texture2DArray.ArraySize = 1;
                        viewDesc.Texture2DArray.FirstArraySlice = slice;
                    }
                    else
                    {
                        viewDesc.Texture2DArray.FirstArraySlice = 0;
                        viewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MostDetailedMip = level;
                    viewDesc.Texture2D.MipLevels = desc.MipLevels;
                }
            }

            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC desc = {};
            ((ID3D11Texture3D*)handle)->GetDesc(&desc);

            viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            viewDesc.Texture3D.MostDetailedMip = level;
            viewDesc.Texture3D.MipLevels = desc.MipLevels;
            break;
        }

        default:
            break;
        }

        RefPtr<ID3D11ShaderResourceView> srv;
        ThrowIfFailed(device->GetD3DDevice()->CreateShaderResourceView(handle, &viewDesc, srv.GetAddressOf()));
        srvs.push_back(srv);
        return srv.Get();
    }

    ID3D11UnorderedAccessView* D3D11GPUTexture::GetUAV(DXGI_FORMAT format, uint32_t level, uint32_t slice)
    {
        // For non-array textures force slice to 0.
        if (GetArrayLayers() <= 1)
            slice = 0;

        const uint32_t subresource = GetSubresourceIndex(level, slice);

        // Already created?
        if (uavs.size() > subresource) {
            return uavs[subresource].Get();
        }

        D3D11_RESOURCE_DIMENSION type;
        handle->GetType(&type);

        D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
        viewDesc.Format = format;

        switch (type)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            ((ID3D11Texture1D*)handle)->GetDesc(&desc);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray.MipSlice = level;

                if (slice > 0)
                {
                    viewDesc.Texture1DArray.FirstArraySlice = slice;
                    viewDesc.Texture1DArray.ArraySize = 1;
                }
                else
                {
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D.MipSlice = level;
            }
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            ((ID3D11Texture2D*)handle)->GetDesc(&desc);

            // UAV cannot be created from multisample texture.
            ALIMER_ASSERT(desc.SampleDesc.Count == 1);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                viewDesc.Texture2DArray.MipSlice = level;

                if (slice > 0)
                {
                    viewDesc.Texture2DArray.ArraySize = 1;
                    viewDesc.Texture2DArray.FirstArraySlice = slice;
                }
                else
                {
                    viewDesc.Texture2DArray.FirstArraySlice = 0;
                    viewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                viewDesc.Texture2D.MipSlice = level;
            }

            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC desc = {};
            ((ID3D11Texture3D*)handle)->GetDesc(&desc);

            viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            viewDesc.Texture3D.MipSlice = level;

            if (slice > 0)
            {
                viewDesc.Texture3D.FirstWSlice = slice;
                viewDesc.Texture3D.WSize = 1;
            }
            else
            {
                viewDesc.Texture3D.FirstWSlice = 0;
                viewDesc.Texture3D.WSize = desc.Depth; /* A value of -1 indicates all of the slices along the w axis, starting from FirstWSlice. */
            }
            break;
        }

        default:
            break;
        }

        RefPtr<ID3D11UnorderedAccessView> uav;
        ThrowIfFailed(device->GetD3DDevice()->CreateUnorderedAccessView(handle, &viewDesc, uav.GetAddressOf()));
        uavs.push_back(uav);
        return uav.Get();
    }

    ID3D11RenderTargetView* D3D11GPUTexture::GetRTV(DXGI_FORMAT format, uint32_t level, uint32_t slice)
    {
        // For non-array textures force slice to 0.
        if (GetArrayLayers() <= 1)
            slice = 0;

        const uint32_t subresource = GetSubresourceIndex(level, slice);

        // Already created?
        if (rtvs.size() > subresource) {
            return rtvs[subresource].Get();
        }

        D3D11_RESOURCE_DIMENSION type;
        handle->GetType(&type);

        D3D11_RENDER_TARGET_VIEW_DESC viewDesc = {};
        viewDesc.Format = format;

        switch (type)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            ((ID3D11Texture1D*)handle)->GetDesc(&desc);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray.MipSlice = level;
                if (slice > 0)
                {
                    viewDesc.Texture1DArray.FirstArraySlice = slice;
                    viewDesc.Texture1DArray.ArraySize = 1;
                }
                else
                {
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D.MipSlice = level;
            }
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            ((ID3D11Texture2D*)handle)->GetDesc(&desc);

            if (desc.SampleDesc.Count > 1)
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = slice;
                        viewDesc.Texture2DMSArray.ArraySize = 1;
                    }
                    else
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = 0;
                        viewDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.MipSlice = level;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DArray.ArraySize = 1;
                        viewDesc.Texture2DArray.FirstArraySlice = slice;
                    }
                    else
                    {
                        viewDesc.Texture2DArray.FirstArraySlice = 0;
                        viewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MipSlice = level;
                }
            }

            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC desc = {};
            ((ID3D11Texture3D*)handle)->GetDesc(&desc);

            viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            viewDesc.Texture3D.MipSlice = level;

            if (slice > 0)
            {
                viewDesc.Texture3D.FirstWSlice = slice;
                viewDesc.Texture3D.WSize = 1;
            }
            else
            {
                viewDesc.Texture3D.FirstWSlice = 0;
                viewDesc.Texture3D.WSize = desc.Depth; /* A value of -1 indicates all of the slices along the w axis, starting from FirstWSlice. */
            }
            break;
        }

        default:
            break;
        }

        RefPtr<ID3D11RenderTargetView> rtv;
        ThrowIfFailed(device->GetD3DDevice()->CreateRenderTargetView(handle, &viewDesc, rtv.GetAddressOf()));
        rtvs.push_back(rtv);
        return rtv.Get();
    }

    ID3D11DepthStencilView* D3D11GPUTexture::GetDSV(DXGI_FORMAT format, uint32_t level, uint32_t slice)
    {
        // For non-array textures force slice to 0.
        if (GetArrayLayers() <= 1)
            slice = 0;

        const uint32_t subresource = GetSubresourceIndex(level, slice);

        // Already created?
        if (dsvs.size() > subresource) {
            return dsvs[subresource].Get();
        }

        D3D11_RESOURCE_DIMENSION type;
        handle->GetType(&type);

        D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
        viewDesc.Format = format;
        viewDesc.Flags = 0; // TODO: Handle ReadOnlyDepth and ReadOnlyStencil D3D11_DSV_READ_ONLY_DEPTH

        switch (type)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            ((ID3D11Texture1D*)handle)->GetDesc(&desc);

            if (desc.ArraySize > 1)
            {
                viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray.MipSlice = level;
                if (slice > 0)
                {
                    viewDesc.Texture1DArray.FirstArraySlice = slice;
                    viewDesc.Texture1DArray.ArraySize = 1;
                }
                else
                {
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.ArraySize = desc.ArraySize;
                }
            }
            else
            {
                viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D.MipSlice = level;
            }
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            ((ID3D11Texture2D*)handle)->GetDesc(&desc);

            if (desc.SampleDesc.Count > 1)
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = slice;
                        viewDesc.Texture2DMSArray.ArraySize = 1;
                    }
                    else
                    {
                        viewDesc.Texture2DMSArray.FirstArraySlice = 0;
                        viewDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
            }
            else
            {
                if (desc.ArraySize > 1)
                {
                    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.MipSlice = level;
                    if (slice > 0)
                    {
                        viewDesc.Texture2DArray.ArraySize = 1;
                        viewDesc.Texture2DArray.FirstArraySlice = slice;
                    }
                    else
                    {
                        viewDesc.Texture2DArray.FirstArraySlice = 0;
                        viewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                    }
                }
                else
                {
                    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MipSlice = level;
                }
            }

            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            ALIMER_VERIFY_MSG(false, "Cannot create 3D Depth Stencil");
            ALIMER_DEBUG_BREAK();
            break;

        default:
            break;
        }

        RefPtr<ID3D11DepthStencilView> view;
        ThrowIfFailed(device->GetD3DDevice()->CreateDepthStencilView(handle, &viewDesc, view.GetAddressOf()));
        dsvs.push_back(view);
        return view.Get();
    }
}
