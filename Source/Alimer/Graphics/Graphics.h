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

#include "Graphics/CommandContext.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Buffer.h"
#include <vector>
#include <mutex>
#include <set>

namespace alimer
{
    /// Enum describing the rendering backend.
    enum class RendererType
    {
        /// Null renderer.
        Null,
        /// Direct3D 11 backend.
        Direct3D11,
        /// Direct3D 12 backend.
        Direct3D12,
        /// Vulkan backend.
        Vulkan,
        /// OpenGL backend.
        OpenGL,
        /// Default best platform supported backend.
        Count
    };

    /// Describes GraphicsDevice capabilities.
    struct GraphicsDeviceCapabilities
    {
        RendererType backendType;
        uint32_t vendorId;
        uint32_t deviceId;
        std::string adapterName;
        GPUAdapterType adapterType;

        struct Features
        {
            bool independentBlend = false;
            bool computeShader = false;
            bool geometryShader = false;
            bool tessellationShader = false;
            bool logicOp = false;
            bool multiViewport = false;
            bool fullDrawIndexUint32 = false;
            bool multiDrawIndirect = false;
            bool fillModeNonSolid = false;
            bool samplerAnisotropy = false;
            bool textureCompressionETC2 = false;
            bool textureCompressionASTC_LDR = false;
            bool textureCompressionBC = false;
            /// Specifies whether cube array textures are supported.
            bool textureCubeArray = false;
            /// Specifies whether raytracing is supported.
            bool raytracing = false;
        };

        struct Limits
        {
            uint32_t maxVertexAttributes;
            uint32_t maxVertexBindings;
            uint32_t maxVertexAttributeOffset;
            uint32_t maxVertexBindingStride;
            uint32_t maxTextureDimension2D;
            uint32_t maxTextureDimension3D;
            uint32_t maxTextureDimensionCube;
            uint32_t maxTextureArrayLayers;
            uint32_t maxColorAttachments;
            uint32_t maxUniformBufferSize;
            uint32_t minUniformBufferOffsetAlignment;
            uint32_t maxStorageBufferSize;
            uint32_t minStorageBufferOffsetAlignment;
            uint32_t maxSamplerAnisotropy;
            uint32_t maxViewports;
            uint32_t maxViewportWidth;
            uint32_t maxViewportHeight;
            uint32_t maxTessellationPatchSize;
            float pointSizeRangeMin;
            float pointSizeRangeMax;
            float lineWidthRangeMin;
            float lineWidthRangeMax;
            uint32_t maxComputeSharedMemorySize;
            uint32_t maxComputeWorkGroupCountX;
            uint32_t maxComputeWorkGroupCountY;
            uint32_t maxComputeWorkGroupCountZ;
            uint32_t maxComputeWorkGroupInvocations;
            uint32_t maxComputeWorkGroupSizeX;
            uint32_t maxComputeWorkGroupSizeY;
            uint32_t maxComputeWorkGroupSizeZ;
        };

        Features features;
        Limits limits;
    };

    struct GraphicsSettings final
    {
        std::string applicationName = "Alimer";
        bool debug = false;
        PixelFormat colorFormat = PixelFormat::BGRA8Unorm;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        bool vSync = true;
        uint32_t sampleCount = 1u;
    };

    class ALIMER_API GraphicsDeviceEvents
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~GraphicsDeviceEvents() = default;
    };

    class Window;

    /// Defines the logical graphics subsystem.
    class ALIMER_API Graphics : public Object
    {
        friend class GraphicsResource;
        ALIMER_OBJECT(Graphics, Object);

    public:
        virtual ~Graphics();

        static RendererType GetDefaultRenderer(RendererType preferredBackend);
        static std::set<RendererType> GetAvailableRenderDrivers();

        static Graphics* Create(RendererType preferredBackend, Window& window, const GraphicsSettings& settings);

        virtual void WaitForGPU() = 0;
        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        /// Get the device capabilities.
        const GraphicsDeviceCapabilities& GetCaps() const { return caps; }

        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        Graphics(Window& window);
        void ReleaseTrackedResources();

        Window& window;
        GraphicsDeviceCapabilities caps{};

        std::mutex trackedResourcesMutex;
        std::vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;

    private:
        ALIMER_DISABLE_COPY_MOVE(Graphics);
    };
}

