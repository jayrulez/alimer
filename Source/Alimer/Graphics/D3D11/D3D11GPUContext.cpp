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
#include "D3D11GPUContext.h"
#include "D3D11GPUTexture.h"
#include "D3D11GPUDevice.h"

namespace alimer
{
    D3D11GPUContext::D3D11GPUContext(D3D11GPUDevice* device_, ID3D11DeviceContext1* context, const GPUContextDescription& desc, bool isMain_)
        : GPUContext(desc.width, desc.height, isMain_)
        , device(device_)
        , handle(context)
    {
        ThrowIfFailed(context->QueryInterface(IID_PPV_ARGS(&annotation)));

        if (desc.handle.window)
        {
            window = desc.handle.window;
        }
    }

    D3D11GPUContext::~D3D11GPUContext()
    {
        SafeRelease(annotation);
        SafeRelease(handle);
        SafeRelease(swapChain);
    }

    bool D3D11GPUContext::BeginFrameImpl()
    {
        if (colorTextures.empty()) {
            CreateObjects();
        }

        return !device->IsLost();
    }

    void D3D11GPUContext::EndFrameImpl()
    {
        if (device->IsLost()) {
            return;
        }

        if (swapChain)
        {
            uint32 syncInterval = 1u;
            uint32 presentFlags = 0u;

            if (!verticalSync)
            {
                syncInterval = 0u;
                presentFlags = device->IsTearingSupported() ? DXGI_PRESENT_ALLOW_TEARING : 0u;
            }

            HRESULT hr = swapChain->Present(syncInterval, presentFlags);
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                device->HandleDeviceLost(hr);
            }
        }

        // TODO: Measure timestamp using query
        if (isMain) {
            device->Frame();
        }
    }

    void D3D11GPUContext::CreateObjects()
    {
        if (window)
        {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            const DXGI_SCALING dxgiScaling = DXGI_SCALING_STRETCH;

            DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

            if (!any(device->GetDXGIFactoryCaps() & DXGIFactoryCaps::FlipPresent))
            {
                swapEffect = DXGI_SWAP_EFFECT_DISCARD;
            }
#else
            const DXGI_SCALING dxgiScaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
            const DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

            DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
            swapchainDesc.Width = extent.width;
            swapchainDesc.Height = extent.height;
            swapchainDesc.Format = ToDXGIFormat(SRGBToLinearFormat(colorFormat));
            swapchainDesc.Stereo = false;
            swapchainDesc.SampleDesc.Count = 1;
            swapchainDesc.SampleDesc.Quality = 0;
            swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchainDesc.BufferCount = kInflightFrameCount;
            swapchainDesc.Scaling = dxgiScaling;
            swapchainDesc.SwapEffect = swapEffect;
            swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            if (device->IsTearingSupported())
            {
                swapchainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapchainDesc = {};
            fsSwapchainDesc.Windowed = !isFullscreen;

            // Create a SwapChain from a Win32 window.
            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForHwnd(
                device->GetD3DDevice(),
                window,
                &swapchainDesc,
                &fsSwapchainDesc,
                nullptr,
                &swapChain
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(device->GetDXGIFactory()->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
            IDXGISwapChain1* tempSwapChain;
            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
                device->GetD3DDevice(),
                window,
                &swapchainDesc,
                nullptr,
                &tempSwapChain
            ));
            ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));
            SafeRelease(tempSwapChain);
#endif

            CreateSwapChainObjects();
        }
        else
        {
            /* TODO: Offscreen rendering. */
        }

        if (depthStencilFormat != PixelFormat::Invalid)
        {
            GPUTextureDescription depthTextureDesc = GPUTextureDescription::New2D(depthStencilFormat, extent.width, extent.height, false, TextureUsage::RenderTarget);
            depthStencilTexture.Reset(new D3D11GPUTexture(device, depthTextureDesc, nullptr));
        }
    }

    void D3D11GPUContext::CreateSwapChainObjects()
    {
        colorTextures.clear();
        depthStencilTexture.Reset();

        // Create a render target view of the swap chain back buffer.
        ID3D11Texture2D* backbufferTexture;
        ThrowIfFailed(swapChain->GetBuffer(0, IID_PPV_ARGS(&backbufferTexture)));
        colorTextures.push_back(MakeRef<D3D11GPUTexture>(device, backbufferTexture, colorFormat));
        backbufferTexture->Release();
    }

    void D3D11GPUContext::Flush()
    {
        handle->Flush();
    }

    void D3D11GPUContext::PushDebugGroup(const String& name)
    {
        auto wideName = ToUtf16(name);
        annotation->BeginEvent(wideName.c_str());
    }

    void D3D11GPUContext::PopDebugGroup()
    {
        annotation->EndEvent();
    }

    void D3D11GPUContext::InsertDebugMarker(const String& name)
    {
        auto wideName = ToUtf16(name);
        annotation->SetMarker(wideName.c_str());
    }

    void D3D11GPUContext::BeginRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        ID3D11RenderTargetView* renderTargetViews[kMaxColorAttachments];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            D3D11GPUTexture* texture = static_cast<D3D11GPUTexture*>(colorAttachments[i].texture);

            ID3D11RenderTargetView* rtv = texture->GetRTV(DXGI_FORMAT_UNKNOWN, colorAttachments[i].mipLevel, colorAttachments[i].slice);

            switch (colorAttachments[i].loadAction)
            {
            case LoadAction::DontCare:
                handle->DiscardView(rtv);
                break;

            case LoadAction::Clear:
                handle->ClearRenderTargetView(rtv, &colorAttachments[i].clearColor.r);
                break;

            default:
                break;
            }

            renderTargetViews[i] = rtv;
        }

        handle->OMSetRenderTargets(numColorAttachments, renderTargetViews, nullptr);
    }

    void D3D11GPUContext::EndRenderPass()
    {
        /* TODO: Resolve */

        handle->OMSetRenderTargets(kMaxColorAttachments, zeroRTVS, nullptr);
    }

    void D3D11GPUContext::SetScissorRect(const URect& scissorRect)
    {
        D3D11_RECT d3dScissorRect;
        d3dScissorRect.left = static_cast<LONG>(scissorRect.x);
        d3dScissorRect.top = static_cast<LONG>(scissorRect.y);
        d3dScissorRect.right = static_cast<LONG>(scissorRect.x + scissorRect.width);
        d3dScissorRect.bottom = static_cast<LONG>(scissorRect.y + scissorRect.height);
        handle->RSSetScissorRects(1, &d3dScissorRect);
    }

    void D3D11GPUContext::SetScissorRects(const URect* scissorRects, uint32_t count)
    {
        D3D11_RECT d3dScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        for (uint32 i = 0; i < count; i++)
        {
            d3dScissorRects[i].left = static_cast<LONG>(scissorRects[i].x);
            d3dScissorRects[i].top = static_cast<LONG>(scissorRects[i].y);
            d3dScissorRects[i].right = static_cast<LONG>(scissorRects[i].x + scissorRects[i].width);
            d3dScissorRects[i].bottom = static_cast<LONG>(scissorRects[i].y + scissorRects[i].height);
        }

        handle->RSSetScissorRects(count, d3dScissorRects);
    }

    void D3D11GPUContext::SetViewport(const Viewport& viewport)
    {

    }

    void D3D11GPUContext::SetViewports(const Viewport* viewports, uint32_t count)
    {

    }

    void D3D11GPUContext::SetBlendColor(const Color& color)
    {

    }

    void D3D11GPUContext::BindBuffer(uint32_t slot, GPUBuffer* buffer)
    {

    }

    void D3D11GPUContext::BindBufferData(uint32_t slot, const void* data, uint32_t size)
    {

    }
}

