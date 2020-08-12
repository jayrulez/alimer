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

#include "config.h"
#include "Core/Log.h"
#include "Math/MathHelper.h"
#include "Graphics/Graphics.h"
#include "Graphics/GraphicsImpl.h"

#if defined(ALIMER_D3D11)
#include "Graphics/D3D11/D3D11GraphicsImpl.h"
#endif

#if defined(ALIMER_VULKAN)
//#include "Graphics/Vulkan/VulkanGraphicsDevice.h"
#endif

namespace alimer
{
    RefPtr<Graphics> graphics;

    Graphics::Graphics(RendererType rendererType)
        : impl(nullptr)
        , frameCount(0)
    {
        window = new Window();

        switch (rendererType)
        {
        case RendererType::Null:
            break;

#if defined(ALIMER_VULKAN)
        case RendererType::Vulkan:
            break;
#endif

#if defined(ALIMER_METAL)
        case RendererType::Metal:
            break;
#endif

#if defined(ALIMER_D3D11)
        case RendererType::Direct3D11:
            impl = new D3D11GraphicsImpl();
            break;
#endif

#if defined(ALIMER_D3D12)
        case RendererType::Direct3D12:
            break;
#endif

        default:
            break;
        }

        RegisterSubsystem(this);
        Texture::RegisterObject();
    }

    Graphics::~Graphics()
    {
        SafeDelete(impl);
        //RemoveSubsystem(this);
    }

    Graphics* Graphics::Create(RendererType preferredRendererType)
    {
        if (graphics != nullptr)
        {
            LOGE("Cannot create more than one Graphics instance.");
            return nullptr;
        }

        if (preferredRendererType == RendererType::Count)
        {
            preferredRendererType = RendererType::Direct3D11;
        }

        graphics = new Graphics(preferredRendererType);
        return graphics.get();
    }

    bool Graphics::SetMode(const UInt2& size, WindowFlags windowFlags, uint32_t sampleCount_)
    {
        sampleCount = Clamp(sampleCount_, 1u, 16u);

        if (!window->SetSize(size, windowFlags))
            return false;

        if (impl->IsInitialized()) {
            // TODO: Resize
        }
        else {
            impl->Initialize(window->GetHandle(), window->GetWidth(), window->GetHeight(), window->IsFullscreen());
        }

        return impl->IsInitialized();
    }

    void Graphics::SetVerticalSync(bool value)
    {
        impl->SetVerticalSync(value);
    }

    bool Graphics::BeginFrame()
    {
        return impl->BeginFrame();
    }

    void Graphics::EndFrame()
    {
        ++frameCount;
        impl->EndFrame(frameCount);
    }

    const GraphicsCapabilities& Graphics::GetCaps() const
    {
        return impl->GetCaps();
    }
}

