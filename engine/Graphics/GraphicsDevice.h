//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "Core/Ptr.h"
#include "Graphics/Texture.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <set>
#include <mutex>
#include <memory>

namespace alimer
{
#if ALIMER_PLATFORM_WINDOWS
    using WindowHandle = HWND;
#elif ALIMER_PLATFORM_UWP
    using WindowHandle = Platform::Agile<Windows::UI::Core::CoreWindow>;
#elif ALIMER_PLATFORM_ANDROID
    using WindowHandle = ANativeWindow*;
#else
    using WindowHandle = void*;
#endif

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define DEFAULT_ENABLE_DEBUG_LAYER true
#else
#   define DEFAULT_ENABLE_DEBUG_LAYER false
#endif

    class ALIMER_API GraphicsDeviceEvents
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~GraphicsDeviceEvents() = default;
    };

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice
    {
        friend class GraphicsResource;

    public:
        struct Desc
        {
            BackendType preferredBackendType = BackendType::Count;
            bool enableValidationLayer = DEFAULT_ENABLE_DEBUG_LAYER;
            PowerPreference powerPreference = PowerPreference::Default;

            PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
            PixelFormat depthStencilFormat = PixelFormat::Depth32Float;  
            bool enableVsync = true;                                     
        };

        virtual ~GraphicsDevice() = default;

        static std::set<BackendType> GetAvailableBackends();
        static std::unique_ptr<GraphicsDevice> Create(WindowHandle window, const Desc& desc);

        virtual void WaitForGPU() = 0;
        void BeginFrame();
        void EndFrame();

        void Resize(uint32_t width, uint32_t height);

        virtual RefPtr<Texture> CreateTexture(const TextureDescription& desc, const void* initialData) = 0;
        RefPtr<Texture> CreateTexture2D(uint32_t width, uint32_t height, PixelFormat format, uint32_t mipLevels = kMaxPossibleMipLevels, uint32_t arraySize = 1, TextureUsage usage = TextureUsage::Sampled, const void* initialData = nullptr);
        RefPtr<Texture> CreateTextureCube(uint32_t size, PixelFormat format, uint32_t mipLevels = kMaxPossibleMipLevels, uint32_t arraySize = 1, TextureUsage usage = TextureUsage::Sampled, const void* initialData = nullptr);

        /// Get the device capabilities.
        const GraphicsDeviceCaps& GetCaps() const { return caps; }

        /// Get the current backbuffer texture.
        Texture* GetBackbufferTexture() const;

        /// Get the depth stencil texture.
        Texture* GetDepthStencilTexture() const;

    private:
        virtual void Shutdown() = 0;
        virtual bool BeginFrameImpl() { return true; }
        virtual void EndFrameImpl() = 0;

        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        GraphicsDevice(const Desc& desc);
        void ReleaseTrackedResources();

        GraphicsDeviceCaps caps{};
        Desc desc;
        SizeU size{};
        float dpiScale = 1.0f;
        PixelFormat colorFormat;
        PixelFormat depthStencilFormat;

        std::mutex trackedResourcesMutex;
        std::vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;

        /// Current active frame index
        uint32_t frameIndex{ 0 };

        /// Whether a frame is active or not
        bool frameActive{ false };

        uint32_t backbufferIndex{ 0 };
        std::vector<RefPtr<Texture>> backbufferTextures;
        RefPtr<Texture> depthStencilTexture;

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
